#include "Timer.h"

namespace PigeonEngine
{
	EBaseTimer::EBaseTimer()
	{
		::ZeroMemory(&this->m_T1, sizeof(this->m_T1));
		::ZeroMemory(&this->m_T2, sizeof(this->m_T2));
		::ZeroMemory(&this->m_Ticks, sizeof(this->m_Ticks));
		this->m_Delta = static_cast<DOUBLE>(0);
		this->m_Clock = static_cast<DOUBLE>(0);
	}
	EBaseTimer::~EBaseTimer()
	{
	}
	void EBaseTimer::Init()
	{
		::ZeroMemory(&this->m_T1, sizeof(this->m_T1));
		::ZeroMemory(&this->m_T2, sizeof(this->m_T2));
		::ZeroMemory(&this->m_Ticks, sizeof(this->m_Ticks));
		this->m_Delta = static_cast<DOUBLE>(0);
		this->m_Clock = static_cast<DOUBLE>(0);

		::QueryPerformanceFrequency(&this->m_Ticks);
		::QueryPerformanceCounter(&this->m_T1);
	}
	void EBaseTimer::Update()
	{
		::QueryPerformanceCounter(&this->m_T2);
		this->m_Delta = static_cast<DOUBLE>(this->m_T2.QuadPart - this->m_T1.QuadPart);
		if (this->m_Delta < 0) { this->m_Delta = static_cast<DOUBLE>(0); }
		this->m_Delta = this->m_Delta / (static_cast<DOUBLE>(this->m_Ticks.QuadPart));
		this->m_Clock += this->m_Delta;
		::QueryPerformanceCounter(&this->m_T1);
	}
	DOUBLE EBaseTimer::GetClockTime()const
	{
		return (this->m_Clock);
	}
	DOUBLE EBaseTimer::GetDeltaTime()const
	{
		return (this->m_Delta);
	}



	EGameTimer::EGameTimer(EBaseTimer* timer)
	{
		this->m_Timer = timer;
		this->m_Total = static_cast<DOUBLE>(0);
		this->m_Delta = static_cast<DOUBLE>(0);
	}
	EGameTimer::~EGameTimer()
	{
	}
	void EGameTimer::Reset()
	{
		this->m_Total = static_cast<DOUBLE>(0);
		this->m_Delta = static_cast<DOUBLE>(0);
	}
	void EGameTimer::Update()
	{
		if (this->m_Timer == nullptr)
		{
			this->Reset();
			return;
		}
		DOUBLE temp = this->m_Timer->GetClockTime();
		m_Delta = temp - m_Total;
		m_Total = temp;
	}
	DOUBLE EGameTimer::GetClockTime()const
	{
		return (this->m_Total);
	}
	DOUBLE EGameTimer::GetDeltaTime()const
	{
		return (this->m_Delta);
	}

	namespace EngineSystemTime
	{

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