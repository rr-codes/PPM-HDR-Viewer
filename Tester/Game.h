//
// Game.h
//

#pragma once

#include "DeviceResources.h"
#include "RenderTexture.h"
#include "SpriteBatch.h"
#include "Participant.h"
#include "SimpleMath.h"
#include "Controller.h"
#include <PostProcess.h>

namespace Experiment {


	// A basic game implementation that creates a D3D11 device and
	// provides a game loop.
	class Game final : public DX::IDeviceNotify
	{
	public:

		Game(Run& run);
		// Initialization and management
		void Initialize(HWND window, int width, int height);
		// Basic game loop
		void Tick();
		void OnEscapeKeyDown();

		// IDeviceNotify
		virtual void OnDeviceLost() override;
		virtual void OnDeviceRestored() override;

		// Messages
		void OnActivated();
		void OnDeactivated();
		void OnSuspending();
		void OnResuming();
		void OnWindowMoved();
		void OnWindowSizeChanged(int width, int height);

		// Properties
		void GetDefaultSize(int& width, int& height) const;

		void Clear();

	private:

		void Update();

		/// This renders two seperate stereo images displayed concurrently given a DuoView
		void Render(const DuoView& duo_view);

		/// This renders a single fullscreen stereo image from a Duo of ShaderViews
		void Render(const SingleView& single_view);

		template<typename F>
		void RenderBase(F&& drawFunction);

		void CreateDeviceDependentResources();
		void CreateWindowSizeDependentResources() const;

		// Device resources.
		std::unique_ptr<DX::DeviceResources> m_deviceResources;

		std::unique_ptr<DirectX::SpriteBatch> m_spriteBatch;

		std::unique_ptr<DX::RenderTexture>		m_hdrScene;
		std::unique_ptr<DirectX::ToneMapPostProcess>	m_toneMap;

		bool m_shouldFlicker = false;


		Controller* m_controller;

		std::pair<DuoView, DuoView> m_stereoViews;
	};

}
