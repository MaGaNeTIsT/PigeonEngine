#pragma once

#include <CoreMinimal.h>

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

#if _EDITOR_ONLY
	namespace EngineSystemTime
	{

		struct ESystemTime
		{
			ESystemTime(const USHORT InYear, const USHORT InMonth, const USHORT InDayOfWeek,
				const USHORT InDay, const USHORT InHour, const USHORT InMinute, const USHORT InSecond,
				const USHORT InMilliseconds)noexcept : Year(InYear), Month(InMonth), DayOfWeek(InDayOfWeek),
				Day(InDay), Hour(InHour), Minute(InMinute), Second(InSecond), Milliseconds(InMilliseconds) {}
			ESystemTime()noexcept : Year(0u), Month(0u), DayOfWeek(0u), Day(0u),
				Hour(0u), Minute(0u), Second(0u), Milliseconds(0u) {}
			ESystemTime(const ESystemTime& Other)noexcept : Year(Other.Year), Month(Other.Month),
				DayOfWeek(Other.DayOfWeek), Day(Other.Day), Hour(Other.Hour), Minute(Other.Minute),
				Second(Other.Second), Milliseconds(Other.Milliseconds) {}
			ESystemTime& operator=(const ESystemTime& Other)
			{
				Year = Other.Year; Month = Other.Month;
				DayOfWeek = Other.DayOfWeek; Day = Other.Day;
				Hour = Other.Hour; Minute = Other.Minute; Second = Other.Second;
				Milliseconds = Other.Milliseconds;
				return (*this);
			}

			USHORT	Year;
			USHORT	Month;
			USHORT	DayOfWeek;
			USHORT	Day;
			USHORT	Hour;
			USHORT	Minute;
			USHORT	Second;
			USHORT	Milliseconds;
		};

		extern PE_INLINE ESystemTime Now();

	};
#endif

};