#include "../../Headers/Base/CTimer.h"

CTimer::CTimer()
{
	ZeroMemory(&this->m_T1, sizeof(this->m_T1));
	ZeroMemory(&this->m_T2, sizeof(this->m_T2));
	ZeroMemory(&this->m_Ticks, sizeof(this->m_Ticks));
	this->m_Delta = 0;
	this->m_Clock = 0;
}

CTimer::~CTimer() { ; }

void CTimer::Init()
{
	ZeroMemory(&this->m_T1, sizeof(this->m_T1));
	ZeroMemory(&this->m_T2, sizeof(this->m_T2));
	ZeroMemory(&this->m_Ticks, sizeof(this->m_Ticks));
	this->m_Delta = 0;
	this->m_Clock = 0;

	QueryPerformanceFrequency(&this->m_Ticks);
	QueryPerformanceCounter(&this->m_T1);
}

void CTimer::Update()
{
	QueryPerformanceCounter(&this->m_T2);
	this->m_Delta = (DOUBLE)(this->m_T2.QuadPart - this->m_T1.QuadPart);
	if (this->m_Delta < 0)
		this->m_Delta = 0;
	this->m_Clock += this->m_Delta / ((DOUBLE)this->m_Ticks.QuadPart);
	QueryPerformanceCounter(&this->m_T1);
}

DOUBLE CTimer::GetClockTime()const
{
	return this->m_Clock;
}

DOUBLE CTimer::GetDeltaTime()const
{
	return this->m_Delta / ((DOUBLE)this->m_Ticks.QuadPart);
}