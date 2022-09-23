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

void CController::DisableMouseRaw()
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
	case WM_KILLFOCUS:
		Keyboard.ClearState();
		break;

	case WM_ACTIVATE:
		OutputDebugString("activeate\n");
		// confine/free cursor on window to foreground/background if cursor disabled
		if (!IsCursorEnabled())
		{
			if (wParam & WA_ACTIVE)
			{
				OutputDebugString("activeate => confine\n");
				ConfineCursor();
				HideCursor();

			}
			else
			{
				OutputDebugString("activeate => free\n");
				FreeCursor();
				ShowCursor();

			}
		}
		break;
		/*********** KEYBOARD MESSAGES ***********/
	case WM_KEYDOWN:
		// syskey commands need to be handled to track ALT key (VK_MENU) and F10
	case WM_SYSKEYDOWN:
		/*if (imio.WantCaptureKeyboard)
		{
			break;
		}*/
		if (!(lParam & 0x40000000) || Keyboard.IsAutorepeatEnabled()) // filter autorepeat
		{
			Keyboard.OnKeyPressed(static_cast<unsigned char>(wParam));
		}
		break;
	case WM_KEYUP:
	case WM_SYSKEYUP:
		/*if (imio.WantCaptureKeyboard)
		{
			break;
		}*/
		Keyboard.OnKeyReleased(static_cast<unsigned char>(wParam));
		break;
	case WM_CHAR:
		/*if (imio.WantCaptureKeyboard)
		{
			break;
		}*/
		Keyboard.OnChar(static_cast<unsigned char>(wParam));
		break;
		/*********** END KEYBOARD MESSAGES ***********/
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

BOOL CController::IsKeyPressed(unsigned char keycode) const
{
	return Keyboard.IsKeyPressed(keycode);
}

std::optional<CKeyboard::Event> CController::ReadKey()
{
	return Keyboard.ReadKey();
}

BOOL CController::IsKeyEmpty() const
{
	return Keyboard.IsKeyEmpty();
}

void CController::FlushKey()
{
	Keyboard.FlushKey();
}

std::optional<char> CController::ReadChar()
{
	return Keyboard.ReadChar();
}

BOOL CController::IsCharEmpty() const
{
	return Keyboard.IsCharEmpty();
}

void CController::FlushChar()
{
	Keyboard.FlushChar();
}

void CController::Flush()
{
	Keyboard.Flush();
}

void CController::EnableAutorepeat()
{
	Keyboard.EnableAutorepeat();
}

void CController::DisableAutorepeat()
{
	Keyboard.DisableAutorepeat();
}

BOOL CController::IsAutorepeatEnabled() const
{
	return Keyboard.IsAutorepeatEnabled();
}
