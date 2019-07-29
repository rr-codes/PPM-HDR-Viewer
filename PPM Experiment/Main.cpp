//
// Main.cpp
//

#include "pch.h"
#include "Game.h"
#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>

HWND* windows;

using string_ref = const std::string &;

namespace
{
	std::unique_ptr<Game> g_game;
};

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

// Indicates to hybrid graphics systems to prefer the discrete part by default
extern "C"
{
	__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

bool argumentsAreValid(std::vector<std::string> arguments)
{
	const std::filesystem::path folder_path(arguments[arguments.size() - 2]);
	const std::filesystem::path config_path(arguments[arguments.size() - 1]);

	return is_directory(folder_path) && is_regular_file(config_path);
}

// Entry point
int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	if (!DirectX::XMVerifyCPUSupport())
		return 1;

	HRESULT hr = CoInitializeEx(nullptr, COINITBASE_MULTITHREADED);
	if (FAILED(hr))
		return 1;

	MSG msg = {};

	auto arguments = string::split(string::to_string(lpCmdLine));
	auto shouldFlicker = vector::contains<std::string>(arguments, "-flicker");

	int w, h;
	g_game->GetDefaultSize(w, h);

	if (!argumentsAreValid(arguments))
	{
		std::string str = "Error parsing arguments.";

		const auto result = MessageBox(
			nullptr,
			std::wstring(str.begin(), str.end()).c_str(),
			L"Error",
			0
		);

		if (result == 1) exit(1);
	}

	g_game = std::make_unique<Game>(
		arguments[arguments.size() - 2],
		arguments[arguments.size() - 1],
		shouldFlicker
		);

	RECT rc;

	windows = new HWND[NUMBER_OF_WINDOWS];


	for (int i = 0; i < NUMBER_OF_WINDOWS; i++) {

		// Register class and create window
		{
			LPCWSTR name = (i == 0) ? L"LEFT" : L"RIGHT";

			rc = { 0, 0, static_cast<LONG>(w), static_cast<LONG>(h) };

			// Register class
			WNDCLASSEXW wcex = {};
			wcex.cbSize = sizeof(WNDCLASSEXW);
			wcex.style = CS_HREDRAW | CS_VREDRAW;
			wcex.lpfnWndProc = WndProc;
			wcex.hInstance = hInstance;
			wcex.hIcon = LoadIconW(hInstance, L"IDI_ICON");
			wcex.hCursor = LoadCursorW(nullptr, IDC_ARROW);
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

			windows[i] = CreateWindowExW(0, name, name, WS_OVERLAPPED,
				CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInstance,
				nullptr);
			// TODO: Change to CreateWindowExW(WS_EX_TOPMOST, L"$safeprojectname$WindowClass", L"$projectname$", WS_POPUP,
			// to default to fullscreen.

			if (!windows[i])
				return 1;

			ShowWindow(windows[i], nCmdShow);
			// TODO: Change nCmdShow to SW_SHOWMAXIMIZED to default to fullscreen.

			SetWindowLongPtr(windows[i], GWLP_USERDATA, reinterpret_cast<LONG_PTR>(g_game.get()));

			GetClientRect(windows[i], &rc);
		}
	}


	g_game->Initialize(windows, rc.right - rc.left, rc.bottom - rc.top);

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

	auto wndIndex = 0;
	for (int i = 0; i < NUMBER_OF_WINDOWS; i++)
	{
		if (windows[i] == hWnd) wndIndex = i;
	}

	auto game = reinterpret_cast<Game*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

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
			game->OnWindowMoved(wndIndex);
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
			game->OnWindowSizeChanged(wndIndex, LOWORD(lParam), HIWORD(lParam));
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

			game->OnWindowSizeChanged(wndIndex, rc.right - rc.left, rc.bottom - rc.top);
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

	case WM_SYSKEYDOWN:
		//if (wParam == VK_RETURN && (lParam & 0x60000000) == 0x20000000)
		//{
		//	// Implements the classic ALT+ENTER fullscreen toggle
		//	if (s_fullscreen)
		//	{
		//		SetWindowLongPtr(hWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);
		//		SetWindowLongPtr(hWnd, GWL_EXSTYLE, 0);

		//		int width = 800;
		//		int height = 600;
		//		if (game)
		//			game->GetDefaultSize(width, height);

		//		ShowWindow(hWnd, SW_SHOWNORMAL);

		//		SetWindowPos(hWnd, HWND_TOP, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED);
		//	}
		//	else
		//	{
		//		SetWindowLongPtr(hWnd, GWL_STYLE, 0);
		//		SetWindowLongPtr(hWnd, GWL_EXSTYLE, WS_EX_TOPMOST);

		//		SetWindowPos(hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);

		//		ShowWindow(hWnd, SW_SHOWMAXIMIZED);
		//	}

		//	s_fullscreen = !s_fullscreen;
		//}
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

