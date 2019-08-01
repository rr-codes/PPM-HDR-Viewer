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

Game::Game(const Experiment::Trial& trial) noexcept(false)
{
	m_trial = trial;
	m_files = getFiles(m_trial.folderPath, m_trial.questions);

	m_deviceResources = std::make_unique<DX::DeviceResources>(
		NUMBER_OF_WINDOWS,
		DXGI_FORMAT_R10G10B10A2_UNORM,
		DXGI_FORMAT_D32_FLOAT,
		2,
		D3D_FEATURE_LEVEL_10_0,
		DX::DeviceResources::c_EnableHDR
	);

	m_deviceResources->RegisterDeviceNotify(this);

	m_hdrScene = new std::unique_ptr<DX::RenderTexture>[NUMBER_OF_WINDOWS];
	m_toneMap = new std::unique_ptr<DirectX::ToneMapPostProcess>[NUMBER_OF_WINDOWS];

	m_textures = new ComPtr<ID3D11Texture2D>[2 * NUMBER_OF_WINDOWS];
	m_shaderResourceViews = new ComPtr<ID3D11ShaderResourceView>[2 * NUMBER_OF_WINDOWS];

	for (int i = 0; i < NUMBER_OF_WINDOWS; i++) {
		m_hdrScene[i] = std::make_unique<DX::RenderTexture>(DXGI_FORMAT_R16G16B16A16_FLOAT);
	}

}

// Initialize the Direct3D resources required to run.
void Game::Initialize(HWND windows[], int width, int height)
{
	m_flickerFrameFlag = new bool[NUMBER_OF_WINDOWS];
	m_textures = new ComPtr<ID3D11Texture2D>[NUMBER_OF_WINDOWS];

	for (int i = 0; i < NUMBER_OF_WINDOWS; i++)
	{
		m_deviceResources->SetWindow(i, windows[i], width, height);
	}

	m_deviceResources->CreateDeviceResources();
	CreateDeviceDependentResources();

	m_deviceResources->CreateWindowSizeDependentResources();

	CreateWindowSizeDependentResources();

	m_gamePad = std::make_unique<DirectX::GamePad>();

	for (int i = 0; i < NUMBER_OF_WINDOWS; i++)
	{
		m_deviceResources->GoFullscreen(i);
	}

	getImagesAsTextures(m_textures);
	Prerender();

	m_timer.SetFixedTimeStep(true);
	m_timer.SetTargetElapsedSeconds(m_trial.flicker_rate);

	m_frameTimer.SetFixedTimeStep(true);
	m_frameTimer.SetTargetElapsedSeconds(1.0f / 60.0f);
}

#pragma region Frame Update
// Executes the basic game loop.
void Game::Tick()
{
	m_frameTimer.Tick([&]() {
		auto state = m_gamePad->GetState(0);
		if (state.IsConnected())
		{
			OnGamePadButton(state);
		}
	});

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
	for (int i = 0; i < NUMBER_OF_WINDOWS; i++)
	{
		m_deviceResources->GetSwapChain(i)->SetFullscreenState(false, nullptr);
	}

	exit(0);
}

void Game::OnGamePadButton(const DirectX::GamePad::State state)
{
	using Button = DirectX::GamePad::ButtonStateTracker;

	const auto right = m_buttons.rightTrigger;
	const auto left = m_buttons.leftTrigger;

	m_buttons.Update(state);

	if (right != Button::PRESSED && left != Button::PRESSED)
	{
		return;
	}

	const std::chrono::duration<double> elapsed_seconds = std::chrono::system_clock::now() - last_time;

	m_trial.responses.push_back(Experiment::Response {
		(right == Button::PRESSED) 
			? Experiment::Option::Right 
			: Experiment::Option::Left,
		elapsed_seconds.count()
	});

	// go to next

	if (++m_imageSetIndex >= m_files.size())
	{
		m_trial.ExportResults(std::filesystem::path("C:/test.csv"));

		OnEscapeKeyDown();
		return;
	}

	getImagesAsTextures(m_textures);
	Prerender();
}


// Updates the world.
void Game::Update(DX::StepTimer const& timer)
{
	for (int i = 0; i < NUMBER_OF_WINDOWS; i++)
	{
		Render(i);
	}

	m_deviceResources->ThreadPresent();

	for (int i = 0; i < NUMBER_OF_WINDOWS; i++)
	{
		m_deviceResources->CleanFrame(i);
	}
}
#pragma endregion


void Game::Prerender()
{
	for (int i = 0; i < NUMBER_OF_WINDOWS * 2; i++)
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

	last_time = std::chrono::system_clock::now();
}

/// Given the four permutations of each image (left & decompressed, left, right & decompressed, right), returns the corresponding indices for the left and right images on a given window
/// For the image that is flickering, the image alternates between the decompressed and original once every two frames
/// 
/// In total, there are 6 possible combinations given the 4 permutations (assuming the left image is the flickering one):
/// 
/// <code>
/// 1) Dec.	| L Image | L Window
/// 2) Dec.	| L Image | R Window
/// 5) Org.	| L Image | L Window
/// 4) Org.	| L Image | R Window
/// 5) Org.	| R Image | L Window
/// 6) Org.	| R Image | R Windows
/// </code>
Utils::Duo<int> Game::GetIndicesForCurrentFrame(int windowIndex)
{
	// first calculate the flickering image index for the window
	int flickerImageIndex = m_flickerFrameFlag[windowIndex] ? 0 : 1; // should it show the default state or flickered state?
	if (windowIndex == 1) flickerImageIndex += 2; // 1 and 2 are for index 0 (left window), 3 and 4 are for index 1 (right window)

	// now get the static index
	const auto staticImageIndex = (windowIndex == 0) ? 1 : 3; // this is easy, its either the left or the right original depending on window

	// we need to know which image to actually flicker
	const auto correct = m_trial.questions[m_imageSetIndex].correct_option;

	m_flickerFrameFlag[windowIndex] = !m_flickerFrameFlag[windowIndex]; // toggle flicker for next frame

	// set the left image to the flickering index and right image static if the correct (flickering) one is left; else vice versa
	return (correct == Experiment::Option::Left)
		? Utils::Duo<int>{ flickerImageIndex, staticImageIndex }
		: Utils::Duo<int>{ staticImageIndex, flickerImageIndex };
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

	auto indices = GetIndicesForCurrentFrame(i);

	m_spriteBatch->Begin();

	m_spriteBatch->Draw(m_shaderResourceViews[indices.left].Get(), m_imagePositions.left);		// left
	m_spriteBatch->Draw(m_shaderResourceViews[indices.right].Get(), m_imagePositions.right);	// right

	m_spriteBatch->End();

	m_deviceResources->PIXEndEvent();

	auto renderTarget = m_deviceResources->GetRenderTargetView(i);
	context->OMSetRenderTargets(1, &renderTarget, nullptr);

	m_toneMap[i]->Process(context);

	ID3D11ShaderResourceView* nullsrv[] = { nullptr };
	context->PSSetShaderResources(0, 1, nullsrv);
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
	m_gamePad->Resume();
	m_buttons.Reset();
}

void Game::OnDeactivated()
{
	// TODO: Game is becoming background window.
	m_gamePad->Suspend();
}

void Game::OnSuspending()
{
	// TODO: Game is being power-suspended (or minimized).
	m_gamePad->Suspend();
}

void Game::OnResuming()
{
	m_timer.ResetElapsedTime();
	m_gamePad->Resume();
	m_buttons.Reset();
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
	}
	catch (cv::Exception& e)
	{
		auto msg = e.msg;
		throw e;
	}

	m_spriteBatch = std::make_unique<DirectX::SpriteBatch>(m_deviceResources->GetD3DDeviceContext());

	for (int i = 0; i < NUMBER_OF_WINDOWS; i++) {
		m_hdrScene[i]->SetDevice(device);
		m_toneMap[i] = std::make_unique<DirectX::ToneMapPostProcess>(device);

		m_toneMap[i]->SetST2084Parameter(64);
	}

	
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateWindowSizeDependentResources()
{
	auto size = m_deviceResources->GetOutputSize();
	for (int i = 0; i < NUMBER_OF_WINDOWS; i++) {
		m_hdrScene[i]->SetWindow(size);

		m_toneMap[i]->SetHDRSourceTexture(m_hdrScene[i]->GetShaderResourceView());
	}
}

void Game::OnDeviceLost()
{
	for (int i = 0; i < NUMBER_OF_WINDOWS; i++) {
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

static cv::Mat CropMatrix(const cv::Mat& mat, const cv::Rect& cropRegion)
{
	const cv::Mat croppedRef(mat, cropRegion);

	cv::Mat cropped;
	croppedRef.copyTo(cropped);

	return cropped;
}

/// The secret sauce. Converts a set of permutations of a single image into an array of <code>Microsoft::WRL::ComPtr<ID3D11Texture2D></code> textures
/// 
/// 1) The image is first read via <code>cv::imread()</code>
/// 2) The BGRA matrix is converted to RGB
/// 3) The matrix is cropped according to the region defined in the configuration file
/// 4) The origin (top left corner) of the left and right images are set accordingly
/// 5) A texture is initialized, and the matrix is converted to it
void Game::getImagesAsTextures(ComPtr<ID3D11Texture2D>* textures)
{
	auto filenames = m_files[m_imageSetIndex];

	for (int i = 0; i < NUMBER_OF_WINDOWS * 2; i++)
	{
		auto matrixoriginal = cv::imread(filenames[i].generic_string(), cv::IMREAD_ANYCOLOR | cv::IMREAD_ANYDEPTH);
		cv::Mat matrix(matrixoriginal.size(), CV_MAKE_TYPE(matrixoriginal.depth(), 4));

		// red is blue and blue is red and alpha is none
		int conversion[] = { 2, 0, 1, 1, 0, 2, -1, 3 };
		cv::mixChannels(&matrixoriginal, 1, &matrix, 1, conversion, 4);

		const auto r = m_trial.questions[m_imageSetIndex].region;
		auto cropped = CropMatrix(matrix, cv::Rect(r.x, r.y, r.w, r.h));

		const auto dims = m_deviceResources->GetDimensions();

		auto left = DirectX::SimpleMath::Vector2(
			dims.x / 2 - m_trial.distance / 2 - cropped.cols,
			dims.y / 2 - cropped.rows / 2
		);

		auto right = DirectX::SimpleMath::Vector2(
			dims.x / 2 + m_trial.distance / 2,
			dims.y / 2 - cropped.rows / 2
		);

		Debug::log("\n LeftW: %f, RightW: %f\n", left.x, right.x);

		m_imagePositions = { left, right };

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

		auto hr = m_deviceResources->GetD3DDevice()->CreateTexture2D(&desc, nullptr, textures[i].GetAddressOf());

		try {
			cv::directx::convertToD3D11Texture2D(cropped, textures[i].Get());
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

matrix<std::filesystem::path> Game::getFiles(string_ref folder, const std::vector<Experiment::Question>& questions)
{
	matrix<std::filesystem::path> folder_vector;

	for (auto& q : questions)
	{
		auto path = folder + "/" + q.image_name;

		std::vector<std::filesystem::path> paths;

		paths.emplace_back(path + "_L_dec.ppm");
		paths.emplace_back(path + "_L.ppm");
		paths.emplace_back(path + "_R_dec.ppm");
		paths.emplace_back(path + "_R.ppm");

		folder_vector.push_back(paths);
	}

	return folder_vector;
}
