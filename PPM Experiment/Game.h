//
// Game.h
//

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"
#include "RenderTexture.h"
#include "SpriteBatch.h"
#include "GamePad.h"
#include "Participant.h"
#include "SimpleMath.h"
#include "Controller.h"
#include <PostProcess.h>

namespace Experiment {

	using string_ref = const std::string &;

	// A basic game implementation that creates a D3D11 device and
	// provides a game loop.
	class Game final : public DX::IDeviceNotify
	{
	public:
		Game(Run run) noexcept(false);

		// Initialization and management
		void Initialize(HWND windows[], int width, int height);
		// Basic game loop
		void Tick();
		void OnEscapeKeyDown();
		void OnGamePadButton(WPARAM key = 0x0);

		// IDeviceNotify
		virtual void OnDeviceLost() override;
		virtual void OnDeviceRestored() override;

		// Messages
		void OnActivated();
		void OnDeactivated();
		void OnSuspending();
		void OnResuming();
		void OnWindowMoved(int i);
		void OnWindowSizeChanged(int i, int width, int height);

		// Properties
		void GetDefaultSize(int& width, int& height) const;

		void Clear(int i);

	private:

		void Update();

		void Render(const DuoView& duo_view);
		void Render(const SingleView& single_view);

		void CreateDeviceDependentResources();
		void CreateWindowSizeDependentResources();

		// Device resources.
		std::unique_ptr<DX::DeviceResources> m_deviceResources;

		std::unique_ptr<DirectX::SpriteBatch> m_spriteBatch;

		std::unique_ptr<DX::RenderTexture>* m_hdrScene;
		std::unique_ptr<DirectX::ToneMapPostProcess>* m_toneMap;

		bool m_shouldFlicker = false;

		std::unique_ptr<DirectX::GamePad>  m_gamePad;
		DirectX::GamePad::ButtonStateTracker m_buttons;

		Run m_run;
		Controller* m_controller;

		std::pair<DuoView, DuoView> m_stereoViews;
		SingleView m_responseView;
	};

}
