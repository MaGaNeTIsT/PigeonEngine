#pragma once

#include <CoreMinimal.h>
#include "Mouse.h"
#include "Keyboard.h"

namespace PigeonEngine
{

	class IController
	{
	public:
		void Initialize(HWND InhWnd, INT WindowSizeX, INT WindowSizeY);

	private:
		INT WindowSizeX = static_cast<INT>(ESettings::ENGINE_SCREEN_WIDTH);
		INT WindowSizeY = static_cast<INT>(ESettings::ENGINE_SCREEN_HEIGHT);
		HWND hWnd = NULL;

		/*Mouse part start*/

	public:
		std::pair<INT, INT> GetMousePosition() const;

		void EnableCursor();
		void DisableCursor();
		BOOL IsCursorEnabled() const;

		void ConfineCursor();
		void FreeCursor();
		void HideCursor();
		void ShowCursor();

		void EnableMouseRaw();
		void DisableMouseRaw();
		BOOL IsMouseRawEnabled() const;

		BOOL IsLeftMouseButtonDown()const;
		BOOL IsRightMouseButtonDown()const;
		std::optional<IMouse::RawDelta> ReadRawDelta();

	private:
		IMouse Mouse;
		BOOL bCursorEnabled = false;
		std::vector<BYTE> rawBuffer;

		/*Mouse part end*/

		/*Keyboard Part Start*/
	public:
		// key event stuff
		BOOL IsKeyPressed(unsigned char keycode) const;
		std::optional<IKeyboard::Event> ReadKey();
		BOOL IsKeyEmpty() const;
		void FlushKey();
		// char event stuff
		std::optional<char> ReadChar();
		BOOL IsCharEmpty() const;
		void FlushChar();
		void Flush();
		// autorepeat control
		void EnableAutorepeat();
		void DisableAutorepeat();
		BOOL IsAutorepeatEnabled() const;
	private:
		IKeyboard Keyboard;

		/*Keyboard Part End*/

	public:
		// WIndow message handler
		LRESULT HandleMsg(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	};

	class EInput
	{
	public:
		static void		Initialize(HWND hWnd);
		static void		ShutDown();
		static void		Update();
		//public:
		//	static BOOL		GetKeyPress(BYTE KeyCode);
		//	static BOOL		GetKeyTrigger(BYTE KeyCode);
		//private:
		//	static BYTE		m_OldKeyState[256];
		//	static BYTE		m_KeyState[256];
	public:
		static std::optional<IMouse::RawDelta>	ReadRawDelta();
	public:
		// WIndow message handler
		static LRESULT	HandleMsg(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	public:
		static IController	Controller;
	};

};