#include "Controller.h"
#include <utility>
#include <ctime>

extern void ExitGame();

namespace Experiment {

	Controller::Controller(Run& run, DX::DeviceResources* deviceResources) : m_deviceResources(deviceResources), m_run(run)
	{		
		this->m_flickerTimer = std::make_unique<Utils::Timer<>>(1000.0 / 60.0);
	}

	static cv::Mat CropMatrix(const cv::Mat& mat, const cv::Rect& cropRegion)
	{
		const cv::Mat croppedRef(mat, cropRegion);

		cv::Mat cropped;
		croppedRef.copyTo(cropped);

		return cropped;
	}

	double timeAtPress = 0;


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

	SingleView Controller::PathToSingleView(const std::filesystem::path& path) const
	{
		const auto img = ToResource(path);
		return {
			Image{img, {0, 0}},
			Image{img, {3840, 0}}
		};
	}

	DuoView Controller::SetFlickerStereoViews(const std::array<std::string, 4>& files) const
	{

		return DuoView{
			SetStaticStereoView({files[0], files[1]}),
			SetStaticStereoView({files[2], files[3]}),
		};
	}

}