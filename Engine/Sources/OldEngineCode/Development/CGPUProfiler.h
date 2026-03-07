/*
#pragma once

#include "../../../../Entry/EngineMain.h"

#ifdef _DEVELOPMENT_EDITOR

#include "../../EngineRender/RenderBase/Headers/CRenderStructCommon.h"

class CGPUProfilerManager
{
public:
	class CGPUProfiler
	{
	public:
		void	ResetAll(const UINT32* newRecordFrameCount = NULL, const std::string* newName = NULL);
		void	ResetData();
		void	Begin();
		void	End();
	public:
		void	Counter(const ULONGLONG& currentFrameIndex, std::function<void(void)> const& func);
	public:
		BOOL32	IsAlreadyInitialize() { return (this->m_AlreadyInit); }
		BOOL32	IsAlreadyBegin() { return (this->m_AlreadyBegin); }
		UINT32	GetRecordFrameCount() { return (this->m_RecordFrameCount); }
		DOUBLE	GetAverageTime() { return (this->m_AverageTime); }
	public:
		CGPUProfiler();
		CGPUProfiler(const std::string& name);
		CGPUProfiler(const std::string& name, const UINT32& recordFrameCount);
		CGPUProfiler(const CGPUProfiler& profiler);
		~CGPUProfiler();
	private:
		std::string													m_Name;
		BOOL32														m_AlreadyInit;
		BOOL32														m_AlreadyBegin;
		UINT32														m_RecordFrameCount;
		DOUBLE														m_AverageTime;
		std::vector<std::shared_ptr<class CGPUQuery>>				m_QueryTimeStart;
		std::vector<std::shared_ptr<class CGPUQuery>>				m_QueryTimeEnd;
		std::vector<std::shared_ptr<class CGPUQuery>>				m_QueryTimeDisjoint;
		std::vector<BOOL32>											m_ReadBackSucceed;
		std::vector<ULONGLONG>										m_StartData;
		std::vector<ULONGLONG>										m_EndData;
		std::vector<CustomStruct::CRenderQueryTimestampDisjoint>	m_DisjointData;
	};
public:
	static void		ShutDown();
public:
	static void		AddPass(const std::string& name, const ULONGLONG& currentFrameIndex, std::function<void(void)> const& func, const UINT32& recordFrameCount = ENGINE_GPU_PROFILER_RECORD_FRAME_COUNT);
	static DOUBLE	GetPassAverageTime(const std::string& name);
public:
	static void		ClearProfilers();
	static std::shared_ptr<CGPUProfiler>	NewProfiler(const std::string& name, const UINT32& recordFrameCount = ENGINE_GPU_PROFILER_RECORD_FRAME_COUNT);
private:
	static void								AddProfiler(const std::string& name, std::shared_ptr<CGPUProfiler> profiler);
	static std::shared_ptr<CGPUProfiler>	FindProfiler(const std::string& name);
private:
	std::map<std::string, std::shared_ptr<CGPUProfiler>>	m_Profilers;
private:
	CGPUProfilerManager() {}
	CGPUProfilerManager(const CGPUProfilerManager&) {}
	~CGPUProfilerManager() {}
private:
	static CGPUProfilerManager* m_GPUProfilerManager;
};

#else

class CGPUProfilerManager
{
public:
	static void AddPass(const std::string& name, const ULONGLONG& currentFrameIndex, std::function<void(void)> const& func, const UINT32& recordFrameCount = ENGINE_GPU_PROFILER_RECORD_FRAME_COUNT);
public:
	CGPUProfilerManager() {}
	~CGPUProfilerManager() {}
};

#endif
*/