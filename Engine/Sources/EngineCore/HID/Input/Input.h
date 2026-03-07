#pragma once

#include "../../Main/Main.h"
#include <Base/DataStructure/BuiltInType.h>
#include <Config/EngineConfig.h>
#include "Mouse.h"
#include "Keyboard.h"
#include "Base/Delegate/Delegate.h"

namespace PigeonEngine
{
	MAKE_DELEGATE_MULTI_ONE_PARAM(OnMyMouseEvent, IMouse::Event::EType)
	MAKE_DELEGATE_MULTI_TWO_PARAM(OnMyKeyEvent, IKeyboard::Event::EType, BYTE)
	class IController
	{
	public:
		void Initialize(HWND InhWnd, INT32 WindowSizeX, INT32 WindowSizeY);

	private:
		INT32 WindowSizeX = static_cast<INT32>(EEngineSettings::ENGINE_SCREEN_WIDTH);
		INT32 WindowSizeY = static_cast<INT32>(EEngineSettings::ENGINE_SCREEN_HEIGHT);
		HWND hWnd = NULL;

		/*Mouse part start*/
	public:
		std::pair<INT32, INT32> GetMousePosition() const;

		void EnableCursor();
		void DisableCursor();
		BOOL32 IsCursorEnabled() const;

		void ConfineCursor();
		void FreeCursor();
		void HideCursor();
		void ShowCursor();

		void EnableMouseRaw();
		void DisableMouseRaw();
		BOOL32 IsMouseRawEnabled() const;

		BOOL32 IsLeftMouseButtonDown()const;
		BOOL32 IsRightMouseButtonDown()const;
		std::optional<IMouse::RawDelta> ReadRawDelta();

	private:
		IMouse Mouse;
		BOOL32 bCursorEnabled = TRUE;
		std::vector<BYTE> rawBuffer;

		/*Mouse part end*/

		/*Keyboard Part Start*/
	public:
		// key event stuff
		BOOL32 IsKeyPressed(BYTE keycode) const;
		std::optional<IKeyboard::Event> ReadKey();
		BOOL32 IsKeyEmpty() const;
		void FlushKey();
		// char event stuff
		std::optional<char> ReadChar();
		BOOL32 IsCharEmpty() const;
		void FlushChar();
		void Flush();
		// autorepeat control
		void EnableAutorepeat();
		void DisableAutorepeat();
		BOOL32 IsAutorepeatEnabled() const;
	private:
		IKeyboard Keyboard;

		/*Keyboard Part End*/

	public:
		// WIndow message handler
		LRESULT HandleMsg(HWND hWnd, UINT32 msg, WPARAM wParam, LPARAM lParam);
	};

	class EInput
	{
	public:
		static void		Initialize(HWND hWnd);
		static void		ShutDown();
		static void		Update();
		//public:
		//	static BOOL32		GetKeyPress(BYTE KeyCode);
		//	static BOOL32		GetKeyTrigger(BYTE KeyCode);
		//private:
		//	static BYTE		m_OldKeyState[256];
		//	static BYTE		m_KeyState[256];
	public:
		static std::optional<IMouse::RawDelta>	ReadRawDelta();
	public:
		// WIndow message handler
		static LRESULT	HandleMsg(HWND hWnd, UINT32 msg, WPARAM wParam, LPARAM lParam);
	public:
		static IController	Controller;

	public:
		static OnMyMouseEvent MouseEvent;
		static OnMyKeyEvent KeyEvent;
	};

}

