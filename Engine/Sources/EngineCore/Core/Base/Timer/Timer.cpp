#include "Timer.h"

namespace PigeonEngine
{
	// Cached QueryPerformanceFrequency result. Win32 guarantees the frequency
	// is constant from boot, so caching is safe; the magic-static guards the
	// init against concurrent first-callers across threads.
	static UINT64 GetCachedTicksPerSecond()
	{
		static const UINT64 Frequency = []() -> UINT64
		{
			::LARGE_INTEGER Freq = {};
			::QueryPerformanceFrequency(&Freq);
			return static_cast<UINT64>(Freq.QuadPart);
		}();
		return Frequency;
	}

	UINT64 EBaseTimer::GetClockTicks()
	{
		::LARGE_INTEGER Now = {};
		::QueryPerformanceCounter(&Now);
		return static_cast<UINT64>(Now.QuadPart);
	}

	DOUBLE EBaseTimer::TicksToSeconds(UINT64 InTicks)
	{
		const UINT64 Freq = GetCachedTicksPerSecond();
		if (Freq == 0u) { return EMath::TruncToDouble(0); }
		return static_cast<DOUBLE>(InTicks) / static_cast<DOUBLE>(Freq);
	}

	DOUBLE EBaseTimer::GetClockSeconds()
	{
		return TicksToSeconds(GetClockTicks());
	}

	UINT64 EBaseTimer::GetTicksPerSecond()
	{
		return GetCachedTicksPerSecond();
	}

	EBaseTimer::EBaseTimer()
	{
		::ZeroMemory(&(this->T1), sizeof(this->T1));
		::ZeroMemory(&(this->T2), sizeof(this->T2));
		::ZeroMemory(&(this->Ticks), sizeof(this->Ticks));
		this->Delta = EMath::TruncToDouble(0);
		this->Clock = EMath::TruncToDouble(0);
	}
	EBaseTimer::~EBaseTimer()
	{
	}
	void EBaseTimer::Init()
	{
		::ZeroMemory(&(this->T1), sizeof(this->T1));
		::ZeroMemory(&(this->T2), sizeof(this->T2));
		::ZeroMemory(&(this->Ticks), sizeof(this->Ticks));
		this->Delta = EMath::TruncToDouble(0);
		this->Clock = EMath::TruncToDouble(0);

		this->Ticks.QuadPart = static_cast<LONGLONG>(GetCachedTicksPerSecond());
		this->T1.QuadPart    = static_cast<LONGLONG>(GetClockTicks());
	}
	void EBaseTimer::Update()
	{
		this->T2.QuadPart = static_cast<LONGLONG>(GetClockTicks());
		LONGLONG Diff = this->T2.QuadPart - this->T1.QuadPart;
		if (Diff < 0) { Diff = 0; }
		const LONGLONG Freq = (this->Ticks.QuadPart > 0) ? this->Ticks.QuadPart : static_cast<LONGLONG>(GetCachedTicksPerSecond());
		this->Delta = static_cast<DOUBLE>(Diff) / static_cast<DOUBLE>(Freq);
		this->Clock += this->Delta;
		this->T1.QuadPart = this->T2.QuadPart;
	}
	DOUBLE EBaseTimer::GetClockTime() const
	{
		return (this->Clock);
	}
	DOUBLE EBaseTimer::GetDeltaTime() const
	{
		return (this->Delta);
	}



	EGameTimer::EGameTimer(EBaseTimer* timer)
	{
		this->Timer = timer;
		this->Total = EMath::TruncToDouble(0);
		this->Delta = EMath::TruncToDouble(0);
	}
	EGameTimer::~EGameTimer()
	{
	}
	void EGameTimer::Reset()
	{
		this->Total = EMath::TruncToDouble(0);
		this->Delta = EMath::TruncToDouble(0);
	}
	void EGameTimer::Update()
	{
		if ((this->Timer) == nullptr)
		{
			this->Reset();
			return;
		}
		DOUBLE temp = this->Timer->GetClockTime();
		Delta = temp - Total;
		Total = temp;
	}
	DOUBLE EGameTimer::GetClockTime() const
	{
		return (this->Total);
	}
	DOUBLE EGameTimer::GetDeltaTime() const
	{
		return (this->Delta);
	}

	namespace EngineSystemTime
	{

		EString EngineSystemTime::ESystemTime::GetDateAsString() const
		{
			return ToString(Year) + "-" + ToString(Month) + "-" + ToString(Day);
		}
		EString EngineSystemTime::ESystemTime::GetTimeAsString() const
		{
			return ToString(Hour) + ":" + ToString(Minute) + ":" + ToString(Second);
		}
		EString EngineSystemTime::ESystemTime::AsString() const
		{
			return ToString(Year) + "-" + ToString(Month) + "-" + ToString(Day) + " " + ToString(Hour) + ":" + ToString(Minute) + ":" + ToString(Second) + ":" + ToString(Milliseconds);
		}

		static PE_INLINE ESystemTime TranslateToSystemTime(const ::SYSTEMTIME& InStruct)
		{
			return ESystemTime(
				InStruct.wYear, InStruct.wMonth, InStruct.wDayOfWeek,
				InStruct.wDay, InStruct.wHour, InStruct.wMinute,
				InStruct.wSecond, InStruct.wMilliseconds
			);
		}

		PE_INLINE ESystemTime Now()
		{
			::SYSTEMTIME _WindowsPlatformSystemTime;
			::GetLocalTime(&_WindowsPlatformSystemTime);
			return TranslateToSystemTime(_WindowsPlatformSystemTime);
		}

	};

};
