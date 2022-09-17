#include "../../../../Entry/EngineMain.h"
#include "../Headers/CTimer.h"

CTimer::CTimer()
{
	ZeroMemory(&this->m_T1, sizeof(this->m_T1));
	ZeroMemory(&this->m_T2, sizeof(this->m_T2));
	ZeroMemory(&this->m_Ticks, sizeof(this->m_Ticks));
	this->m_Delta = 0;
	this->m_Clock = 0;
}
CTimer::~CTimer()
{
}
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
	this->m_Delta = static_cast<DOUBLE>(this->m_T2.QuadPart - this->m_T1.QuadPart);
	if (this->m_Delta < 0)
		this->m_Delta = 0;
	this->m_Delta = this->m_Delta / (static_cast<DOUBLE>(this->m_Ticks.QuadPart));
	this->m_Clock += this->m_Delta;
	QueryPerformanceCounter(&this->m_T1);
}
DOUBLE CTimer::GetClockTime()const
{
	return this->m_Clock;
}
DOUBLE CTimer::GetDeltaTime()const
{
	return this->m_Delta;
}


CGameTimer::CGameTimer(CTimer* timer)
{
	this->m_Timer = timer;
	this->m_Total = 0;
	this->m_Delta = 0;
}
CGameTimer::~CGameTimer()
{
}
void CGameTimer::Reset()
{
	this->m_Total = 0;
	this->m_Delta = 0;
}
void CGameTimer::Update()
{
	if (this->m_Timer == NULL)
	{
		this->Reset();
		return;
	}
	DOUBLE temp = this->m_Timer->GetClockTime();
	m_Delta = temp - m_Total;
	m_Total = temp;
}
DOUBLE CGameTimer::GetClockTime()const
{
	return this->m_Total;
}
DOUBLE CGameTimer::GetDeltaTime()const
{
	return this->m_Delta;
}