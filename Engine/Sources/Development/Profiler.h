#pragma once

#include <CoreMinimal.h>
#include <Config/EngineConfig.h>
#include <Base/Timer/Timer.h>

namespace PigeonEngine
{

	class ETimeStamp final
	{
	public:
		void	Begin();
		void	End();
		DOUBLE	GetAverageTime()const;
	public:
		ETimeStamp();
		ETimeStamp(const ETimeStamp& Other);
		~ETimeStamp();
		ETimeStamp& operator=(const ETimeStamp& Other);
	private:
		EBaseTimer		BaseTimer;
		const CHAR*		TargetName;
		DOUBLE			StartTimeStamp;
		UINT32			FrameCounter;
		DOUBLE			AverageTime;
		DOUBLE			DuringTime[EEngineSettings::ENGINE_PROFILER_AVERAGE_NUM];
	};

	class EProfiler final : public EManagerBase
	{
	public:
		virtual void	Initialize()override;
		virtual void	ShutDown()override;
	public:
		void			ClearTraces();
		UINT32			AddNewTrace(const CHAR* InTraceName);
		void			RemoveTrace(const UINT32 InTraceID);
		void			BeginTrace(const UINT32 InTraceID);
		void			EndTrace(const UINT32 InTraceID);
		DOUBLE			GetTraceAverage(const UINT32 InTraceID);
	private:
		TMapManager<UINT32, ETimeStamp> Traces;

		CLASS_MANAGER_SINGLETON_BODY(EProfiler)

	};

};