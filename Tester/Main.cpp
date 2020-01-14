//
// Main.cpp
//

#ifndef NOMINMAX
#define NOMINMAX
#endif


#include "pch.h"
#include "Game.h"
#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <shlobj.h>

#pragma comment(lib, "Comdlg32.lib")
#pragma comment(lib, "Comctl32.lib")


using string_ref = const std::string &;

std::unique_ptr<Experiment::Game> g_game;
HWND window;


LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

// Indicates to hybrid graphics systems to prefer the discrete part by default
extern "C"
{
	__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

static int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
	if (uMsg == BFFM_INITIALIZED)
	{
		LPCTSTR path = reinterpret_cast<LPCTSTR>(lpData);
		::SendMessage(hwnd, BFFM_SETSELECTION, true, (LPARAM)path);
	}

	return 0;
}

std::wstring BrowseFolder(std::wstring saved_path, std::wstring title = L"Browse for Folder")
{
	TCHAR path[MAX_PATH];

	const wchar_t* path_param = saved_path.c_str();

	BROWSEINFO bi = { 0 };
	bi.lpszTitle = title.c_str();
	bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
	bi.lpfn = BrowseCallbackProc;
	bi.lParam = (LPARAM)path_param;

	LPITEMIDLIST pidl = SHBrowseForFolder(&bi);

	if (pidl != 0)
	{
		//get the name of the folder and put it in path
		SHGetPathFromIDList(pidl, path);

		//free memory used
		IMalloc* imalloc = 0;
		if (SUCCEEDED(SHGetMalloc(&imalloc)))
		{
			imalloc->Free(pidl);
			imalloc->Release();
		}

		return path;
	}

	return L"";
}

// Entry point
int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	if (!DirectX::XMVerifyCPUSupport())
		return 1;

	HRESULT hr = CoInitializeEx(nullptr, COINITBASE_MULTITHREADED | COINIT_APARTMENTTHREADED);
	if (FAILED(hr))
		return 1;

	MSG msg = {};

	int w, h;
	g_game->GetDefaultSize(w, h);

	auto tryInitialDirectory = L"E:\\Mohona_PhD\\Qualcom\\Qualcomm_VESA_HDR 3D testing_2019\\VESA 3D HDR CA testing February 2020\\Images_CA\\";
	if (!std::filesystem::is_directory(tryInitialDirectory)) {
		tryInitialDirectory = L"C:\\";
	}


	const auto originalsFolder = BrowseFolder(tryInitialDirectory, L"Select Original Folder");
	const auto compressedFolder = BrowseFolder(originalsFolder, L"Browse for Compressed Folder");

	auto run = Experiment::Run::CreateRun(originalsFolder, compressedFolder);

	g_game = std::make_unique<Experiment::Game>(run);

	RECT rc;

	LPCWSTR name = L"RIGHT";

	rc = { 0, 0, static_cast<LONG>(w), static_cast<LONG>(h) };

	// Register class
	WNDCLASSEXW wcex = {};
	wcex.cbSize = sizeof(WNDCLASSEXW);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIconW(hInstance, L"IDI_ICON");
	wcex.hCursor = LoadCursorW(nullptr, IDC_CROSS);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszClassName = name;
	wcex.hIconSm = LoadIconW(wcex.hInstance, L"IDI_ICON");

	if (!RegisterClassExW(&wcex)) {
		auto err = GetLastError();
		auto hr = HRESULT_FROM_WIN32(err);

		_com_error err2(hr);
		auto str = err2.ErrorMessage();
		return 1;
	}

	// Create window


	AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

	window = CreateWindowExW(
		WS_EX_TOPMOST,
		name,
		name,
		WS_POPUP,
		CW_USEDEFAULT, CW_USEDEFAULT,
		(rc.right - rc.left) * 2,
		rc.bottom - rc.top,
		nullptr,
		nullptr,
		hInstance,
		nullptr
	);

	// TODO: Change to CreateWindowExW(WS_EX_TOPMOST, L"$safeprojectname$WindowClass", L"$projectname$", WS_POPUP,
	// to default to fullscreen.

	if (!window)
		return 1;

	ShowWindow(window, nCmdShow);
	// TODO: Change nCmdShow to SW_SHOWMAXIMIZED to default to fullscreen.

	SetWindowLongPtr(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(g_game.get()));

	GetClientRect(window, &rc);

	g_game->Initialize(window, rc.right - rc.left, rc.bottom - rc.top);

	// Main message loop
	while (WM_QUIT != msg.message)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			g_game->Tick();
		}
	}

	g_game.reset();

	CoUninitialize();

	return static_cast<int>(msg.wParam);
}

// Windows procedure
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;

	static bool s_in_sizemove = false;
	static bool s_in_suspend = false;
	static bool s_minimized = false;
	static bool s_fullscreen = false;
	// TODO: Set s_fullscreen to true if defaulting to fullscreen.

	auto game = reinterpret_cast<Experiment::Game*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

	switch (message)
	{
	case WM_PAINT:
		if (s_in_sizemove && game)
		{
			game->Tick();
		}
		else
		{
			HDC hdc = BeginPaint(hWnd, &ps);
			EndPaint(hWnd, &ps);
		}
		break;

	case WM_QUIT:
		if (game)
		{
			game->OnEscapeKeyDown();
		}
		break;

	case WM_KEYDOWN:
		if (game && (wParam == VK_LEFT || wParam == VK_RIGHT))
		{
			game->OnArrowKeyDown(wParam);
		}
		if (game && wParam == VK_ESCAPE)
		{
			game->OnEscapeKeyDown();
		}
		break;

	case WM_MOVE:
		if (game)
		{
			game->OnWindowMoved();
		}
		break;

	case WM_SIZE:
		if (wParam == SIZE_MINIMIZED)
		{
			if (!s_minimized)
			{
				s_minimized = true;
				if (!s_in_suspend && game)
					game->OnSuspending();
				s_in_suspend = true;
			}
		}
		else if (s_minimized)
		{
			s_minimized = false;
			if (s_in_suspend && game)
				game->OnResuming();
			s_in_suspend = false;
		}
		else if (!s_in_sizemove && game)
		{
			game->OnWindowSizeChanged(LOWORD(lParam), HIWORD(lParam));
		}
		break;

	case WM_ENTERSIZEMOVE:
		s_in_sizemove = true;
		break;

	case WM_EXITSIZEMOVE:
		s_in_sizemove = false;
		if (game)
		{
			RECT rc;
			GetClientRect(hWnd, &rc);

			game->OnWindowSizeChanged(rc.right - rc.left, rc.bottom - rc.top);
		}
		break;

	case WM_GETMINMAXINFO:
	{
		auto info = reinterpret_cast<MINMAXINFO*>(lParam);
		info->ptMinTrackSize.x = 320;
		info->ptMinTrackSize.y = 200;
	}
	break;

	case WM_ACTIVATEAPP:
		if (game)
		{
			if (wParam)
			{
				game->OnActivated();
			}
			else
			{
				game->OnDeactivated();
			}
		}
		break;

	case WM_POWERBROADCAST:
		switch (wParam)
		{
		case PBT_APMQUERYSUSPEND:
			if (!s_in_suspend && game)
				game->OnSuspending();
			s_in_suspend = true;
			return TRUE;

		case PBT_APMRESUMESUSPEND:
			if (!s_minimized)
			{
				if (s_in_suspend && game)
					game->OnResuming();
				s_in_suspend = false;
			}
			return TRUE;
		}
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	case WM_MENUCHAR:
		// A menu is active and the user presses a key that does not correspond
		// to any mnemonic or accelerator key. Ignore so we don't produce an error beep.
		return MAKELRESULT(0, MNC_CLOSE);
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}




// Exit helper
void ExitGame()
{
	PostQuitMessage(0);
}

