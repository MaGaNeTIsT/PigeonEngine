#pragma once
#include "CMouse.h"
#include "CKeyboard.h"
#include <vector>

class CController
{

public:

	void Initialize(HWND InhWnd, INT WindowSizeX, INT WindowSizeY);

private:

	INT WindowSizeX = ENGINE_SCREEN_WIDTH;
	INT WindowSizeY = ENGINE_SCREEN_HEIGHT;
	HWND hWnd;

	/*Mouse part start*/
public:
	
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
	std::optional<CMouse::RawDelta> ReadRawDelta();

private:

	CMouse Mouse;
	BOOL bCursorEnabled = false;
	std::vector<BYTE> rawBuffer;
	/*Mouse part end*/

	/*Keyboard Part Start*/
public:
	// key event stuff
	BOOL IsKeyPressed(unsigned char keycode) const;
	std::optional<CKeyboard::Event> ReadKey();
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
	CKeyboard Keyboard;

	/*Keyboard Part End*/

public:
	// WIndow message handler
	LRESULT HandleMsg(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

};

class CInput
{
private:
	static BYTE m_OldKeyState[256];
	static BYTE m_KeyState[256];

public:

	static void	Initialize(HWND hWnd);
	static void	ShutDown();
	static void	Update();
public:
	static BOOL	GetKeyPress( BYTE KeyCode );
	static BOOL	GetKeyTrigger( BYTE KeyCode );

public:

	static std::optional<CMouse::RawDelta> ReadRawDelta();

public:
	// WIndow message handler
	static LRESULT HandleMsg(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

public:
	static CController Controller;
};

