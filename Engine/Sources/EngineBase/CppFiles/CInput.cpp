#include "../../../../Entry/EngineMain.h"
#include "../Headers/CInput.h"

BYTE CInput::m_OldKeyState[256];
BYTE CInput::m_KeyState[256];
MOUSEMOVEPOINT CInput::m_OldMouseMove;
MOUSEMOVEPOINT CInput::m_MouseMove;

void CInput::Initialize()
{
	memset(m_OldKeyState, 0, 256);
	memset(m_KeyState, 0, 256);
	::ZeroMemory(&m_OldMouseMove, sizeof(MOUSEMOVEPOINT));
	::ZeroMemory(&m_MouseMove, sizeof(MOUSEMOVEPOINT));
}
void CInput::ShutDown()
{
}
void CInput::Update()
{
	{
		memcpy(m_OldKeyState, m_KeyState, 256);
		memcpy(&m_OldMouseMove, &m_MouseMove, sizeof(MOUSEMOVEPOINT));
	}

	{
		BOOL keyResult = GetKeyboardState(m_KeyState);
	}

	{
		//GetCursorPos();
		//if (GetMouseMovePointsEx( NULL, &m_MouseMove, 1, GMMP_USE_DISPLAY_POINTS) != -1)
		//{
		//	INT nVirtualWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
		//	INT nVirtualHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);
		//	INT nVirtualLeft = GetSystemMetrics(SM_XVIRTUALSCREEN);
		//	INT nVirtualTop = GetSystemMetrics(SM_YVIRTUALSCREEN);
		//	INT cpt = 0;
		//	INT mode = GMMP_USE_DISPLAY_POINTS;

		//	MOUSEMOVEPOINT mp_in;
		//	MOUSEMOVEPOINT mp_out[64];

		//	::ZeroMemory(&mp_in, sizeof(MOUSEMOVEPOINT));
		//	//Ensure that this number will pass through.
		//	mp_in.x = pt.x & 0x0000FFFF;
		//	mp_in.y = pt.y & 0x0000FFFF;
		//	cpt = GetMouseMovePointsEx(sizeof(MOUSEMOVEPOINT), &mp_in, &mp_out, 64, mode);

		//	for (int i = 0; i < cpt; i++)
		//	{
		//		switch (mode)
		//		{
		//		case GMMP_USE_DISPLAY_POINTS:
		//			if (mp_out[i].x > 32767)
		//				mp_out[i].x -= 65536;
		//			if (mp_out[i].y > 32767)
		//				mp_out[i].y -= 65536;
		//			break;
		//		case GMMP_USE_HIGH_RESOLUTION_POINTS:
		//			mp_out[i].x = ((mp_out[i].x * (nVirtualWidth - 1)) - (nVirtualLeft * 65536)) / nVirtualWidth;
		//			mp_out[i].y = ((mp_out[i].y * (nVirtualHeight - 1)) - (nVirtualTop * 65536)) / nVirtualHeight;
		//			break;
		//		}
		//	}
		//}
	}
}
BOOL CInput::GetKeyPress(BYTE KeyCode)
{
	INT curr = (m_KeyState[KeyCode] & 0x80);
	return (curr > 0);
}
BOOL CInput::GetKeyTrigger(BYTE KeyCode)
{
	INT curr = (m_KeyState[KeyCode] & 0x80);
	INT prev = (m_OldKeyState[KeyCode] & 0x80);
	return ((curr > 0) && (prev == 0));
}