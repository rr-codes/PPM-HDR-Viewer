//
// Game.cpp
//

#include "pch.h"
#include "Game.h"
#include <filesystem>
#include <utility>
#include "Controller.h"
#include "Stopwatch.h"

extern void ExitGame();

namespace Experiment
{
	const static std::string wd = std::filesystem::cwd().generic_string();

	Game::Game(Run& run) noexcept(false)
	{
		m_deviceResources = std::make_unique<DX::DeviceResources>(
			DXGI_FORMAT_R10G10B10A2_UNORM,
			DXGI_FORMAT_D32_FLOAT,
			2,
			D3D_FEATURE_LEVEL_10_0,
			DX::DeviceResources::c_EnableHDR
			);

		m_deviceResources->RegisterDeviceNotify(this);
		m_controller = new Controller(run, m_deviceResources.get());

		// m_hdrScene = new std::unique_ptr<DX::RenderTexture>[NUMBER_OF_WINDOWS];
		// m_toneMap = new std::unique_ptr<DirectX::ToneMapPostProcess>[NUMBER_OF_WINDOWS];

		m_hdrScene = std::make_unique<DX::RenderTexture>(DXGI_FORMAT_R16G16B16A16_FLOAT);
	}

	// Initialize the Direct3D resources required to run.
	void Game::Initialize(HWND window, int width, int height)
	{
		while (ShowCursor(false) >= 0)
		{
		}

		m_deviceResources->SetWindow(window, width, height);

		m_deviceResources->CreateDeviceResources();
		CreateDeviceDependentResources();

		m_deviceResources->CreateWindowSizeDependentResources();
		CreateWindowSizeDependentResources();


		m_deviceResources->GoFullscreen();

		m_stereoViews = m_controller->GetCurrentImage();

		m_controller->GetFlickerTimer()->Start();
	}

#pragma region Frame Update
	// Executes the basic game loop.
	void Game::Tick()
	{
		m_controller->GetFlickerTimer()->Tick([=]() {Update(); });
	}

	void Game::OnEscapeKeyDown()
	{
		m_deviceResources->GetSwapChain()->SetFullscreenState(false, nullptr);

		delete m_controller;

		m_spriteBatch.reset();

		m_deviceResources.reset();
		exit(0);
	}

	void Game::OnArrowKeyDown(WPARAM key)
	{
		if (key == VK_LEFT)
		{
			m_controller->m_currentImageIndex = (m_controller->m_currentImageIndex == 0)
				? m_controller->numberOfImages() - 1
				: m_controller->m_currentImageIndex - 1;
		}
		else if (key == VK_RIGHT)
		{
			m_controller->m_currentImageIndex = (m_controller->m_currentImageIndex + 1) % m_controller->numberOfImages();
		}

		m_stereoViews = m_controller->GetCurrentImage();
	}

	// Updates the world.
	void Game::Update()
	{
		// under all other circumstances render the appropriate pair of DuoViews
		Render(m_stereoViews);
	}
#pragma endregion

	bool flickerToggle = true;

	/// Used to render flickering images on a single display
	void Game::Render(const StereoFlickerArtefact<Image>& view)
	{

		auto context = m_deviceResources->GetD3DDeviceContext();

		Clear();

		m_deviceResources->PIXBeginEvent(L"Render");

		m_spriteBatch->Begin();

		m_spriteBatch->Draw(
			(flickerToggle ? view.leftOriginal : view.leftCompressed).Get(),
			{ 0, 0 },
			nullptr,
			DirectX::Colors::White,
			0,
			DirectX::g_XMZero,
			1.0,
			DirectX::SpriteEffects_FlipHorizontally
		);

		m_spriteBatch->Draw(
			(flickerToggle ? view.rightOriginal : view.rightCompressed).Get(),
			{ 3840, 0 },
			nullptr,
			DirectX::Colors::White,
			0,
			DirectX::g_XMZero,
			1.0,
			DirectX::SpriteEffects_FlipHorizontally
		);


		flickerToggle = !flickerToggle;

		m_spriteBatch->End();

		m_deviceResources->PIXEndEvent();

		auto renderTarget = m_deviceResources->GetRenderTargetView();
		context->OMSetRenderTargets(1, &renderTarget, nullptr);

		m_toneMap->Process(context);

		ID3D11ShaderResourceView* nullsrv[] = { nullptr };
		context->PSSetShaderResources(0, 1, nullsrv);

		m_deviceResources->ThreadPresent();

		m_deviceResources->DiscardView();
	}

#pragma region Frame Render
	// Helper method to clear the back buffers.
	void Game::Clear()
	{
		m_deviceResources->PIXBeginEvent(L"Clear");

		// Clear the views.
		auto context = m_deviceResources->GetD3DDeviceContext();

		auto renderTarget = m_hdrScene->GetRenderTargetView();
		const auto depthStencil = m_deviceResources->GetDepthStencilView();

		DirectX::XMVECTORF32 color;
		auto actual = DirectX::FXMVECTOR({ {0, 0, 0, 0} });
		color.v = DirectX::XMColorSRGBToRGB(actual);
		context->ClearRenderTargetView(renderTarget, color);


		context->ClearDepthStencilView(depthStencil, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
		context->OMSetRenderTargets(1, &renderTarget, depthStencil);

		// Set the viewport.
		auto viewport = m_deviceResources->GetScreenViewport();
		context->RSSetViewports(1, &viewport);

		m_deviceResources->PIXEndEvent();
	}
#pragma endregion

#pragma region Message Handlers
	// Message handlers
	void Game::OnActivated()
	{

	}

	void Game::OnDeactivated()
	{
		// TODO: Game is becoming background window.
	}

	void Game::OnSuspending()
	{
		// TODO: Game is being power-suspended (or minimized).
	}

	void Game::OnResuming()
	{
	}

	void Game::OnWindowMoved()
	{
		auto r = m_deviceResources->GetOutputSize();
		m_deviceResources->WindowSizeChanged(r.right, r.bottom);
	}

	void Game::OnWindowSizeChanged(int width, int height)
	{
		if (!m_deviceResources->WindowSizeChanged(width, height))
			return;

		CreateWindowSizeDependentResources();
	}

	// Properties
	void Game::GetDefaultSize(int& width, int& height) const
	{
		width = 7680;
		height = 2160;
	}
#pragma endregion

#pragma region Direct3D Resources
	// These are the resources that depend on the device.
	void Game::CreateDeviceDependentResources()
	{
		auto device = m_deviceResources->GetD3DDevice();

		try {
			cv::directx::ocl::initializeContextFromD3D11Device(device);
		}
		catch (cv::Exception & e)
		{
			auto msg = e.msg;
			throw;
		}

		m_spriteBatch = std::make_unique<DirectX::SpriteBatch>(m_deviceResources->GetD3DDeviceContext());

		m_hdrScene->SetDevice(device);
		m_toneMap = std::make_unique<DirectX::ToneMapPostProcess>(device);

		m_toneMap->SetST2084Parameter(64);
	}

	// Allocate all memory resources that change on a window SizeChanged event.
	void Game::CreateWindowSizeDependentResources() const
	{
		auto size = m_deviceResources->GetOutputSize();
		m_hdrScene->SetWindow(size);

		m_toneMap->SetHDRSourceTexture(m_hdrScene->GetShaderResourceView());
	}

	void Game::OnDeviceLost()
	{
		m_hdrScene->ReleaseDevice();

		m_toneMap.reset();
	}

	void Game::OnDeviceRestored()
	{
		CreateDeviceDependentResources();

		CreateWindowSizeDependentResources();
		CreateWindowSizeDependentResources();
	}
#pragma endregion

}