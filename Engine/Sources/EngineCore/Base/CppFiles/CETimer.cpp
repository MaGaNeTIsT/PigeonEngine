#include "../Headers/CETimer.h"

namespace PigeonEngine
{
	CEBaseTimer::CEBaseTimer()
	{
		::ZeroMemory(&this->m_T1, sizeof(this->m_T1));
		::ZeroMemory(&this->m_T2, sizeof(this->m_T2));
		::ZeroMemory(&this->m_Ticks, sizeof(this->m_Ticks));
		this->m_Delta = static_cast<DOUBLE>(0);
		this->m_Clock = static_cast<DOUBLE>(0);
	}
	CEBaseTimer::~CEBaseTimer()
	{
	}
	void CEBaseTimer::Init()
	{
		::ZeroMemory(&this->m_T1, sizeof(this->m_T1));
		::ZeroMemory(&this->m_T2, sizeof(this->m_T2));
		::ZeroMemory(&this->m_Ticks, sizeof(this->m_Ticks));
		this->m_Delta = static_cast<DOUBLE>(0);
		this->m_Clock = static_cast<DOUBLE>(0);

		::QueryPerformanceFrequency(&this->m_Ticks);
		::QueryPerformanceCounter(&this->m_T1);
	}
	void CEBaseTimer::Update()
	{
		::QueryPerformanceCounter(&this->m_T2);
		this->m_Delta = static_cast<DOUBLE>(this->m_T2.QuadPart - this->m_T1.QuadPart);
		if (this->m_Delta < 0) { this->m_Delta = static_cast<DOUBLE>(0); }
		this->m_Delta = this->m_Delta / (static_cast<DOUBLE>(this->m_Ticks.QuadPart));
		this->m_Clock += this->m_Delta;
		::QueryPerformanceCounter(&this->m_T1);
	}
	DOUBLE CEBaseTimer::GetClockTime()const
	{
		return this->m_Clock;
	}
	DOUBLE CEBaseTimer::GetDeltaTime()const
	{
		return this->m_Delta;
	}



	CEGameTimer::CEGameTimer(CEBaseTimer* timer)
	{
		this->m_Timer = timer;
		this->m_Total = static_cast<DOUBLE>(0);
		this->m_Delta = static_cast<DOUBLE>(0);
	}
	CEGameTimer::~CEGameTimer()
	{
	}
	void CEGameTimer::Reset()
	{
		this->m_Total = static_cast<DOUBLE>(0);
		this->m_Delta = static_cast<DOUBLE>(0);
	}
	void CEGameTimer::Update()
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
	DOUBLE CEGameTimer::GetClockTime()const
	{
		return this->m_Total;
	}
	DOUBLE CEGameTimer::GetDeltaTime()const
	{
		return this->m_Delta;
	}
};