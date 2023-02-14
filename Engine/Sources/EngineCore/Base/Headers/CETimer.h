#pragma once

#include "../../Core/Headers/CEMain.h"

namespace PigeonEngine
{
	class CEBaseTimer
	{
	public:
		void	Init();
		void	Update();
	public:
		DOUBLE	GetClockTime()const;
		DOUBLE	GetDeltaTime()const;
	public:
		CEBaseTimer();
		virtual ~CEBaseTimer();
	protected:
		LARGE_INTEGER	m_T1;
		LARGE_INTEGER	m_T2;
		LARGE_INTEGER	m_Ticks;
	protected:
		DOUBLE			m_Delta;
		DOUBLE			m_Clock;
	};

	class CEGameTimer
	{
	public:
		void	Reset();
		void	Update();
	public:
		DOUBLE	GetClockTime()const;
		DOUBLE	GetDeltaTime()const;
	public:
		CEGameTimer(CEBaseTimer* timer);
		virtual ~CEGameTimer();
	protected:
		CEBaseTimer*	m_Timer;
		DOUBLE			m_Total;
		DOUBLE			m_Delta;
	};
};