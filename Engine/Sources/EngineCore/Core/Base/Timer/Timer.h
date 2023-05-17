#pragma once

#include "../../Core/Headers/Main.h"

namespace PigeonEngine
{
	class EBaseTimer
	{
	public:
		void	Init();
		void	Update();
	public:
		DOUBLE	GetClockTime()const;
		DOUBLE	GetDeltaTime()const;
	public:
		EBaseTimer();
		virtual ~EBaseTimer();
	protected:
		LARGE_INTEGER	m_T1;
		LARGE_INTEGER	m_T2;
		LARGE_INTEGER	m_Ticks;
	protected:
		DOUBLE			m_Delta;
		DOUBLE			m_Clock;
	};

	class EGameTimer
	{
	public:
		void	Reset();
		void	Update();
	public:
		DOUBLE	GetClockTime()const;
		DOUBLE	GetDeltaTime()const;
	public:
		EGameTimer(EBaseTimer* timer);
		virtual ~EGameTimer();
	protected:
		EBaseTimer*		m_Timer;
		DOUBLE			m_Total;
		DOUBLE			m_Delta;
	};
};