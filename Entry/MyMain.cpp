#include "MyMain.h"
#include "../Headers/Base/CManager.h"
#include "../Headers/Base/CTimer.h"


const char* CLASS_NAME = "DX11AppClass";
const char* WINDOW_NAME = "DX11";


LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);


HWND _Window;

HWND GetMainWindowHandle()
{
	return _Window;
}

CTimer _Timer;

CTimer* GetMainWindowTimer()
{
	return (&_Timer);
}


int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	WNDCLASSEX wcex =
	{
		sizeof(WNDCLASSEX),
		CS_CLASSDC,
		WndProc,
		0,
		0,
		hInstance,
		NULL,
		LoadCursor(NULL, IDC_ARROW),
		(HBRUSH)(COLOR_WINDOW + 1),
		NULL,
		CLASS_NAME,
		NULL
	};

	RegisterClassEx(&wcex);

	_Window = CreateWindowEx(0,
		CLASS_NAME,
		WINDOW_NAME,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		(ENGINE_SCREEN_WIDTH + GetSystemMetrics(SM_CXDLGFRAME) * 2),
		(ENGINE_SCREEN_HEIGHT + GetSystemMetrics(SM_CXDLGFRAME) * 2 + GetSystemMetrics(SM_CYCAPTION)),
		NULL,
		NULL,
		hInstance,
		NULL);

	CManager::Init();

	ShowWindow(_Window, nCmdShow);
	UpdateWindow(_Window);

	DOUBLE dCurrentTime, dExecLastTime;
	_Timer.Init();
	dCurrentTime = dExecLastTime = _Timer.GetClockTime();

	MSG msg;
	while(1)
	{
        if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if(msg.message == WM_QUIT)
			{
				break;
			}
			else
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
        }
		else
		{
			_Timer.Update();

			dCurrentTime = _Timer.GetClockTime();

			if ((dCurrentTime - dExecLastTime) >= (1.f / 60.f))
			{
				dExecLastTime = dCurrentTime;

				CManager::Update();

				CManager::Draw();
			}
		}
	}

	UnregisterClass(CLASS_NAME, wcex.hInstance);

	CManager::Uninit();

	return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	case WM_KEYDOWN:
		switch(wParam)
		{
		case VK_ESCAPE:
			DestroyWindow(hWnd);
			break;
		}
		break;

	default:
		break;
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}