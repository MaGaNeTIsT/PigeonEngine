#pragma once

class CInput
{
private:
	static BYTE m_OldKeyState[256];
	static BYTE m_KeyState[256];
	static MOUSEMOVEPOINT m_OldMouseMove;
	static MOUSEMOVEPOINT m_MouseMove;
public:
	static void	Initialize();
	static void	ShutDown();
	static void	Update();
public:
	static BOOL	GetKeyPress( BYTE KeyCode );
	static BOOL	GetKeyTrigger( BYTE KeyCode );
};