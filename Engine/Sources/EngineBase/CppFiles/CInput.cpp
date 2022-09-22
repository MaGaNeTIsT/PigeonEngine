#include "../../../../Entry/EngineMain.h"
#include "../Headers/CInput.h"

BYTE CInput::m_OldKeyState[256];
BYTE CInput::m_KeyState[256];
MOUSEMOVEPOINT CInput::m_OldMouseMove;
MOUSEMOVEPOINT CInput::m_MouseMove;
CController CInput::Controller;

void CInput::Initialize(HWND hWnd)
{
	memset(m_OldKeyState, 0, 256);
	memset(m_KeyState, 0, 256);
	::ZeroMemory(&m_OldMouseMove, sizeof(MOUSEMOVEPOINT));
	::ZeroMemory(&m_MouseMove, sizeof(MOUSEMOVEPOINT));

	// Controller Initialization
	Controller.Initialize(hWnd, ENGINE_SCREEN_WIDTH, ENGINE_SCREEN_HEIGHT);

}
void CInput::ShutDown()
{
}
void CInput::Update()
{
	{
		memcpy(m_OldKeyState, m_KeyState, 256);
		memcpy(&m_OldMouseMove, &m_MouseMove, sizeof(MOUSEMOVEPOINT));
	}

	{
		BOOL keyResult = GetKeyboardState(m_KeyState);
	}

	{
		//GetCursorPos();
		//if (GetMouseMovePointsEx( NULL, &m_MouseMove, 1, GMMP_USE_DISPLAY_POINTS) != -1)
		//{
		//	INT nVirtualWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
		//	INT nVirtualHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);
		//	INT nVirtualLeft = GetSystemMetrics(SM_XVIRTUALSCREEN);
		//	INT nVirtualTop = GetSystemMetrics(SM_YVIRTUALSCREEN);
		//	INT cpt = 0;
		//	INT mode = GMMP_USE_DISPLAY_POINTS;

		//	MOUSEMOVEPOINT mp_in;
		//	MOUSEMOVEPOINT mp_out[64];

		//	::ZeroMemory(&mp_in, sizeof(MOUSEMOVEPOINT));
		//	//Ensure that this number will pass through.
		//	mp_in.x = pt.x & 0x0000FFFF;
		//	mp_in.y = pt.y & 0x0000FFFF;
		//	cpt = GetMouseMovePointsEx(sizeof(MOUSEMOVEPOINT), &mp_in, &mp_out, 64, mode);

		//	for (int i = 0; i < cpt; i++)
		//	{
		//		switch (mode)
		//		{
		//		case GMMP_USE_DISPLAY_POINTS:
		//			if (mp_out[i].x > 32767)
		//				mp_out[i].x -= 65536;
		//			if (mp_out[i].y > 32767)
		//				mp_out[i].y -= 65536;
		//			break;
		//		case GMMP_USE_HIGH_RESOLUTION_POINTS:
		//			mp_out[i].x = ((mp_out[i].x * (nVirtualWidth - 1)) - (nVirtualLeft * 65536)) / nVirtualWidth;
		//			mp_out[i].y = ((mp_out[i].y * (nVirtualHeight - 1)) - (nVirtualTop * 65536)) / nVirtualHeight;
		//			break;
		//		}
		//	}
		//}
	}
}
BOOL CInput::GetKeyPress(BYTE KeyCode)
{
	INT curr = (m_KeyState[KeyCode] & 0x80);
	return (curr > 0);
}
BOOL CInput::GetKeyTrigger(BYTE KeyCode)
{
	INT curr = (m_KeyState[KeyCode] & 0x80);
	INT prev = (m_OldKeyState[KeyCode] & 0x80);
	return ((curr > 0) && (prev == 0));
}

std::optional<CMouse::RawDelta> CInput::ReadRawDelta()
{
	return Controller.ReadRawDelta();
}

LRESULT CInput::HandleMsg(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	return Controller.HandleMsg(hWnd, msg, wParam, lParam);
}

void CController::Initialize(HWND InhWnd, INT InWindowSizeX, INT InWindowSizeY)
{
	hWnd = InhWnd;
	WindowSizeX = InWindowSizeX;
	WindowSizeY = InWindowSizeY;

}

void CController::EnableCursor()
{
	bCursorEnabled = true;
	ShowCursor();
	//EnableImGuiMouse();
	FreeCursor();

}

void CController::DisableCursor()
{
	bCursorEnabled = false;
	HideCursor();
	//DisableImGuiMouse();
	ConfineCursor();

}
BOOL CController::IsCursorEnabled() const
{
	return bCursorEnabled;
}

void CController::ConfineCursor()
{
	RECT rect;
	GetClientRect(hWnd, &rect);
	MapWindowPoints(hWnd, nullptr, reinterpret_cast<POINT*>(&rect), 2);
	ClipCursor(&rect);
}

void CController::FreeCursor()
{
	ClipCursor(nullptr);
}

void CController::HideCursor()
{
	while (::ShowCursor(FALSE) >= 0);
}

void CController::ShowCursor()
{
	while (::ShowCursor(TRUE) < 0);
}

void CController::EnableMouseRaw()
{
	Mouse.EnableRaw();
}

void CController::DisabaleMouseRaw()
{
	Mouse.DisableRaw();
}

BOOL CController::IsMouseRawEnabled() const
{
	return Mouse.IsRawEnabled();
}

std::optional<CMouse::RawDelta> CController::ReadRawDelta()
{
	return Mouse.ReadRawDelta();
}


LRESULT CController::HandleMsg(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		/************* MOUSE MESSAGES ****************/
	case WM_MOUSEMOVE:
	{
		const POINTS pt = MAKEPOINTS(lParam);
		// cursorless exclusive gets first dibs
		if (!bCursorEnabled)
		{
			if (!Mouse.IsInWindow())
			{
				SetCapture(hWnd);
				Mouse.OnMouseEnter();
				HideCursor();
			}
			break;
		}
		//// stifle this mouse message if imgui wants to capture
		//if (imio.WantCaptureMouse)
		//{
		//	break;
		//}

		// in client region -> log move, and log enter + capture mouse (if not previously in window)
		if (pt.x >= 0 && pt.x < WindowSizeX && pt.y >= 0 && pt.y < WindowSizeY)
		{
			Mouse.OnMouseMove(pt.x, pt.y);
			if (!Mouse.IsInWindow())
			{
				SetCapture(hWnd);
				Mouse.OnMouseEnter();
			}
		}
		// not in client -> log move / maintain capture if button down
		else
		{
			if (wParam & (MK_LBUTTON | MK_RBUTTON))
			{
				Mouse.OnMouseMove(pt.x, pt.y);
			}
			// button up -> release capture / log event for leaving
			else
			{
				ReleaseCapture();
				Mouse.OnMouseLeave();
			}
		}
		break;
	}
	case WM_LBUTTONDOWN:
	{
		/*if (imio.WantCaptureMouse)
		{
			break;
		}*/
		const POINTS pt = MAKEPOINTS(lParam);
		Mouse.OnLeftPressed(pt.x, pt.y);
		SetForegroundWindow(hWnd);
		break;
	}
	case WM_RBUTTONDOWN:
	{
		/*if (imio.WantCaptureMouse)
		{
			break;
		}*/
		const POINTS pt = MAKEPOINTS(lParam);
		Mouse.OnRightPressed(pt.x, pt.y);
		break;
	}
	case WM_LBUTTONUP:
	{
		/*	if (imio.WantCaptureMouse)
			{
				break;
			}*/
		const POINTS pt = MAKEPOINTS(lParam);
		Mouse.OnLeftReleased(pt.x, pt.y);
		// release mouse if outside of window
		if (pt.x < 0 || pt.x >= WindowSizeX || pt.y < 0 || pt.y >= WindowSizeY)
		{
			ReleaseCapture();
			Mouse.OnMouseLeave();
		}
		break;
	}
	case WM_RBUTTONUP:
	{
		/*if (imio.WantCaptureMouse)
		{
			break;
		}*/
		const POINTS pt = MAKEPOINTS(lParam);
		Mouse.OnRightReleased(pt.x, pt.y);
		// release mouse if outside of window
		if (pt.x < 0 || pt.x >= WindowSizeX || pt.y < 0 || pt.y >= WindowSizeY)
		{
			ReleaseCapture();
			Mouse.OnMouseLeave();
		}
		break;
	}
	case WM_MOUSEWHEEL:
	{
		/*if (imio.WantCaptureMouse)
		{
			break;
		}*/
		const POINTS pt = MAKEPOINTS(lParam);
		const int delta = GET_WHEEL_DELTA_WPARAM(wParam);
		Mouse.OnWheelDelta(pt.x, pt.y, delta);
		break;
	}
	/************** END MOUSE MESSAGES **************/
	/************** RAW MOUSE MESSAGES **************/
	case WM_INPUT:
	{
		if (!Mouse.IsRawEnabled())
		{
			break;
		}
		UINT size;
		// first get the size of the input data
		if (GetRawInputData(
			reinterpret_cast<HRAWINPUT>(lParam),
			RID_INPUT,
			nullptr,
			&size,
			sizeof(RAWINPUTHEADER)) == -1)
		{
			// bail msg processing if error
			break;
		}
		rawBuffer.resize(size);
		// read in the input data
		if (GetRawInputData(
			reinterpret_cast<HRAWINPUT>(lParam),
			RID_INPUT,
			rawBuffer.data(),
			&size,
			sizeof(RAWINPUTHEADER)) != size)
		{
			// bail msg processing if error
			break;
		}
		// process the raw input data
		auto& ri = reinterpret_cast<const RAWINPUT&>(*rawBuffer.data());
		if (ri.header.dwType == RIM_TYPEMOUSE &&
			(ri.data.mouse.lLastX != 0 || ri.data.mouse.lLastY != 0))
		{
			Mouse.OnRawDelta(ri.data.mouse.lLastX, ri.data.mouse.lLastY);
		}
		break;
	}
	/************** END RAW MOUSE MESSAGES **************/
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}