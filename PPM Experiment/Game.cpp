//
// Game.cpp
//

#include "pch.h"
#include "Game.h"
#include <synchapi.h>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <utility>
#include "Controller.h"
#include "Stopwatch.h"

extern void ExitGame();

namespace Experiment 
{

	Game::Game(Run run) noexcept(false) : m_run(std::move(run))
	{
		m_deviceResources = std::make_unique<DX::DeviceResources>(
			NUMBER_OF_WINDOWS,
			DXGI_FORMAT_R10G10B10A2_UNORM,
			DXGI_FORMAT_D32_FLOAT,
			2,
			D3D_FEATURE_LEVEL_10_0,
			DX::DeviceResources::c_EnableHDR
		);

		m_deviceResources->RegisterDeviceNotify(this);
		m_controller = new Controller(m_run, m_deviceResources.get());

		// m_hdrScene = new std::unique_ptr<DX::RenderTexture>[NUMBER_OF_WINDOWS];
		// m_toneMap = new std::unique_ptr<DirectX::ToneMapPostProcess>[NUMBER_OF_WINDOWS];

		for (int i = 0; i < NUMBER_OF_WINDOWS; i++) {
			m_hdrScene[i] = std::make_unique<DX::RenderTexture>(DXGI_FORMAT_R16G16B16A16_FLOAT);
		}
	}

	// Initialize the Direct3D resources required to run.
	void Game::Initialize(HWND windows[], int width, int height)
	{
		while (ShowCursor(false) >= 0)
		{
		}

		for (int i = 0; i < NUMBER_OF_WINDOWS; i++)
		{
			m_deviceResources->SetWindow(i, windows[i], width, height);
		}

		m_deviceResources->CreateDeviceResources();
		CreateDeviceDependentResources();

		m_deviceResources->CreateWindowSizeDependentResources();
		CreateWindowSizeDependentResources();

		m_gamePad = std::make_unique<DirectX::GamePad>();

		for (int i = 0; i < NUMBER_OF_WINDOWS; i++)
		{
			m_deviceResources->GoFullscreen(i);
		}

		m_stereoViews = m_controller->SetFlickerStereoViews(m_run.trials[0]);

		const auto dir = std::filesystem::current_path().generic_string() + "/instructions/";

		m_responseView = m_controller->SetStaticStereoView({
			dir + "responsescreen_L.ppm",
			dir + "responsescreen_R.ppm"
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

		m_controller->GetFlickerTimer()->Tick([&]()
		{
			Update();
		});
	}

	void Game::OnEscapeKeyDown()
	{
		for (int i = 0; i < NUMBER_OF_WINDOWS; i++)
		{
			m_deviceResources->GetSwapChain(i)->SetFullscreenState(false, nullptr);
		}

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
			auto q = m_run.trials[++m_controller->m_currentImageIndex];
			m_stereoViews = m_controller->SetFlickerStereoViews(q);

			m_controller->GetStopwatch()->Restart();
		}
	}

	// Updates the world.
	void Game::Update()
	{
		// before session has started, present the start screen
		if (!m_controller->m_startButtonHasBeenPressed)
		{
			const auto dir = std::filesystem::current_path().generic_string() + "/instructions/";
			
			const auto stereo = m_controller->SetStaticStereoView({
			dir + "startscreen_L.ppm",
			dir + "startscreen_R.ppm"
			});

			Render(stereo);
			
			return;
		}

		const auto elapsed = m_controller->GetStopwatch()->Elapsed();
		const auto delta = std::chrono::milliseconds(m_run.intermediateDuration);

		// if it is transiting between two images, show a black screen for the duration of the transition (intermediateDuration)
		if (elapsed < delta)
		{
			const auto dir = std::filesystem::current_path().generic_string() + "/black/";
			auto black = m_controller->SetStaticStereoView({ 
			dir + "blackscreen_L.ppm", 
			dir + "blackscreen_R.ppm"
			});

			Render(black);
			return;
		}

		// if more than timeOut time has passed with the image visible, render the response view
		if (elapsed > std::chrono::seconds(m_run.timeOut) + delta)
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
		auto context = m_deviceResources->GetD3DDeviceContext();

		for (int i = 0; i < NUMBER_OF_WINDOWS; i++)
		{
			Clear(i);

			m_deviceResources->PIXBeginEvent(L"Render");

			m_spriteBatch->Begin();

			m_spriteBatch->Draw(
				duo_view.views[i].left.Get(), 
				duo_view.positions.left, 
				nullptr, 
				DirectX::Colors::White, 
				0, 
				DirectX::g_XMZero, 
				1,
				DirectX::SpriteEffects_FlipHorizontally
			);

			m_spriteBatch->Draw(
				duo_view.views[i].right.Get(),
				duo_view.positions.right,
				nullptr,
				DirectX::Colors::White,
				0,
				DirectX::g_XMZero,
				1,
				DirectX::SpriteEffects_FlipHorizontally
			);

			m_spriteBatch->End();

			m_deviceResources->PIXEndEvent();

			auto renderTarget = m_deviceResources->GetRenderTargetView(i);
			context->OMSetRenderTargets(1, &renderTarget, nullptr);

			m_toneMap[i]->Process(context);
		}

		ID3D11ShaderResourceView* nullsrv[] = { nullptr };
		context->PSSetShaderResources(0, 1, nullsrv);

		m_deviceResources->ThreadPresent();

		for (int i = 0; i < NUMBER_OF_WINDOWS; i++)
		{
			m_deviceResources->DiscardView(i);
		}
	}

	void Game::Render(const SingleView& single_view)
	{
		auto context = m_deviceResources->GetD3DDeviceContext();

		for (int i = 0; i < NUMBER_OF_WINDOWS; i++)
		{
			Clear(i);

			m_deviceResources->PIXBeginEvent(L"Render");

			m_spriteBatch->Begin();
			
			m_spriteBatch->Draw(single_view[i].Get(), DirectX::SimpleMath::Vector2{ 0, 0 });

			m_spriteBatch->End();

			m_deviceResources->PIXEndEvent();

			auto renderTarget = m_deviceResources->GetRenderTargetView(i);
			context->OMSetRenderTargets(1, &renderTarget, nullptr);

			m_toneMap[i]->Process(context);
		}

		ID3D11ShaderResourceView* nullsrv[] = { nullptr };
		context->PSSetShaderResources(0, 1, nullsrv);

		m_deviceResources->ThreadPresent();

		for (int i = 0; i < NUMBER_OF_WINDOWS; i++)
		{
			m_deviceResources->DiscardView(i);
		}
	}

#pragma region Frame Render
	// Helper method to clear the back buffers.
	void Game::Clear(int i)
	{
		m_deviceResources->PIXBeginEvent(L"Clear");

		// Clear the views.
		auto context = m_deviceResources->GetD3DDeviceContext();

		auto renderTarget = m_hdrScene[i]->GetRenderTargetView();
		auto depthStencil = m_deviceResources->GetDepthStencilView();

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

	void Game::OnWindowMoved(int i)
	{
		auto r = m_deviceResources->GetOutputSize();
		m_deviceResources->WindowSizeChanged(i, r.right, r.bottom);
	}

	void Game::OnWindowSizeChanged(int i, int width, int height)
	{
		if (!m_deviceResources->WindowSizeChanged(i, width, height))
			return;

		CreateWindowSizeDependentResources();
	}

	// Properties
	void Game::GetDefaultSize(int& width, int& height) const
	{
		width = 1920;
		height = 1080;
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
			throw e;
		}

		m_spriteBatch = std::make_unique<DirectX::SpriteBatch>(m_deviceResources->GetD3DDeviceContext());

		for (int i = 0; i < NUMBER_OF_WINDOWS; i++) {
			m_hdrScene[i]->SetDevice(device);
			m_toneMap[i] = std::make_unique<DirectX::ToneMapPostProcess>(device);

			m_toneMap[i]->SetST2084Parameter(64);
		}
	}

	// Allocate all memory resources that change on a window SizeChanged event.
	void Game::CreateWindowSizeDependentResources()
	{
		auto size = m_deviceResources->GetOutputSize();
		for (int i = 0; i < NUMBER_OF_WINDOWS; i++) {
			m_hdrScene[i]->SetWindow(size);

			m_toneMap[i]->SetHDRSourceTexture(m_hdrScene[i]->GetShaderResourceView());
		}
	}

	void Game::OnDeviceLost()
	{
		for (int i = 0; i < NUMBER_OF_WINDOWS; i++) {
			m_hdrScene[i]->ReleaseDevice();

			m_toneMap[i].reset();
		}
	}

	void Game::OnDeviceRestored()
	{
		CreateDeviceDependentResources();

		CreateWindowSizeDependentResources();
		CreateWindowSizeDependentResources();
	}
#pragma endregion

}