#pragma once

#include "../../../../Entry/EngineMain.h"
#include "../../EngineRender/RenderBase/Headers/CStructCommon.h"

class CGPUProfilerManager
{
public:
	class CGPUProfiler
	{
	public:
		void	ResetAll(const UINT* newRecordFrameCount = NULL, const std::string* newName = NULL);
		void	ResetData();
		void	Begin();
		void	End();
	public:
		void	Counter(const ULONGLONG& currentFrameIndex, std::function<void(void)> const& func);
	public:
		BOOL	IsAlreadyInitialize() { return (this->m_AlreadyInit); }
		BOOL	IsAlreadyBegin() { return (this->m_AlreadyBegin); }
		UINT	GetRecordFrameCount() { return (this->m_RecordFrameCount); }
		DOUBLE	GetAverageTime() { return (this->m_AverageTime); }
	public:
		CGPUProfiler();
		CGPUProfiler(const std::string& name);
		CGPUProfiler(const std::string& name, const UINT& recordFrameCount);
		CGPUProfiler(const CGPUProfiler& profiler);
		~CGPUProfiler();
	private:
		std::string													m_Name;
		BOOL														m_AlreadyInit;
		BOOL														m_AlreadyBegin;
		UINT														m_RecordFrameCount;
		DOUBLE														m_AverageTime;
		std::vector<std::shared_ptr<class CGPUQuery>>				m_QueryTimeStart;
		std::vector<std::shared_ptr<class CGPUQuery>>				m_QueryTimeEnd;
		std::vector<std::shared_ptr<class CGPUQuery>>				m_QueryTimeDisjoint;
		std::vector<BOOL>											m_ReadBackSucceed;
		std::vector<ULONGLONG>										m_StartData;
		std::vector<ULONGLONG>										m_EndData;
		std::vector<CustomStruct::CRenderQueryTimestampDisjoint>	m_DisjointData;
	};
public:
	static void		ShutDown();
public:
	static void		AddPass(const std::string& name, const ULONGLONG& currentFrameIndex, std::function<void(void)> const& func, const UINT& recordFrameCount = ENGINE_GPU_PROFILER_RECORD_FRAME_COUNT);
	static DOUBLE	GetPassAverageTime(const std::string& name);
public:
	static void		ClearProfilers();
	static std::shared_ptr<CGPUProfiler>	NewProfiler(const std::string& name, const UINT& recordFrameCount = ENGINE_GPU_PROFILER_RECORD_FRAME_COUNT);
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