#include "MyMain.h"
#include "../Headers/Base/CManager.h"
#include "../Headers/Base/CTimer.h"

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

INT APIENTRY WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ INT nCmdShow)
{
	const CHAR* CLASS_NAME = "EngineClass_D3D11_0";
	const CHAR* WINDOW_NAME	= "Engine_D3D11_0";

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

	HWND windowHandle = CreateWindowEx(0,
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

	CManager::Initialize(windowHandle);
	CManager* gameManager = CManager::GetManager();

	ShowWindow(windowHandle, nCmdShow);
	UpdateWindow(windowHandle);

	gameManager->Init();

	DOUBLE currentTime, fixedLastTime, updateLastTime, fixedStepTime, updateStepTime;
	fixedStepTime = static_cast <DOUBLE>(1) / static_cast<DOUBLE>(ENGINE_FIXED_UPDATE_FRAME);
	updateStepTime = static_cast <DOUBLE>(1) / static_cast<DOUBLE>(ENGINE_UPDATE_FRAME);
	currentTime = fixedLastTime = updateLastTime = gameManager->GetWindowTimer().GetClockTime();

	MSG msg;
	while (true)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
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
			CManager::StaticUpdate();
			currentTime = gameManager->GetWindowTimer().GetClockTime();
			if ((currentTime - fixedLastTime) >= fixedStepTime)
			{
				fixedLastTime = currentTime;
				gameManager->FixedUpdate();
			}
			if ((currentTime - updateLastTime) >= updateStepTime)
			{
				updateLastTime = currentTime;
				gameManager->Update();
				gameManager->Draw();
			}
		}
	}

	gameManager->Uninit();

	UnregisterClass(CLASS_NAME, wcex.hInstance);

	CManager::ShutDown();

	return (INT)msg.wParam;
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