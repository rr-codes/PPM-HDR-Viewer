#include "Controller.h"
#include <utility>
#include <ctime>

extern void ExitGame();

namespace Experiment {
	static const std::string DESTINATION_PATH = R"(C:\projects\VESA_3D_HDR_Testing_October_2019\Data\)";
	constexpr int FRAME_INTERVAL = 20;

	Controller::Controller(Run& run, DX::DeviceResources* deviceResources) : m_deviceResources(deviceResources), m_run(run)
	{
		m_audioEngine = std::make_unique<DirectX::AudioEngine>(DirectX::AudioEngine_Default);
		
		const auto dir = std::filesystem::cwd().generic_wstring() + L"/sounds/" + FAILURE;
		m_failureSound = std::make_unique<DirectX::SoundEffect>(m_audioEngine.get(), dir.c_str());

		this->m_fpstimer = std::make_unique<Utils::Timer<>>(FRAME_INTERVAL);
		this->m_flickerTimer = std::make_unique<Utils::Timer<>>(Configuration::FlickerRate * 1000.0);

		this->m_stopwatch = std::make_unique<Utils::Stopwatch<>>();
	}

	static cv::Mat CropMatrix(const cv::Mat& mat, const cv::Rect& cropRegion)
	{
		const cv::Mat croppedRef(mat, cropRegion);

		cv::Mat cropped;
		croppedRef.copyTo(cropped);

		return cropped;
	}

	bool Controller::GetResponse(const WPARAM key)
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

		const auto response = (key == VK_LEFT) ? Option::Right : Option::Left;
		AppendResponse(response);

		return true;
	}

	double timeAtPress = 0;

	bool Controller::GetResponse(const DirectX::GamePad::State state)
	{
		timeAtPress = m_stopwatch->Elapsed().count();
		
		const auto PRESSED = DirectX::GamePad::ButtonStateTracker::PRESSED;

		const auto right = m_buttons.rightTrigger;
		const auto left = m_buttons.leftTrigger;

		m_buttons.Update(state);

		if (m_buttons.a == PRESSED || m_buttons.b == PRESSED)
		{
			m_startButtonHasBeenPressed = true;
			m_stopwatch->Restart();
			return false;
		}

		if (right != PRESSED && left != PRESSED)
		{
			return false;
		}

		if (!m_startButtonHasBeenPressed)
		{
			return false;
		}

		const auto response = (left == PRESSED) ? Option::Right : Option::Left;
		AppendResponse(response);

		return true;
	}

	void Controller::AppendResponse(const Option response)
	{
		m_run.trials[m_currentImageIndex].participantResponse = response;
		m_run.trials[m_currentImageIndex].duration = timeAtPress;

		if (response != m_run.trials[m_currentImageIndex].correctOption)
		{
			m_failureSound->Play();
		}

		if ((1 + m_currentImageIndex) % m_run.SessionsPerTrial() == 0)
		{
			const auto s = Utils::FormatTime("%H-%M", std::chrono::system_clock::now());
			const auto filename = "Group" + std::to_string(m_run.participant.groupNumber)
				+ "_Session" + std::to_string(m_currentSession + m_run.minMax.first);
				+ "_Id" + m_run.participant.id + "_" + s + ".csv";

			m_run.Export(DESTINATION_PATH + filename, m_currentSession); 

			m_currentSession++;
			m_startButtonHasBeenPressed = false;
		}

		if (1 + m_currentImageIndex >= m_run.trials.size()) {
			ExitGame();
			exit(0);
		}
	}

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> Controller::ToResource(const std::filesystem::path& image) const
	{
		return ToResourceBase(image, [](auto m)
		{
			return m;
		});
	}

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> Controller::ToResource(const std::filesystem::path& image, Vector region) const
	{
		return ToResourceBase(image, [&](auto m)
		{
			return CropMatrix(m, cv::Rect(region.x, region.y, Configuration::ImageDimensions.x, Configuration::ImageDimensions.y));
		});
	}


	template<typename F>
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> Controller::ToResourceBase(const std::filesystem::path& image, F&& matTransformFunction) const
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

		auto cropped = matTransformFunction(matrix);

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
		const auto l = ToResource(views.left);
		const auto r = ToResource(views.right);

		const auto dims = m_deviceResources->GetDimensions();

		return {
			Image{ l, {0, 0} },
			Image{ r, {3840, 0} }
		};
	}

	std::pair<DuoView, DuoView> Controller::SetFlickerStereoViews(const Trial& trial) const
	{
		const auto modifiedImageStart = trial.directory + "/" + trial.imageName;
		const auto originalImageStart = m_run.originalImageDirectory + "/" + trial.imageName;

		Utils::Duo<std::string> sidePrefixes = {};

		switch (trial.mode)
		{
		case Mode::Mono_Left:
			sidePrefixes = { "_L", "_L" };
			break;

		case Mode::Mono_Right:
			sidePrefixes = { "_R", "_R" };
			break;

		case Mode::Stereo:
			sidePrefixes = { "_L", "_R" };
		}

		auto files = std::vector<std::string>{
			modifiedImageStart + sidePrefixes.left  + "_dec.ppm" ,
			originalImageStart + sidePrefixes.left  + "_orig.ppm",
			modifiedImageStart + sidePrefixes.right + "_dec.ppm" ,
			originalImageStart + sidePrefixes.right + "_orig.ppm",
		};

		std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> views;
		const auto dims = DirectX::SimpleMath::Vector2{3840 * 2, 2160};

		views.reserve(files.size());
		for (auto& file : files)
		{
			views.push_back(ToResource(file, trial.position));
		}

		auto halfDist = static_cast<float>(Configuration::ImageDistance) / 2;
		auto yPos = dims.y / 2 - static_cast<float>(Configuration::ImageDimensions.y) / 2;

		using Vec = DirectX::SimpleMath::Vector2;

		auto ll = Vec(dims.x / 4 - halfDist - Configuration::ImageDimensions.x, yPos);
		auto lr = Vec(dims.x / 4 + halfDist, yPos);
		auto rl = Vec(dims.x * 3 / 4 - halfDist - Configuration::ImageDimensions.x, yPos);
		auto rr = Vec(dims.x * 3 / 4 + halfDist, yPos);

		DuoView no_flicker = {
			{Image{views[1], ll}, Image{views[1], lr} },
			{Image{views[3], rl}, Image{views[3], rr} }
		};

		DuoView flicker = no_flicker;
		auto i = static_cast<int>(trial.correctOption) - 1;

		flicker.left[i].image = views[0];
		flicker.right[i].image = views[2];

		return std::make_pair(no_flicker, flicker);
	}

}