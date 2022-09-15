#pragma once

class CInput
{
private:
	static BYTE m_OldKeyState[256];
	static BYTE m_KeyState[256];
public:
	static void	Initialize();
	static void	ShutDown();
	static void	Update();

	static BOOL	GetKeyPress( BYTE KeyCode );
	static BOOL	GetKeyTrigger( BYTE KeyCode );
};