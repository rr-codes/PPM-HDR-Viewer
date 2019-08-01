//
// Game.h
//

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"
#include "RenderTexture.h"
#include "SpriteBatch.h"
#include "GamePad.h"
#include "Participant.h"
#include "SimpleMath.h"

using string_ref = const std::string &;

// A basic game implementation that creates a D3D11 device and
// provides a game loop.
class Game final : public DX::IDeviceNotify
{
public:
	Game(const Experiment::Trial& trial) noexcept(false);

	// Initialization and management
	void Initialize(HWND windows[], int width, int height);
	// Basic game loop
	void Tick();
	void OnArrowKeyDown(WPARAM key);
	void OnEscapeKeyDown();
	void OnGamePadButton(DirectX::GamePad::State state);

	// IDeviceNotify
	virtual void OnDeviceLost() override;
	virtual void OnDeviceRestored() override;
	void getImagesAsTextures(Microsoft::WRL::ComPtr<ID3D11Texture2D>* textures);
	static matrix<std::filesystem::path> getFiles(string_ref folder, const std::vector<Experiment::Question>& questions);

	// Messages
	void OnActivated();
	void OnDeactivated();
	void OnSuspending();
	void OnResuming();
	void OnWindowMoved(int i);
	void OnWindowSizeChanged(int i, int width, int height);

	// Properties
	void GetDefaultSize(int& width, int& height) const;

	void Prerender();
	Utils::Duo<int> GetIndicesForCurrentFrame(int windowIndex);
	void Render(int i);
	void Clear(int i);


private:

	void Update(DX::StepTimer const& timer);

	void CreateDeviceDependentResources();
	void CreateWindowSizeDependentResources();

	// Device resources.
	std::unique_ptr<DX::DeviceResources> m_deviceResources;

	// Rendering loop timer.
	DX::StepTimer m_timer;
	DX::StepTimer m_frameTimer;

	std::unique_ptr<DirectX::SpriteBatch> m_spriteBatch;

	std::unique_ptr<DX::RenderTexture>* m_hdrScene;
	std::unique_ptr<DirectX::ToneMapPostProcess>* m_toneMap;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>* m_shaderResourceViews;
	Microsoft::WRL::ComPtr<ID3D11Texture2D>* m_textures;

	bool* m_flickerFrameFlag;
	int m_imageSetIndex = 0;

	matrix<std::filesystem::path> m_files;

	std::unique_ptr<DirectX::GamePad>  m_gamePad;
	DirectX::GamePad::ButtonStateTracker m_buttons;

	Experiment::Trial m_trial;

	std::chrono::time_point<std::chrono::system_clock> last_time;

	Utils::Duo<DirectX::SimpleMath::Vector2> m_imagePositions;
};