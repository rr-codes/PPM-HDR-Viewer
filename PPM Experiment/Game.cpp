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

		m_gamePad = std::make_unique<DirectX::GamePad>();

		m_deviceResources->GoFullscreen();

		m_stereoViews = m_controller->SetFlickerStereoViews(m_controller->GetRun()->trials[0]);

		m_responseView = m_controller->SetStaticStereoView({
			wd + "/instructions/responsescreen_L.ppm",
			wd + "/instructions/responsescreen_R.ppm"
		});

		m_controller->GetFlickerTimer()->Start();
		m_controller->GetFPSTimer()->Start();
	}

#pragma region Frame Update
	// Executes the basic game loop.
	void Game::Tick()
	{
		m_controller->GetFPSTimer()->Tick([&]()
			{
				m_controller->GetAudioEngine()->Update();
				OnGamePadButton();
			});

		m_controller->GetFlickerTimer()->Tick([=](){Update();});
	}

	void Game::OnEscapeKeyDown()
	{
		m_deviceResources->GetSwapChain()->SetFullscreenState(false, nullptr);

		delete m_controller;

		m_spriteBatch.reset();
		m_gamePad.reset();

		m_deviceResources.reset();
		exit(0);
	}


	void Game::OnGamePadButton(const WPARAM key)
	{
		const auto state = m_gamePad->GetState(0);

		auto shouldGoToNextImage = state.IsConnected()
			? m_controller->GetResponse(state)
			: m_controller->GetResponse(key);

		if (shouldGoToNextImage)
		{
			m_controller->GetStopwatch()->Restart();
			Update();

			auto q = m_controller->GetRun()->trials[++m_controller->m_currentImageIndex];
			m_stereoViews = m_controller->SetFlickerStereoViews(q);
		}
	}

	// Updates the world.
	void Game::Update()
	{
		// before session has started, present the start screen
		if (!m_controller->m_startButtonHasBeenPressed)
		{
			const auto stereo = m_controller->SetStaticStereoView({
				wd + "/instructions/startscreen_L.ppm",
				wd + "/instructions/startscreen_R.ppm"
			});

			Render(stereo);
			return;
		}

		const auto elapsed = m_controller->GetStopwatch()->Elapsed();
		const auto delta = Configuration::ImageTransitionDuration;

		// if it is transiting between two images, show a black screen for the duration of the transition (intermediateDuration)
		if (elapsed < delta)
		{
			auto black = m_controller->SetStaticStereoView({
				wd + "/black/blackscreen_L.ppm",
				wd + "/black/blackscreen_R.ppm"
			});

			Render(black);
			return;
		}

		// if more than timeOut time has passed with the image visible, render the response view
		if (elapsed > Configuration::ImageTimeoutDuration + delta)
		{
			Render(m_responseView);
			return;
		}

		// under all other circumstances render the appropriate pair of DuoViews
		Render(m_shouldFlicker ? m_stereoViews.first : m_stereoViews.second);
		m_shouldFlicker = !m_shouldFlicker;
	}
#pragma endregion

	void Game::Render(const DuoView& duo_view)
	{
		RenderBase([&](int i)
		{
			for (size_t j = 0; j < 2; j++)
			{
				m_spriteBatch->Draw(
					duo_view[i][j].image.Get(),
					duo_view[i][j].position,
					nullptr,
					DirectX::Colors::White,
					0,
					DirectX::g_XMZero,
					1.0,
					DirectX::SpriteEffects_FlipHorizontally
				);
			}
		});
	}

	void Game::Render(const SingleView& single_view)
	{
		RenderBase([&](int i)
		{
			m_spriteBatch->Draw(single_view[i].image.Get(), single_view[i].position);
		});
	}

	template<typename F>
	void Game::RenderBase(F&& drawFunction)
	{
		
		auto context = m_deviceResources->GetD3DDeviceContext();

		Clear();

		m_deviceResources->PIXBeginEvent(L"Render");

		m_spriteBatch->Begin();

		for (size_t i = 0; i < 2; ++i)
		{
			drawFunction(i);
		}

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
		// TODO: Game is becoming active window.
		m_gamePad->Resume();
		m_buttons.Reset();
	}

	void Game::OnDeactivated()
	{
		// TODO: Game is becoming background window.
		m_gamePad->Suspend();
	}

	void Game::OnSuspending()
	{
		// TODO: Game is being power-suspended (or minimized).
		m_gamePad->Suspend();
	}

	void Game::OnResuming()
	{
		m_gamePad->Resume();
		m_buttons.Reset();
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
		catch (cv::Exception& e)
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