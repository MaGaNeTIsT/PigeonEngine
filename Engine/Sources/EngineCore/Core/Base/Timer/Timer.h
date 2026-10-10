#pragma once

#include <CoreMinimal.h>

namespace PigeonEngine
{
	// EBaseTimer
	//
	// Thin wrapper around Windows QueryPerformanceCounter / QueryPerformanceFrequency.
	// Two usage modes coexist:
	//
	// 1) **Stateless static helpers** (preferred for new code, fully thread-safe):
	//      EBaseTimer::GetClockTicks()    -> UINT64 raw counter
	//      EBaseTimer::GetClockSeconds()  -> DOUBLE wall-clock seconds
	//      EBaseTimer::TicksToSeconds(t)  -> DOUBLE
	//    These touch only OS APIs (QPC) and a lazy-initialised cached frequency.
	//    Safe to call from any thread concurrently - no shared mutable state.
	//
	// 2) **Instance-based** (existing engine timing code): per-instance m_T1/m_T2
	//    cache the last sample. Each instance is owned by exactly one thread; two
	//    threads must NOT share a single EBaseTimer instance. Internally the
	//    instance methods call the static helpers above, so the only thread-safety
	//    contract is "don't share the object".
	class EBaseTimer
	{
	public:
		// Stateless helpers - safe to call from any thread.
		static UINT64	GetClockTicks();
		static DOUBLE	GetClockSeconds();
		static DOUBLE	TicksToSeconds(UINT64 InTicks);
		static UINT64	GetTicksPerSecond();

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
		::LARGE_INTEGER	m_T1;
		::LARGE_INTEGER	m_T2;
		::LARGE_INTEGER	m_Ticks;
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

			PE_NODISCARD EString GetDateAsString() const;
			PE_NODISCARD EString GetTimeAsString() const;
			PE_NODISCARD EString AsString() const;

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

};
