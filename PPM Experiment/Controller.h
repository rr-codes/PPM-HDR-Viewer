#pragma once
#include <wrl/client.h>
#include "DeviceResources.h"
#include <GamePad.h>

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
		Controller(Trial trial, DX::DeviceResources* deviceResources);

		[[nodiscard]] std::pair<DuoView, DuoView> SetFlickerStereoViews(const Question& question) const;

		[[nodiscard]] SingleView SetStaticStereoView(const Utils::Duo<std::filesystem::path>& views) const;

		bool GetResponse(DirectX::GamePad* gamePad, DirectX::GamePad::State state);

		std::chrono::time_point<std::chrono::system_clock> last_time;
		int m_currentImageIndex = 0;
		bool m_startButtonHasBeenPressed = false;


	private:
		[[nodiscard]] ComPtr<ID3D11ShaderResourceView> ConvertImageToResource(
			const std::filesystem::path& image,
			Region* region
		) const;

		DX::DeviceResources* m_deviceResources;
		Experiment::Trial m_trial;

		DirectX::GamePad::ButtonStateTracker m_buttons;

	};

}