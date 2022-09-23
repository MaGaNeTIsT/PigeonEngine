#pragma once
#include "CMouse.h"
#include <vector>

class CController
{
public:

	CController() = default;

public:

	void Initialize(HWND InhWnd, INT WindowSizeX, INT WindowSizeY);

private:

	INT WindowSizeX = ENGINE_SCREEN_WIDTH;
	INT WindowSizeY = ENGINE_SCREEN_HEIGHT;
	HWND hWnd;

public:

	void EnableCursor();
	void DisableCursor();
	BOOL IsCursorEnabled() const;

	void ConfineCursor();
	void FreeCursor();
	void HideCursor();
	void ShowCursor();

	void EnableMouseRaw();
	void DisabaleMouseRaw();
	BOOL IsMouseRawEnabled() const;

	std::optional<CMouse::RawDelta> ReadRawDelta();

private:

	CMouse Mouse;
	BOOL bCursorEnabled = false;
	std::vector<BYTE> rawBuffer;

public:

	LRESULT HandleMsg(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

};

class CInput
{
private:
	static BYTE m_OldKeyState[256];
	static BYTE m_KeyState[256];
	static MOUSEMOVEPOINT m_OldMouseMove;
	static MOUSEMOVEPOINT m_MouseMove;
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

	static LRESULT HandleMsg(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

public:
	static CController Controller;
};

