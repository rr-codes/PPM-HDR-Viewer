#pragma once
#include <wrl/client.h>
#include "DeviceResources.h"
#include <GamePad.h>
#include "Stopwatch.h"
#include "StepTimer.h"

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

		bool GetResponse(DirectX::GamePad::State state);

		Utils::Counter* GetCounter() { return m_counter.get(); }

		int m_currentImageIndex = 0;
		bool m_startButtonHasBeenPressed = false;

	private:
		[[nodiscard]] ComPtr<ID3D11ShaderResourceView> ConvertImageToResource(
			const std::filesystem::path& image,
			Vector* region
		) const;

		DX::DeviceResources* m_deviceResources;
		Experiment::Run m_run;

		DirectX::GamePad::ButtonStateTracker m_buttons;

		std::unique_ptr<Utils::Counter> m_counter;
	};

}
