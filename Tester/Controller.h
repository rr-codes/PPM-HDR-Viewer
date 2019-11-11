#pragma once
#include <wrl/client.h>
#include "DeviceResources.h"
#include "Stopwatch.h"
#include "Participant.h"
#include "Utils.h"
#include <filesystem>
#include <SimpleMath.h>

constexpr auto FAILURE = L"Success3.wav";

namespace Experiment
{
	struct Vector;

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

		[[nodiscard]] DuoView SetFlickerStereoViews(const std::array<std::string, 4>& files) const;

		[[nodiscard]] SingleView SetStaticStereoView(const Utils::Duo<std::filesystem::path>& views) const;
		SingleView PathToSingleView(const std::filesystem::path& path) const;

		[[nodiscard]] Utils::Timer<>* GetFlickerTimer() const { return m_flickerTimer.get(); }

		SingleView GetCurrentImage()
		{
			return PathToSingleView(m_run.files[m_currentImageIndex]);
		}

		int numberOfImages() const { return m_run.files.size(); }

		int m_currentImageIndex = 0;

	private:
		
		[[nodiscard]] Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> ToResource(const std::filesystem::path& image) const;
		[[nodiscard]] Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> ToResource(const std::filesystem::path& image, Vector region) const;
		template <class F>
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> ToResourceBase(const std::filesystem::path& image,  F&& matTransformFunction) const;

		DX::DeviceResources* m_deviceResources;
		Experiment::Run m_run;

		std::unique_ptr<Utils::Timer<>> m_flickerTimer;

	};

}
