#pragma once

#include <windows.h>

class CTimer
{
public:
					CTimer();
	virtual			~CTimer();
public:
	void			Init();
	void			Update();
public:
	DOUBLE			GetClockTime()const;
	DOUBLE			GetDeltaTime()const;
private:
	LARGE_INTEGER	m_T1;
	LARGE_INTEGER	m_T2;
	LARGE_INTEGER	m_Ticks;
private:
	DOUBLE			m_Delta;
	DOUBLE			m_Clock;
};