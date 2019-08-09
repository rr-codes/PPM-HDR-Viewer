//
// Game.cpp
//

#include "pch.h"
#include "Game.h"
#include <synchapi.h>
#include <filesystem>
#include <iostream>
#include <fstream>

extern void ExitGame();

using Microsoft::WRL::ComPtr;

/**
 * Sets the rate at which to alternate between the images (i.e, there are `rate` seconds between the original and decompressed)
 * For stereo, every `rate` seconds, the two windows' frames are rendered, one before the other, and then they are presented, one before the other
 */
static float rate = 0.1f;


Game::Game(const std::wstring& folderPath, bool flicker, bool stereo) noexcept(false)
{
	m_numberOfWindows = stereo ? 2 : 1;
	m_flickerEnable = flicker;
	m_files = getFiles(folderPath);

	m_deviceResources = std::make_unique<DX::DeviceResources>(
		m_numberOfWindows, 
		DXGI_FORMAT_R10G10B10A2_UNORM,
		DXGI_FORMAT_D32_FLOAT, 
		2, 
		D3D_FEATURE_LEVEL_10_0, 
		DX::DeviceResources::c_EnableHDR
	);

	m_deviceResources->RegisterDeviceNotify(this);

	m_hdrScene = new std::unique_ptr<DX::RenderTexture>[m_numberOfWindows];
	m_toneMap = new std::unique_ptr<DirectX::ToneMapPostProcess>[m_numberOfWindows];

	m_textures = new ComPtr<ID3D11Texture2D>[2 * m_numberOfWindows];
	m_shaderResourceViews = new ComPtr<ID3D11ShaderResourceView>[2 * m_numberOfWindows];

	for (int i = 0; i < m_numberOfWindows; i++) {
		m_hdrScene[i] = std::make_unique<DX::RenderTexture>(DXGI_FORMAT_R16G16B16A16_FLOAT);
	}

} 

// Initialize the Direct3D resources required to run.
void Game::Initialize(HWND windows[], int width, int height)
{
	m_flickerFrameFlag = new bool[m_numberOfWindows];
	m_textures = new ComPtr<ID3D11Texture2D>[m_numberOfWindows];

	for (int i = 0; i < m_numberOfWindows; i++)
	{
		m_deviceResources->SetWindow(i, windows[i], width, height);
	}

	m_deviceResources->CreateDeviceResources();
	CreateDeviceDependentResources();

	m_deviceResources->CreateWindowSizeDependentResources();

	CreateWindowSizeDependentResources();
	
	m_timer.SetFixedTimeStep(true);
	m_timer.SetTargetElapsedSeconds(rate);

	for (int i = 0; i < m_numberOfWindows; i++)
	{
		m_deviceResources->GoFullscreen(i);
	}
}

#pragma region Frame Update
// Executes the basic game loop.
void Game::Tick()
{
	m_timer.Tick([&]() { Update(m_timer); });
}

void Game::OnArrowKeyDown(WPARAM key)
{
	if (key == VK_LEFT)
	{
		m_imageSetIndex = (m_imageSetIndex == 0) ? m_files.size() - 1 : m_imageSetIndex - 1;
	}
	else if (key == VK_RIGHT)
	{
		m_imageSetIndex = (m_imageSetIndex + 1) % m_files.size();
	}

	getImagesAsTextures(m_textures);
	Prerender();
}


void Game::OnEscapeKeyDown()
{
	m_deviceResources.reset();
	OnDeviceLost();

	exit(0);
}


// Updates the world.
void Game::Update(DX::StepTimer const& timer)
{
	for (int i = 0; i < m_numberOfWindows; i++)
	{
		Render(i);
	}

	m_deviceResources->ThreadPresent();

	for (int i = 0; i < m_numberOfWindows; i++)
	{
		m_deviceResources->DiscardView(i);
	}
}
#pragma endregion


void Game::Prerender()
{
	for (int i = 0; i < m_numberOfWindows * 2; i++)
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC desc2 = { };
		desc2.Format = DXGI_FORMAT_R16G16B16A16_UNORM;
		desc2.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		desc2.Texture2D.MipLevels = 1;

		auto hr = m_deviceResources->m_d3dDevice->CreateShaderResourceView(
			m_textures[i].Get(),
			&desc2,
			m_shaderResourceViews[i].GetAddressOf()
		);
	}
}


#pragma region Frame Render
// Draws the scene.
void Game::Render(int i)
{
	// Don't try to render anything before the first Update.
	if (m_timer.GetFrameCount() == 0)
	{
		return;
	}

	Clear(i);

	m_deviceResources->PIXBeginEvent(L"Render");
	auto context = m_deviceResources->GetD3DDeviceContext();

	// Determines which image out of the possible four to present this frame. If NUM_WINDOWS is 2, i will be 1 50% of the time, else 0%
	int index = (m_flickerFrameFlag[i] || !m_flickerEnable) ? 0 : 1;
	if (i == 1) index += 2;

	m_spriteBatch->Begin();

	m_spriteBatch->Draw(m_shaderResourceViews[index].Get(), DirectX::XMFLOAT2(0, 0));

	m_spriteBatch->End();


	m_deviceResources->PIXEndEvent();

	auto renderTarget = m_deviceResources->GetRenderTargetView(i);
	context->OMSetRenderTargets(1, &renderTarget, nullptr);

	m_toneMap[i]->Process(context);

	ID3D11ShaderResourceView* nullsrv[] = { nullptr };
	context->PSSetShaderResources(0, 1, nullsrv);

	// Show the new frame.
	// m_deviceResources->CleanFrame(i);

	m_flickerFrameFlag[i] = !m_flickerFrameFlag[i];
}

// Helper method to clear the back buffers.
void Game::Clear(int i)
{
	m_deviceResources->PIXBeginEvent(L"Clear");

	// Clear the views.
	auto context = m_deviceResources->GetD3DDeviceContext();

	auto renderTarget = m_hdrScene[i]->GetRenderTargetView();
	auto depthStencil = m_deviceResources->GetDepthStencilView();

	DirectX::XMVECTORF32 color;
	auto actual = DirectX::FXMVECTOR({ {0, 0, 0, 0} });
	color.v = DirectX::XMColorSRGBToRGB(actual);
	context->ClearRenderTargetView(renderTarget, color);


	context->ClearDepthStencilView(depthStencil, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	context->OMSetRenderTargets(1, &renderTarget, depthStencil);

	// Set the viewport.
	auto viewport = m_deviceResources->GetScreenViewport();
	context->RSSetViewports(1, &viewport);

	m_deviceResources->PIXEndEvent();
}
#pragma endregion

#pragma region Message Handlers
// Message handlers
void Game::OnActivated()
{
	// TODO: Game is becoming active window.
}

void Game::OnDeactivated()
{
	// TODO: Game is becoming background window.
}

void Game::OnSuspending()
{
	// TODO: Game is being power-suspended (or minimized).
}

void Game::OnResuming()
{
	m_timer.ResetElapsedTime();
}
 
void Game::OnWindowMoved(int i)
{
	auto r = m_deviceResources->GetOutputSize();
	m_deviceResources->WindowSizeChanged(i, r.right, r.bottom);
}

void Game::OnWindowSizeChanged(int i, int width, int height)
{
	if (!m_deviceResources->WindowSizeChanged(i, width, height))
		return;

	CreateWindowSizeDependentResources();
}

// Properties
void Game::GetDefaultSize(int& width, int& height) const
{
	width = 800;
	height = 600;
}
#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Game::CreateDeviceDependentResources()
{
	auto device = m_deviceResources->GetD3DDevice();

	try {
		cv::directx::ocl::initializeContextFromD3D11Device(device);
	} catch (cv::Exception& e)
	{
		auto msg = e.msg;
		throw e;
	}

	m_spriteBatch = std::make_unique<DirectX::SpriteBatch>(m_deviceResources->GetD3DDeviceContext());

	for (int i = 0; i < m_numberOfWindows; i++) {
		m_hdrScene[i]->SetDevice(device);
		m_toneMap[i] = std::make_unique<DirectX::ToneMapPostProcess>(device);

		m_toneMap[i]->SetST2084Parameter(64);
	}

	getImagesAsTextures(m_textures);
	Prerender();
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateWindowSizeDependentResources()
{
	auto size = m_deviceResources->GetOutputSize();
	for (int i = 0; i < m_numberOfWindows; i++) {
		m_hdrScene[i]->SetWindow(size);

		m_toneMap[i]->SetHDRSourceTexture(m_hdrScene[i]->GetShaderResourceView());
	}
}

void Game::OnDeviceLost()
{
	for (int i = 0; i < m_numberOfWindows; i++) {
		m_hdrScene[i]->ReleaseDevice();

		m_toneMap[i].reset();
	}
}

void Game::OnDeviceRestored()
{
	CreateDeviceDependentResources();

	CreateWindowSizeDependentResources();
	CreateWindowSizeDependentResources();

}
#pragma endregion


void Game::getImagesAsTextures(ComPtr<ID3D11Texture2D>* textures)
{
	auto filenames = m_files[m_imageSetIndex];

	for (int i = 0; i < m_numberOfWindows * 2; i++)
	{
		auto matrixoriginal = cv::imread(filenames[i], cv::IMREAD_ANYCOLOR | cv::IMREAD_ANYDEPTH);
		cv::Mat matrix(matrixoriginal.size(), CV_MAKE_TYPE(matrixoriginal.depth(), 4));

		int conversion[] = { 2, 0, 1, 1, 0, 2, -1, 3 };
		cv::mixChannels(&matrixoriginal, 1, &matrix, 1, conversion, 4);

		// crop

		D3D11_TEXTURE2D_DESC desc = {};
		desc.Width = matrix.cols;
		desc.Height = matrix.rows;
		desc.MipLevels = desc.ArraySize = 1;
		desc.Format = DXGI_FORMAT_R16G16B16A16_UNORM;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ;
		desc.MiscFlags = 0;

		auto hr = m_deviceResources->GetD3DDevice()->CreateTexture2D(&desc, nullptr, textures[i].GetAddressOf());

		try {
			cv::directx::convertToD3D11Texture2D(matrix, textures[i].Get());
		}
		catch (cv::Exception& e)
		{
			auto msg = e.err;
			throw cv::Exception();
		}

		if (textures[i] == nullptr)
		{
			throw std::exception("cannot read image");
		}
	}

}

matrix<std::string> Game::getFiles(const std::wstring& folder)
{
	matrix<std::string> folder_vector;
	std::vector<std::string> files;

	auto path = std::filesystem::path(folder);

	if (path.extension() == ".ppm")
	{
		folder_vector.push_back(std::vector<std::string> {
			path.generic_string(), 
			path.generic_string()
		});

		return folder_vector;
	}
	
	for (const auto& file : std::filesystem::directory_iterator(folder))
	{
		files.push_back(file.path().generic_string());
	}

	auto delta = m_numberOfWindows * (1 + (m_flickerEnable ? 1 : 0));
	for (auto i = 0; i < files.size() - delta + 1; i += delta)
	{
		std::vector<std::string> permutations(4);

		if (m_numberOfWindows == 1 && !m_flickerEnable)
		{
			permutations = { files[i], files[i] };
		}
		else if (m_numberOfWindows == 1 && m_flickerEnable)
		{
			permutations = { files[i], files[i + 1] };
		}
		else if (!m_flickerEnable)
		{
			permutations = { files[i + 1], files[i + 1], files[i], files[i] };
		}
		else if (m_flickerEnable)
		{
			permutations = { files[i], files[i + 1], files[i + 2], files[i + 3]};
		}

		folder_vector.push_back(permutations);
	}

	return folder_vector;
}
