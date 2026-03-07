#include "Profiler.h"

namespace PigeonEngine
{

	ETimeStamp::ETimeStamp()
		: TargetName(nullptr), StartTimeStamp(EMath::TruncToDouble(0))
		, FrameCounter(0u), AverageTime(EMath::TruncToDouble(0))
	{
		for (UINT32 i = 0u; i < EEngineSettings::ENGINE_PROFILER_AVERAGE_NUM; i++)
		{
			DuringTime[i] = EMath::TruncToDouble(0);
		}
	}
	ETimeStamp::ETimeStamp(const ETimeStamp& Other)
		: TargetName(Other.TargetName), StartTimeStamp(EMath::TruncToDouble(0))
		, FrameCounter(0u), AverageTime(EMath::TruncToDouble(0))
	{
		for (UINT32 i = 0u; i < EEngineSettings::ENGINE_PROFILER_AVERAGE_NUM; i++)
		{
			DuringTime[i] = EMath::TruncToDouble(0);
		}
	}
	ETimeStamp::~ETimeStamp()
	{
	}
	ETimeStamp& ETimeStamp::operator=(const ETimeStamp& Other)
	{
		TargetName = Other.TargetName;
		StartTimeStamp = EMath::TruncToDouble(0);
		FrameCounter = 0u;
		AverageTime = EMath::TruncToDouble(0);
		for (UINT32 i = 0u; i < EEngineSettings::ENGINE_PROFILER_AVERAGE_NUM; i++)
		{
			DuringTime[i] = EMath::TruncToDouble(0);
		}
		BaseTimer.Init();
		return (*this);
	}
	void ETimeStamp::Begin()
	{
		BaseTimer.Update();
		StartTimeStamp = BaseTimer.GetClockTime();
	}
	void ETimeStamp::End()
	{
		BaseTimer.Update();
		DuringTime[FrameCounter % EEngineSettings::ENGINE_PROFILER_AVERAGE_NUM] = BaseTimer.GetClockTime() - StartTimeStamp;
		AverageTime = EMath::TruncToDouble(0);
		for (UINT32 i = 0u; i < EEngineSettings::ENGINE_PROFILER_AVERAGE_NUM; i++)
		{
			AverageTime += DuringTime[i];
		}
		static_assert(EEngineSettings::ENGINE_PROFILER_AVERAGE_NUM != 0u, "Profiler average num can not be zero!");
		AverageTime /= EEngineSettings::ENGINE_PROFILER_AVERAGE_NUM;
		FrameCounter++;
	}
	DOUBLE ETimeStamp::GetAverageTime()const
	{
		return AverageTime;
	}

	EProfiler::EProfiler()
	{
	}
	EProfiler::~EProfiler()
	{
	}
	void EProfiler::Initialize()
	{
	}
	void EProfiler::ShutDown()
	{
	}
	void EProfiler::ClearTraces()
	{
		Traces.Empty();
	}
	UINT32 EProfiler::AddNewTrace(const CHAR* InTraceName)
	{
		return 0u;
	}
	void EProfiler::RemoveTrace(const UINT32 InTraceID)
	{

	}
	void EProfiler::BeginTrace(const UINT32 InTraceID)
	{

	}
	void EProfiler::EndTrace(const UINT32 InTraceID)
	{

	}
	DOUBLE EProfiler::GetTraceAverage(const UINT32 InTraceID)
	{
		return 0.0;
	}

};