#include "Input.h"

#include <imm.h>

#pragma comment(lib, "Imm32.lib")

namespace PigeonEngine
{
    namespace
	{
        UINT32 ResolveKeycodeForInputLogic(HWND InhWnd, WPARAM InKeycode, BOOL32 bTextInputEnabled)
		{
			if (InKeycode == VK_PROCESSKEY)
			{
                if (bTextInputEnabled)
				{
					return 0u;
				}

				const UINT32 OriginalVirtualKey = static_cast<UINT32>(::ImmGetVirtualKey(InhWnd));
				if (OriginalVirtualKey != 0u && OriginalVirtualKey != VK_PROCESSKEY)
				{
					return OriginalVirtualKey;
				}

				return 0u;
			}

            return static_cast<UINT32>(InKeycode);
		}
	}

	OnMyMouseEvent EInput::MouseEvent;
	OnMyKeyEvent   EInput::KeyEvent;
	IController    EInput::Controller;
	//BYTE EInput::m_OldKeyState[256];
	//BYTE EInput::m_KeyState[256];

	void EInput::Initialize(HWND InhWnd)
	{
		// Controller Initialization
		Controller.Initialize(InhWnd, static_cast<INT32>(EEngineSettings::ENGINE_SCREEN_WIDTH), static_cast<INT32>(EEngineSettings::ENGINE_SCREEN_HEIGHT));

		//memset(m_OldKeyState, 0, 256);
		//memset(m_KeyState, 0, 256);
	}
	void EInput::ShutDown()
	{
	}
	void EInput::Update()
	{
		
	}
	
	TOptional<IMouse::RawDelta> EInput::ReadRawDelta()
	{
		return Controller.ReadRawDelta();
	}
	TOptional<WCHAR> EInput::ReadChar()
	{
		return Controller.ReadChar();
	}
	BOOL32 EInput::IsCharEmpty()
	{
		return Controller.IsCharEmpty();
	}
	void EInput::FlushChar()
	{
		Controller.FlushChar();
	}
	void EInput::EnableTextInput()
	{
		Controller.EnableTextInput();
	}
	void EInput::DisableTextInput()
	{
		Controller.DisableTextInput();
	}
	BOOL32 EInput::IsTextInputEnabled()
	{
		return Controller.IsTextInputEnabled();
	}
	LRESULT EInput::HandleMsg(HWND InhWnd, UINT32 msg, WPARAM wParam, LPARAM lParam)
	{
		return Controller.HandleMsg(InhWnd, msg, wParam, lParam);
	}
	void IController::Initialize(HWND InhWnd, INT32 InWindowSizeX, INT32 InWindowSizeY)
	{
		hWnd = InhWnd;
		WindowSizeX = InWindowSizeX;
		WindowSizeY = InWindowSizeY;
       bTextInputEnabled = FALSE;
		ApplyTextInputState();
	}
	void IController::UpdateWindowSize(INT32 InWindowSizeX, INT32 InWindowSizeY)
	{
		WindowSizeX = InWindowSizeX;
		WindowSizeY = InWindowSizeY;
	}
	void IController::ApplyTextInputState()
	{
        if (!bTextInputEnabled)
		{
			Keyboard.FlushChar();
		}
	}
	TPair<INT32, INT32> IController::GetMousePosition() const
	{
		return Mouse.GetPos();
	}
	void IController::EnableCursor()
	{
		bCursorEnabled = TRUE;
		ShowCursor();
		FreeCursor();
	}
	void IController::DisableCursor()
	{
		bCursorEnabled = FALSE;
		HideCursor();
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
	TOptional<IMouse::RawDelta> IController::ReadRawDelta()
	{
		return Mouse.ReadRawDelta();
	}
	LRESULT IController::HandleMsg(HWND InhWnd, UINT32 msg, WPARAM wParam, LPARAM lParam)
	{
		switch (msg)
		{
		case WM_KILLFOCUS:
			Keyboard.ClearState();
           Keyboard.FlushChar();
			break;

		case WM_ACTIVATE:
			// confine/free cursor on window to foreground/background if cursor disabled
			if (!IsCursorEnabled())
			{
                if (LOWORD(wParam) != WA_INACTIVE)
				{
					ConfineCursor();
					HideCursor();

				}
				else
				{
					FreeCursor();
					ShowCursor();

				}
			}
			if (LOWORD(wParam) == WA_INACTIVE)
			{
				Keyboard.FlushChar();
			}
			break;
			/*********** KEYBOARD MESSAGES ***********/
		case WM_KEYDOWN:
			// syskey commands need to be handled to track ALT key (VK_MENU) and F10
		case WM_SYSKEYDOWN:
		{
			const UINT32 Keycode = ResolveKeycodeForInputLogic(InhWnd, wParam, bTextInputEnabled);
			if (Keycode == 0u)
			{
				return 0;
			}
			/*if (imio.WantCaptureKeyboard)
			{
				break;
			}*/
			if (!(lParam & 0x40000000) || Keyboard.IsAutorepeatEnabled()) // filter autorepeat
			{
               Keyboard.OnKeyPressed(static_cast<BYTE>(Keycode));
			}
			break;
		}
		case WM_KEYUP:
		case WM_SYSKEYUP:
		{
			const UINT32 Keycode = ResolveKeycodeForInputLogic(InhWnd, wParam, bTextInputEnabled);
			if (Keycode == 0u)
			{
				return 0;
			}
			/*if (imio.WantCaptureKeyboard)
			{
				break;
			}*/
			Keyboard.OnKeyReleased(static_cast<BYTE>(Keycode));
			break;
		}
		case WM_CHAR:
            if (bTextInputEnabled)
			{
				Keyboard.OnChar(static_cast<WCHAR>(wParam));
			}
			return 0;
		case WM_IME_CHAR:
			if (bTextInputEnabled)
			{
				Keyboard.OnChar(static_cast<WCHAR>(wParam));
			}
			return 0;
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
					SetCapture(InhWnd);
					Mouse.OnMouseEnter();
					HideCursor();
				}
				break;
			}

			// in client region -> log move, and log enter + capture mouse (if not previously in window)
			if (pt.x >= 0 && pt.x < WindowSizeX && pt.y >= 0 && pt.y < WindowSizeY)
			{
				Mouse.OnMouseMove(pt.x, pt.y);
				if (!Mouse.IsInWindow())
				{
					SetCapture(InhWnd);
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
			SetForegroundWindow(InhWnd);
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
			const INT32 delta = GET_WHEEL_DELTA_WPARAM(wParam);
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
			rawBuffer.SetNum(size);
			// read in the input data
			if (GetRawInputData(
				reinterpret_cast<HRAWINPUT>(lParam),
				RID_INPUT,
               rawBuffer.GetData(),
				&size,
				sizeof(RAWINPUTHEADER)) != size)
			{
				// bail msg processing if error
				break;
			}
			// process the raw input data
            auto& ri = reinterpret_cast<const RAWINPUT&>(*rawBuffer.GetData());
			if (ri.header.dwType == RIM_TYPEMOUSE &&
				(ri.data.mouse.lLastX != 0 || ri.data.mouse.lLastY != 0))
			{
				Mouse.OnRawDelta(ri.data.mouse.lLastX, ri.data.mouse.lLastY);
			}
			break;
		}
		/************** END RAW MOUSE MESSAGES **************/
		}

		return ::DefWindowProc(InhWnd, msg, wParam, lParam);
	}
	BOOL32 IController::IsKeyPressed(BYTE keycode) const
	{
		return Keyboard.IsKeyPressed(keycode);
	}
	TOptional<IKeyboard::Event> IController::ReadKey()
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
    TOptional<WCHAR> IController::ReadChar()
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
	void IController::EnableTextInput()
	{
		if (!bTextInputEnabled)
		{
			bTextInputEnabled = TRUE;
			ApplyTextInputState();
		}
	}
	void IController::DisableTextInput()
	{
		if (bTextInputEnabled)
		{
			bTextInputEnabled = FALSE;
			ApplyTextInputState();
		}
	}
	BOOL32 IController::IsTextInputEnabled() const
	{
		return bTextInputEnabled;
	}

};