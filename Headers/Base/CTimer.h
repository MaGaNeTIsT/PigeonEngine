#pragma once

#include <windows.h>

class CTimer
{
public:
	void			Init();
	void			Update();
public:
	DOUBLE			GetClockTime()const;
	DOUBLE			GetDeltaTime()const;
public:
	CTimer();
	virtual ~CTimer();
protected:
	LARGE_INTEGER	m_T1;
	LARGE_INTEGER	m_T2;
	LARGE_INTEGER	m_Ticks;
protected:
	DOUBLE			m_Delta;
	DOUBLE			m_Clock;
};

class CGameTimer
{
public:
	void	Reset();
	void	Update();
public:
	DOUBLE	GetClockTime()const;
	DOUBLE	GetDeltaTime()const;
public:
	CGameTimer(CTimer* timer);
	virtual ~CGameTimer();
protected:
	CTimer*	m_Timer;
	DOUBLE	m_Total;
	DOUBLE	m_Delta;
};