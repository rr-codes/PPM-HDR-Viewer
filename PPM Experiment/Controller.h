#pragma once
#include <wrl/client.h>
#include "DeviceResources.h"
#include <GamePad.h>
#include "Stopwatch.h"

constexpr auto SUCCESS = L"Success3.wav";
constexpr auto FAILURE = L"Error1.wav";

namespace Experiment {

	using Microsoft::WRL::ComPtr;

	struct DuoView
	{
		// The outer Duo is the windows, and the inner Duo the images in each window
		Utils::Duo<Utils::Duo<ComPtr<ID3D11ShaderResourceView>>> views = {};

		Utils::Duo<DirectX::SimpleMath::Vector2> positions;
	};

	using SingleView = Utils::Duo<ComPtr<ID3D11ShaderResourceView>>;

	class Controller
	{
	public:
		Controller(Run run, DX::DeviceResources* deviceResources);

		[[nodiscard]] std::pair<DuoView, DuoView> SetFlickerStereoViews(const Trial& trial) const;

		[[nodiscard]] SingleView SetStaticStereoView(const Utils::Duo<std::filesystem::path>& views) const;

		bool GetResponse(WPARAM key);
		bool GetResponse(DirectX::GamePad::State state);

		[[nodiscard]] Utils::Timer<>* GetFPSTimer() const { return m_fpstimer.get(); }
		[[nodiscard]] Utils::Timer<>* GetFlickerTimer() const { return m_flickerTimer.get(); }
		[[nodiscard]] Utils::Stopwatch<>* GetStopwatch() const { return m_stopwatch.get(); }

		DirectX::AudioEngine* GetAudioEngine() const { return m_audioEngine.get(); }

		int m_currentImageIndex = 0;
		bool m_startButtonHasBeenPressed = false;

	private:
		[[nodiscard]] ComPtr<ID3D11ShaderResourceView> ConvertImageToResource(
			const std::filesystem::path& image,
			Vector* region
		) const;

		void AppendResponse(Option response);

		DX::DeviceResources* m_deviceResources;
		Experiment::Run m_run;

		DirectX::GamePad::ButtonStateTracker m_buttons;

		std::unique_ptr<DirectX::AudioEngine> m_audioEngine;
		std::unique_ptr<DirectX::SoundEffect> m_successSound, m_failureSound;

		std::unique_ptr<Utils::Timer<>> m_fpstimer;
		std::unique_ptr<Utils::Timer<>> m_flickerTimer;
		std::unique_ptr<Utils::Stopwatch<>> m_stopwatch;
	};

}
