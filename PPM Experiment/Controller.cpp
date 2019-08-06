#include "Controller.h"
#include <utility>

namespace Experiment {

	Controller::Controller(Trial trial, DX::DeviceResources* deviceResources)
	{
		this->m_trial = std::move(trial);
		this->m_deviceResources = deviceResources;
	}


	static cv::Mat CropMatrix(const cv::Mat& mat, const cv::Rect& cropRegion)
	{
		const cv::Mat croppedRef(mat, cropRegion);

		cv::Mat cropped;
		croppedRef.copyTo(cropped);

		return cropped;
	}

	bool Controller::GetResponse(DirectX::GamePad::State state)
	{
		using Button = DirectX::GamePad::ButtonStateTracker;

		const auto right = m_buttons.rightTrigger;
		const auto left = m_buttons.leftTrigger;

		m_buttons.Update(state);

		if (m_buttons.a == Button::PRESSED || m_buttons.b == Button::PRESSED)
		{
			m_startButtonHasBeenPressed = true;
			ResetTimer();
			return false;
		}

		if (right != Button::PRESSED && left != Button::PRESSED)
		{
			return false;
		}


		if (!m_startButtonHasBeenPressed)
		{
			return false;
		}

		const auto elapsed = DeltaSeconds();

		const auto response = Response{
			(right == Button::PRESSED) ? Right : Left,
			elapsed
		};

		m_trial.responses.push_back(response);
		
		if (m_currentImageIndex + 1 >= static_cast<int>(m_trial.questions.size()))
		{
			m_trial.ExportResults("result" + m_trial.id + ".csv");

			exit(0);
			return false;
		}

		return true;
	}

	long long Controller::DeltaSeconds() const
	{
		const auto t = Clock::now();
		const auto t0 = m_start;

		const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(t - t0).count();
		return duration;
	}

	void Controller::ResetTimer()
	{
		Debug::Console::log("\nTimer reset\n");
		m_start = Clock::now();
	}

	ComPtr<ID3D11ShaderResourceView> Controller::ConvertImageToResource(const std::filesystem::path& image, Region* region) const
	{
		if (!is_regular_file(image))
		{
			Utils::FatalError("" + image.generic_string() + " is not a valid path");
		}

		Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shader;

		auto matrixoriginal = cv::imread(image.generic_string(), cv::IMREAD_ANYCOLOR | cv::IMREAD_ANYDEPTH);
		cv::Mat matrix(matrixoriginal.size(), CV_MAKE_TYPE(matrixoriginal.depth(), 4));

		// red is blue and blue is red and alpha is none
		int conversion[] = { 2, 0, 1, 1, 0, 2, -1, 3 };
		cv::mixChannels(&matrixoriginal, 1, &matrix, 1, conversion, 4);

		auto cropped = (region == nullptr) 
			? matrix 
			: CropMatrix(matrix, cv::Rect(region->x, region->y, region->w, region->h));

		D3D11_TEXTURE2D_DESC desc = {};
		desc.Width = cropped.cols;
		desc.Height = cropped.rows;
		desc.MipLevels = desc.ArraySize = 1;
		desc.Format = DXGI_FORMAT_R16G16B16A16_UNORM;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ;
		desc.MiscFlags = 0;

		auto hr = m_deviceResources->GetD3DDevice()->CreateTexture2D(&desc, nullptr, texture.GetAddressOf());

		DX::ThrowIfFailed(hr);

		try {
			cv::directx::convertToD3D11Texture2D(cropped, texture.Get());
		}
		catch (cv::Exception& e)
		{
			Utils::FatalError(e.err);
		}

		D3D11_SHADER_RESOURCE_VIEW_DESC desc2 = { };
		desc2.Format = DXGI_FORMAT_R16G16B16A16_UNORM;
		desc2.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		desc2.Texture2D.MipLevels = 1;

		m_deviceResources->m_d3dDevice->CreateShaderResourceView(
			texture.Get(),
			&desc2,
			shader.GetAddressOf()
		);

		return shader;
	}

	SingleView Controller::SetStaticStereoView(const Utils::Duo<std::filesystem::path>& views) const
	{
		const auto l = ConvertImageToResource(views.left, nullptr);
		const auto r = ConvertImageToResource(views.right, nullptr);

		return SingleView{ l, r };
	}


	std::pair<DuoView, DuoView> Controller::SetFlickerStereoViews(const Question& question) const
	{
		const auto start = m_trial.folderPath + "/" + question.image_name;
		std::vector<std::string> files = {
			start + "_L_dec.ppm",
			start + "_L_orig.ppm",
			start + "_R_dec.ppm",
			start + "_R_orig.ppm"
		};

		std::vector<ComPtr<ID3D11ShaderResourceView>> views;
		const auto dims = m_deviceResources->GetDimensions();

		views.reserve(files.size());
		for (auto& file : files)
		{
			auto region = question.region;
			views.push_back(ConvertImageToResource(file, &region));
		}

		auto left = DirectX::SimpleMath::Vector2(
			dims.x / 2 - static_cast<float>(m_trial.distance) / 2 - static_cast<float>(question.region.w),
			dims.y / 2 - static_cast<float>(question.region.h) / 2
		);

		auto right = DirectX::SimpleMath::Vector2(
			dims.x / 2 + static_cast<float>(m_trial.distance) / 2,
			dims.y / 2 - static_cast<float>(question.region.h) / 2
		);

		DuoView no_flicker = {};
		no_flicker.views.left = { views[1], views[1] };
		no_flicker.views.right = { views[3], views[3] };

		no_flicker.positions = { left, right };

		DuoView flicker = {};
		flicker.positions = { left, right };

		if (question.correct_option == Experiment::Left)
		{
			flicker.views.left = { views[0], views[1] };
			flicker.views.right = { views[2], views[3] };
		}
		else
		{
			flicker.views.left = { views[1], views[0] };
			flicker.views.right = { views[3], views[2] };
		}

		return std::make_pair(no_flicker, flicker);
	}

}