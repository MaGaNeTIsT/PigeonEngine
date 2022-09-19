#include "../Headers/CGPUProfiler.h"
#include "../../EngineBase/Headers/CBaseType.h"
#include "../../EngineRender/RenderBase/Headers/CRenderDevice.h"
#include "../../EngineRender/RenderBase/Headers/CGPUQuery.h"
#include "../../EngineRender/RenderBase/Headers/CGPUQueryManager.h"

CGPUProfilerManager::CGPUProfiler::CGPUProfiler()
{
	this->m_Name = ENGINE_GPU_PROFILER_NONE;
	this->m_AlreadyInit = FALSE;
	this->m_AlreadyBegin = FALSE;
	this->m_RecordFrameCount = 1u;
	this->m_AverageTime = static_cast<DOUBLE>(0);
}
CGPUProfilerManager::CGPUProfiler::CGPUProfiler(const std::string& name)
{
	this->m_Name = name;
	this->m_AlreadyInit = FALSE;
	this->m_AlreadyBegin = FALSE;
	this->m_RecordFrameCount = 1u;
	this->m_AverageTime = static_cast<DOUBLE>(0);
}
CGPUProfilerManager::CGPUProfiler::CGPUProfiler(const std::string& name, const UINT& recordFrameCount)
{
	this->m_Name = name;
	this->m_AlreadyInit = FALSE;
	this->m_AlreadyBegin = FALSE;
	this->m_RecordFrameCount = recordFrameCount;
	this->m_AverageTime = static_cast<DOUBLE>(0);
}
CGPUProfilerManager::CGPUProfiler::CGPUProfiler(const CGPUProfiler& profiler)
{
	this->m_Name = profiler.m_Name;
	this->m_AlreadyInit = FALSE;
	this->m_AlreadyBegin = FALSE;
	this->m_RecordFrameCount = profiler.m_RecordFrameCount;
	this->m_AverageTime = static_cast<DOUBLE>(0);
}
CGPUProfilerManager::CGPUProfiler::~CGPUProfiler()
{
}
void CGPUProfilerManager::CGPUProfiler::ResetAll(const UINT* newRecordFrameCount, const std::string* newName)
{
	if (this->m_AlreadyBegin == TRUE)
	{
		this->End();
	}
	if (newRecordFrameCount != NULL)
	{
		this->m_RecordFrameCount = CustomType::CMath::Max((*newRecordFrameCount), 1u);
	}
	if (newName != NULL)
	{
		this->m_Name = (*newName);
	}
	if (this->m_QueryTimeStart.size() != this->m_RecordFrameCount)
	{
		this->m_QueryTimeStart.resize(this->m_RecordFrameCount);
	}
	if (this->m_QueryTimeEnd.size() != this->m_RecordFrameCount)
	{
		this->m_QueryTimeEnd.resize(this->m_RecordFrameCount);
	}
	if (this->m_QueryTimeDisjoint.size() != this->m_RecordFrameCount)
	{
		this->m_QueryTimeDisjoint.resize(this->m_RecordFrameCount);
	}
	if (this->m_ReadBackSucceed.size() != this->m_RecordFrameCount)
	{
		this->m_ReadBackSucceed.resize(this->m_RecordFrameCount);
	}
	if (this->m_StartData.size() != this->m_RecordFrameCount)
	{
		this->m_StartData.resize(this->m_RecordFrameCount);
	}
	if (this->m_EndData.size() != this->m_RecordFrameCount)
	{
		this->m_EndData.resize(this->m_RecordFrameCount);
	}
	if (this->m_DisjointData.size() != this->m_RecordFrameCount)
	{
		this->m_DisjointData.resize(this->m_RecordFrameCount);
	}
	for (UINT i = 0; i < m_RecordFrameCount; i++)
	{
		std::string tempName = this->m_Name + "_Start_" + std::to_string(i);
		this->m_QueryTimeStart[i] = CGPUQueryManager::CreateQuery(tempName, CustomStruct::CRenderQueryDesc(CustomStruct::CRenderQueryType::QUERY_TIMESTAMP));
		tempName = this->m_Name + "_End_" + std::to_string(i);
		this->m_QueryTimeEnd[i] = CGPUQueryManager::CreateQuery(tempName, CustomStruct::CRenderQueryDesc(CustomStruct::CRenderQueryType::QUERY_TIMESTAMP));
		tempName = this->m_Name + "_Disjoint_" + std::to_string(i);
		this->m_QueryTimeDisjoint[i] = CGPUQueryManager::CreateQuery(tempName, CustomStruct::CRenderQueryDesc(CustomStruct::CRenderQueryType::QUERY_TIMESTAMP_DISJOINT));
		this->m_ReadBackSucceed[i] = FALSE;
		this->m_StartData[i] = 0u;
		this->m_EndData[i] = 0u;
		this->m_DisjointData[i].Reset();
	}
	this->m_AverageTime = static_cast<DOUBLE>(0);
	this->m_AlreadyInit = TRUE;
}
void CGPUProfilerManager::CGPUProfiler::ResetData()
{
	for (UINT i = 0; i < m_RecordFrameCount; i++)
	{
		this->m_ReadBackSucceed[i] = FALSE;
		this->m_StartData[i] = 0u;
		this->m_EndData[i] = 0u;
		this->m_DisjointData[i].Reset();
	}
	this->m_AverageTime = static_cast<DOUBLE>(0);
}
void CGPUProfilerManager::CGPUProfiler::Begin()
{
	this->m_AlreadyBegin = TRUE;
	this->ResetData();
	for (UINT i = 0; i < m_RecordFrameCount; i++)
	{
		CRenderDevice::Begin(this->m_QueryTimeStart[i]->GetQuery().Get());
		CRenderDevice::Begin(this->m_QueryTimeEnd[i]->GetQuery().Get());
	}
}
void CGPUProfilerManager::CGPUProfiler::Counter(const ULONGLONG& currentFrameIndex, std::function<void(void)> const& func)
{
	{
		CRenderDevice::Begin(this->m_QueryTimeDisjoint[currentFrameIndex % this->m_RecordFrameCount]->GetQuery().Get());
		CRenderDevice::End(this->m_QueryTimeStart[currentFrameIndex % this->m_RecordFrameCount]->GetQuery().Get());

		{
			func();
		}

		CRenderDevice::End(this->m_QueryTimeEnd[currentFrameIndex % this->m_RecordFrameCount]->GetQuery().Get());
		CRenderDevice::End(this->m_QueryTimeDisjoint[currentFrameIndex % this->m_RecordFrameCount]->GetQuery().Get());
	}

	{
		for (UINT i = 0u; i < this->m_RecordFrameCount; i++)
		{
			this->m_ReadBackSucceed[i] = FALSE;
		}

		{
			this->m_AverageTime = static_cast<DOUBLE>(0);
			INT totalNum = 0;
			ULONGLONG deltaTimeStamp, tempData; DOUBLE currentFrequency;
			CustomStruct::CRenderQueryTimestampDisjoint tempDisjoint;
			for (UINT i = 0u; i < this->m_RecordFrameCount; i++)
			{
				if (CRenderDevice::GetData(this->m_QueryTimeDisjoint[i]->GetQuery().Get(), &(tempDisjoint), sizeof(CustomStruct::CRenderQueryTimestampDisjoint)))
				{
					this->m_DisjointData[i] = tempDisjoint;
					if (!(this->m_DisjointData[i].Disjoint))
					{
						if (CRenderDevice::GetData(this->m_QueryTimeStart[i]->GetQuery().Get(), &(tempData), sizeof(ULONGLONG)))
						{
							this->m_StartData[i] = tempData;
							if (CRenderDevice::GetData(this->m_QueryTimeEnd[i]->GetQuery().Get(), &(tempData), sizeof(ULONGLONG)))
							{
								this->m_EndData[i] = tempData;

								this->m_ReadBackSucceed[i] = TRUE;
								deltaTimeStamp = this->m_EndData[i] - this->m_StartData[i];
								currentFrequency = static_cast<DOUBLE>(this->m_DisjointData[i].Frequency);
								this->m_AverageTime += (static_cast<DOUBLE>(deltaTimeStamp) / currentFrequency);
								totalNum += 1;
							}
						}
					}
				}
			}
			if (totalNum > 1)
			{
				this->m_AverageTime /= static_cast<DOUBLE>(totalNum);
			}
		}
	}
}
void CGPUProfilerManager::CGPUProfiler::End()
{
	this->m_AlreadyBegin = FALSE;
	for (UINT i = 0; i < m_RecordFrameCount; i++)
	{
		CRenderDevice::End(this->m_QueryTimeEnd[i]->GetQuery().Get());
		CRenderDevice::End(this->m_QueryTimeStart[i]->GetQuery().Get());
	}
}

CGPUProfilerManager* CGPUProfilerManager::m_GPUProfilerManager = new CGPUProfilerManager();

void CGPUProfilerManager::ShutDown()
{
	delete (CGPUProfilerManager::m_GPUProfilerManager);
}
void CGPUProfilerManager::AddPass(const std::string& name, const ULONGLONG& currentFrameIndex, std::function<void(void)> const& func, const UINT& recordFrameCount)
{
	shared_ptr<CGPUProfilerManager::CGPUProfiler> result = CGPUProfilerManager::FindProfiler(name);
	if (result == nullptr)
	{
		result = shared_ptr<CGPUProfilerManager::CGPUProfiler>(new CGPUProfilerManager::CGPUProfiler(name, recordFrameCount));
		CGPUProfilerManager::AddProfiler(name, result);
	}
	if (result->GetRecordFrameCount() != recordFrameCount || !(result->IsAlreadyInitialize()))
	{
		result->ResetAll(&recordFrameCount);
	}
	if (!(result->IsAlreadyBegin()))
	{
		result->Begin();
	}
	result->Counter(currentFrameIndex, func);
}
DOUBLE CGPUProfilerManager::GetPassAverageTime(const std::string& name)
{
	shared_ptr<CGPUProfilerManager::CGPUProfiler> result = CGPUProfilerManager::FindProfiler(name);
	if (result == nullptr || !(result->IsAlreadyInitialize()) || !(result->IsAlreadyBegin()))
	{
		return static_cast<DOUBLE>(0);
	}
	return (result->GetAverageTime());
}
void CGPUProfilerManager::ClearProfilers()
{
	CGPUProfilerManager::m_GPUProfilerManager->m_Profilers.clear();
}
shared_ptr<CGPUProfilerManager::CGPUProfiler> CGPUProfilerManager::NewProfiler(const std::string& name, const UINT& recordFrameCount)
{
	shared_ptr<CGPUProfilerManager::CGPUProfiler> result = CGPUProfilerManager::FindProfiler(name);
	if (result != nullptr)
	{
		if (result->IsAlreadyBegin() == TRUE)
		{
			result->End();
		}
		if (result->GetRecordFrameCount() != recordFrameCount || !(result->IsAlreadyInitialize()))
		{
			result->ResetAll(&recordFrameCount);
		}
		return result;
	}
	result = shared_ptr<CGPUProfilerManager::CGPUProfiler>(new CGPUProfilerManager::CGPUProfiler(name, recordFrameCount));
	CGPUProfilerManager::AddProfiler(name, result);
	result->ResetAll();
	return result;
}
void CGPUProfilerManager::AddProfiler(const std::string& name, std::shared_ptr<CGPUProfilerManager::CGPUProfiler> profiler)
{
	CGPUProfilerManager::m_GPUProfilerManager->m_Profilers[name] = profiler;
}
std::shared_ptr<CGPUProfilerManager::CGPUProfiler> CGPUProfilerManager::FindProfiler(const std::string& name)
{
	std::shared_ptr<CGPUProfilerManager::CGPUProfiler> result = nullptr;
	std::map<std::string, std::shared_ptr<CGPUProfilerManager::CGPUProfiler>>::iterator is = CGPUProfilerManager::m_GPUProfilerManager->m_Profilers.find(name);
	result = (is == CGPUProfilerManager::m_GPUProfilerManager->m_Profilers.end()) ? result : (is->second);
	return result;
}