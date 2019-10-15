#pragma once
#include <wrl/client.h>
#include "DeviceResources.h"
#include <GamePad.h>
#include "Stopwatch.h"
#include "Participant.h"

constexpr auto FAILURE = L"Success3.wav";

namespace Experiment
{
	struct Image
	{
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> image;
		DirectX::SimpleMath::Vector2 position;
	};

	using DuoView = Utils::Duo<Utils::Duo<Image>>;
	using SingleView = Utils::Duo<Image>;
	

	class Controller
	{
	public:
		Controller(Run& run, DX::DeviceResources* deviceResources);

		[[nodiscard]] std::pair<DuoView, DuoView> SetFlickerStereoViews(const Trial& trial) const;

		[[nodiscard]] SingleView SetStaticStereoView(const Utils::Duo<std::filesystem::path>& views) const;

		bool GetResponse(WPARAM key);
		bool GetResponse(DirectX::GamePad::State state);

		[[nodiscard]] Utils::Timer<>* GetFPSTimer() const { return m_fpstimer.get(); }
		[[nodiscard]] Utils::Timer<>* GetFlickerTimer() const { return m_flickerTimer.get(); }

		[[nodiscard]] Utils::Stopwatch<>* GetStopwatch() const { return m_stopwatch.get(); }

		[[nodiscard]] DirectX::AudioEngine* GetAudioEngine() const { return m_audioEngine.get(); }

		int m_currentImageIndex = 0;
		bool m_startButtonHasBeenPressed = false;

		Run* GetRun()
		{
			return &m_run;
		}

	private:
		void AppendResponse(Option response);
		
		[[nodiscard]] Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> ToResource(const std::filesystem::path& image) const;
		[[nodiscard]] Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> ToResource(const std::filesystem::path& image, Vector region) const;
		template <class F>
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> ToResourceBase(const std::filesystem::path& image,  F&& matTransformFunction) const;

		DX::DeviceResources* m_deviceResources;
		Experiment::Run m_run;

		DirectX::GamePad::ButtonStateTracker m_buttons;

		std::unique_ptr<DirectX::AudioEngine> m_audioEngine;
		std::unique_ptr<DirectX::SoundEffect> m_failureSound;

		std::unique_ptr<Utils::Timer<>> m_fpstimer;
		std::unique_ptr<Utils::Timer<>> m_flickerTimer;

		std::unique_ptr<Utils::Stopwatch<>> m_stopwatch;
	};

}
