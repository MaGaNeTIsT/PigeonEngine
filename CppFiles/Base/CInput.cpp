#include "../../Entry/MyMain.h"
#include "../../Headers/Base/CInput.h"

BYTE CInput::m_OldKeyState[256];
BYTE CInput::m_KeyState[256];

void CInput::Init()
{
	memset(m_OldKeyState, 0, 256);
	memset(m_KeyState, 0, 256);
}

void CInput::Uninit()
{

}

void CInput::Update()
{
	memcpy(m_OldKeyState, m_KeyState, 256);
	GetKeyboardState(m_KeyState);
}

bool CInput::GetKeyPress(BYTE KeyCode)
{
	return (m_KeyState[KeyCode] & 0x80);
}

bool CInput::GetKeyTrigger(BYTE KeyCode)
{
	return ((m_KeyState[KeyCode] & 0x80) && !(m_OldKeyState[KeyCode] & 0x80));
}