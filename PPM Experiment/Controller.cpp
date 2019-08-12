#include "Controller.h"
#include <utility>

extern void ExitGame();

namespace Experiment {

	Controller::Controller(Run run, DX::DeviceResources* deviceResources)
	{
		this->m_run = std::move(run);
		this->m_deviceResources = deviceResources;

		m_audioEngine = std::make_unique<DirectX::AudioEngine>(DirectX::AudioEngine_Default);

		const auto dir = std::filesystem::current_path().generic_wstring() + L"/sounds/";
		const auto success = dir + SUCCESS;
		const auto failure = dir + FAILURE;

		m_successSound = std::make_unique<DirectX::SoundEffect>(m_audioEngine.get(), success.c_str());
		m_failureSound = std::make_unique<DirectX::SoundEffect>(m_audioEngine.get(), failure.c_str());

		this->m_fpstimer = std::make_unique<Utils::Timer<>>(20);
		this->m_flickerTimer = std::make_unique<Utils::Timer<>>(m_run.flickerRate * 1000.0);
		this->m_stopwatch = std::make_unique<Utils::Stopwatch<>>();
	}

	static cv::Mat CropMatrix(const cv::Mat& mat, const cv::Rect& cropRegion)
	{
		const cv::Mat croppedRef(mat, cropRegion);

		cv::Mat cropped;
		croppedRef.copyTo(cropped);

		return cropped;
	}

	bool Controller::GetResponse(WPARAM key)
	{
		if (key == VK_RETURN)
		{
			m_startButtonHasBeenPressed = true;
			m_stopwatch->Restart();
			return false;
		}

		if (key != VK_LEFT && key != VK_RIGHT)
		{
			return false;
		}

		if (!m_startButtonHasBeenPressed)
		{
			return false;
		}

		const auto response = (key == VK_LEFT) ? Right : Left;

		m_run.trials[m_currentImageIndex].participantResponse = response;
		m_run.trials[m_currentImageIndex].duration = m_stopwatch->Elapsed().count();

		if (response == m_run.trials[m_currentImageIndex].correctOption)
		{
			m_successSound->Play();
		}
		else
		{
			m_failureSound->Play();
		}

		if (m_currentImageIndex + 1 >= m_run.trials.size())
		{
			m_run.Export("result" + m_run.id + ".csv");

			ExitGame();
			exit(0);
			return false;
		}

		return true;
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
			m_stopwatch->Restart();
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

		const auto response = (left == Button::PRESSED) ? Right : Left;

		m_run.trials[m_currentImageIndex].participantResponse = response;
		m_run.trials[m_currentImageIndex].duration = m_stopwatch->Elapsed().count();

		
		if (response == m_run.trials[m_currentImageIndex].correctOption)
		{
			m_successSound->Play();
		}
		else
		{
			m_failureSound->Play();
		}
		
		if (m_currentImageIndex + 1 >= m_run.trials.size())
		{
			m_run.Export("result" + m_run.id + ".csv");

			ExitGame();
			exit(0);
		}

		return true;
	}

	ComPtr<ID3D11ShaderResourceView> Controller::ConvertImageToResource(const std::filesystem::path& image, Vector* region) const
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
			: CropMatrix(matrix, cv::Rect(region->x, region->y, m_run.dimensions.x, m_run.dimensions.y));

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


	std::pair<DuoView, DuoView> Controller::SetFlickerStereoViews(const Trial& trial) const
	{
		const auto start = m_run.folderPath + "/" + trial.imageName;
		Utils::Duo<std::string> sidePrefixes = {};

		switch (trial.mode)
		{
		case Mono_Left:
			sidePrefixes = { "_L", "_L" };
			break;

		case Mono_Right:
			sidePrefixes = { "_R", "_R" };
			break;

		case Stereo:
			sidePrefixes = { "_L", "_R" };
		}

		auto files = std::vector<std::string>{
			start + sidePrefixes.left  + "_dec.ppm" ,
			start + sidePrefixes.left  + "_orig.ppm",
			start + sidePrefixes.right + "_dec.ppm" ,
			start + sidePrefixes.right + "_orig.ppm",
		};

		std::vector<ComPtr<ID3D11ShaderResourceView>> views;
		const auto dims = m_deviceResources->GetDimensions();

		views.reserve(files.size());
		for (auto& file : files)
		{
			auto region = trial.position;
			views.push_back(ConvertImageToResource(file, &region));
		}

		auto left = DirectX::SimpleMath::Vector2(
			dims.x / 2 - m_run.distance / 2 - m_run.dimensions.x,
			dims.y / 2 - m_run.dimensions.y / 2
		);

		auto right = DirectX::SimpleMath::Vector2(
			dims.x / 2 + m_run.distance / 2,
			dims.y / 2 - m_run.dimensions.y / 2
		);

		DuoView no_flicker = {};
		no_flicker.views.left = { views[1], views[1] };
		no_flicker.views.right = { views[3], views[3] };

		no_flicker.positions = { left, right };

		DuoView flicker = {};
		flicker.positions = { left, right };

		if (trial.correctOption == Experiment::Left)
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