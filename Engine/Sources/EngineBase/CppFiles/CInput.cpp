#include "../../../../Entry/EngineMain.h"
#include "../Headers/CInput.h"

BYTE CInput::m_OldKeyState[256];
BYTE CInput::m_KeyState[256];

void CInput::Initialize()
{
	memset(m_OldKeyState, 0, 256);
	memset(m_KeyState, 0, 256);
}

void CInput::ShutDown()
{

}

void CInput::Update()
{
	memcpy(m_OldKeyState, m_KeyState, 256);
	BOOL result = GetKeyboardState(m_KeyState);
}

BOOL CInput::GetKeyPress(BYTE KeyCode)
{
	return (m_KeyState[KeyCode] & 0x80);
}

BOOL CInput::GetKeyTrigger(BYTE KeyCode)
{
	INT curr = (m_KeyState[KeyCode] & 0x80);
	INT prev = (m_OldKeyState[KeyCode] & 0x80);
	return ((curr == 1) && (prev == 0));
}