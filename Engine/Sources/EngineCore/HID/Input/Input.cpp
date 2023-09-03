#include "Input.h"

namespace PigeonEngine
{

	IController EInput::Controller;
	//BYTE EInput::m_OldKeyState[256];
	//BYTE EInput::m_KeyState[256];

	void EInput::Initialize(HWND hWnd)
	{
		// Controller Initialization
		Controller.Initialize(hWnd, static_cast<INT32>(ESettings::ENGINE_SCREEN_WIDTH), static_cast<INT32>(ESettings::ENGINE_SCREEN_HEIGHT));

		//memset(m_OldKeyState, 0, 256);
		//memset(m_KeyState, 0, 256);
	}
	void EInput::ShutDown()
	{
	}
	void EInput::Update()
	{
		//{
		//	memcpy(m_OldKeyState, m_KeyState, 256);
		//}

		//{
		//	BOOL32 keyResult = GetKeyboardState(m_KeyState);
		//}
	}
	//BOOL32 EInput::GetKeyPress(BYTE KeyCode)
	//{
	//	INT32 curr = (m_KeyState[KeyCode] & 0x80);
	//	return (curr > 0);
	//}
	//BOOL32 EInput::GetKeyTrigger(BYTE KeyCode)
	//{
	//	INT32 curr = (m_KeyState[KeyCode] & 0x80);
	//	INT32 prev = (m_OldKeyState[KeyCode] & 0x80);
	//	return ((curr > 0) && (prev == 0));
	//}
	std::optional<IMouse::RawDelta> EInput::ReadRawDelta()
	{
		return Controller.ReadRawDelta();
	}
	LRESULT EInput::HandleMsg(HWND hWnd, UINT32 msg, WPARAM wParam, LPARAM lParam)
	{
		return Controller.HandleMsg(hWnd, msg, wParam, lParam);
	}
	void IController::Initialize(HWND InhWnd, INT32 InWindowSizeX, INT32 InWindowSizeY)
	{
		hWnd = InhWnd;
		WindowSizeX = InWindowSizeX;
		WindowSizeY = InWindowSizeY;
	}
	std::pair<INT32, INT32> IController::GetMousePosition() const
	{
		return Mouse.GetPos();
	}
	void IController::EnableCursor()
	{
		bCursorEnabled = true;
		ShowCursor();
		//EnableImGuiMouse();
		FreeCursor();
	}
	void IController::DisableCursor()
	{
		bCursorEnabled = false;
		HideCursor();
		//DisableImGuiMouse();
		ConfineCursor();
	}
	BOOL32 IController::IsCursorEnabled() const
	{
		return bCursorEnabled;
	}
	void IController::ConfineCursor()
	{
		RECT rect;
		GetClientRect(hWnd, &rect);
		MapWindowPoints(hWnd, nullptr, reinterpret_cast<POINT*>(&rect), 2);
		ClipCursor(&rect);
	}
	void IController::FreeCursor()
	{
		ClipCursor(nullptr);
	}
	void IController::HideCursor()
	{
		while (::ShowCursor(FALSE) >= 0);
	}
	void IController::ShowCursor()
	{
		while (::ShowCursor(TRUE) < 0);
	}
	void IController::EnableMouseRaw()
	{
		Mouse.EnableRaw();
	}
	void IController::DisableMouseRaw()
	{
		Mouse.DisableRaw();
	}
	BOOL32 IController::IsMouseRawEnabled() const
	{
		return Mouse.IsRawEnabled();
	}
	BOOL32 IController::IsLeftMouseButtonDown() const
	{
		return Mouse.LeftIsPressed();
	}
	BOOL32 IController::IsRightMouseButtonDown() const
	{
		return Mouse.RightIsPressed();
	}
	std::optional<IMouse::RawDelta> IController::ReadRawDelta()
	{
		return Mouse.ReadRawDelta();
	}
	LRESULT IController::HandleMsg(HWND hWnd, UINT32 msg, WPARAM wParam, LPARAM lParam)
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
			UINT32 size = 0u;
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
	BOOL32 IController::IsKeyPressed(unsigned char keycode) const
	{
		return Keyboard.IsKeyPressed(keycode);
	}
	std::optional<IKeyboard::Event> IController::ReadKey()
	{
		return Keyboard.ReadKey();
	}
	BOOL32 IController::IsKeyEmpty() const
	{
		return Keyboard.IsKeyEmpty();
	}
	void IController::FlushKey()
	{
		Keyboard.FlushKey();
	}
	std::optional<char> IController::ReadChar()
	{
		return Keyboard.ReadChar();
	}
	BOOL32 IController::IsCharEmpty() const
	{
		return Keyboard.IsCharEmpty();
	}
	void IController::FlushChar()
	{
		Keyboard.FlushChar();
	}
	void IController::Flush()
	{
		Keyboard.Flush();
	}
	void IController::EnableAutorepeat()
	{
		Keyboard.EnableAutorepeat();
	}
	void IController::DisableAutorepeat()
	{
		Keyboard.DisableAutorepeat();
	}
	BOOL32 IController::IsAutorepeatEnabled() const
	{
		return Keyboard.IsAutorepeatEnabled();
	}

};