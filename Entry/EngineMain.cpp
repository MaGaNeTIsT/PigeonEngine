#include "./EngineMain.h"
#include "../Engine/Sources/EngineBase/Headers/CManager.h"
#include "../Engine/Sources/EngineBase/Headers/CTimer.h"
#include "../Engine/EngineThirdParty/CimGUI/Headers/CimGUIManager.h"

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

INT APIENTRY WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ INT nCmdShow)
{
	const CHAR* CLASS_NAME = "EngineClass_D3D11";
	const CHAR* WINDOW_NAME = "Engine_D3D11";
	HWND windowHandle; WNDCLASSEX wcex = {};
	{
		wcex.cbSize = sizeof(WNDCLASSEX);
		wcex.style = CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc = WndProc;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = 0;
		wcex.hInstance = hInstance;
		wcex.hIcon = NULL;
		wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
		wcex.hbrBackground = (HBRUSH)(COLOR_BACKGROUND + 1);
		wcex.lpszMenuName = NULL;
		wcex.lpszClassName = CLASS_NAME;
		wcex.hIconSm = NULL;
		if (!RegisterClassEx(&wcex))
			return TRUE;
		RECT clientRect = { 0,0,ENGINE_SCREEN_WIDTH,ENGINE_SCREEN_HEIGHT };
		::AdjustWindowRect(&clientRect, WS_OVERLAPPEDWINDOW ^ (WS_MAXIMIZEBOX | WS_THICKFRAME), FALSE);
		windowHandle = CreateWindowEx(
			0,
			CLASS_NAME,
			WINDOW_NAME,
			(WS_OVERLAPPEDWINDOW ^ (WS_MAXIMIZEBOX | WS_THICKFRAME)),
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			clientRect.right - clientRect.left,
			clientRect.bottom - clientRect.top,
			NULL,
			NULL,
			hInstance,
			NULL);
	}

	CManager::Initialize(windowHandle);
	CManager* gameManager = CManager::GetManager();

	gameManager->Init();

	DOUBLE currentTime, fixedLastTime, updateLastTime, fixedStepTime, updateStepTime;
	fixedStepTime = static_cast <DOUBLE>(1) / static_cast<DOUBLE>(ENGINE_FIXED_UPDATE_FRAME);
	updateStepTime = static_cast <DOUBLE>(1) / static_cast<DOUBLE>(ENGINE_UPDATE_FRAME);
	currentTime = fixedLastTime = updateLastTime = gameManager->GetWindowTimer().GetClockTime();

	::ShowWindow(windowHandle, nCmdShow);
	::UpdateWindow(windowHandle);

	MSG msg;
	while (TRUE)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
			{
				break;
			}
			else
			{
				::TranslateMessage(&msg);
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
	if (CimGUIManager::WndProcHandler(hWnd, uMsg, wParam, lParam))
		return TRUE;

	switch(uMsg)
	{
	case WM_DESTROY:
		::PostQuitMessage(0);
		break;

	case WM_KEYDOWN:
		switch(wParam)
		{
		case VK_ESCAPE:
			::DestroyWindow(hWnd);
			break;
		}
		break;

	default:
		break;
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}