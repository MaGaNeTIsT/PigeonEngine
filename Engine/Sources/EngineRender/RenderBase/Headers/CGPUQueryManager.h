#pragma once

#include "../../../../../Entry/EngineMain.h"
#include "./CStructCommon.h"

class CGPUQueryManager
{
public:
	static const CGPUQueryManager* const GetGPUQueryManager() { return m_GPUQueryManager; }
	static void ShutDown();
public:
	static void								ClearQueryData();
public:
	static std::shared_ptr<class CGPUQuery>	CreateQuery(const std::string& name, const CustomStruct::CRenderQueryDesc& desc);
private:
	static void								AddQueryData(const std::string& name, std::shared_ptr<class CGPUQuery> query);
	static std::shared_ptr<class CGPUQuery>	FindQueryData(const std::string& name);
private:
	std::map<std::string, std::shared_ptr<class CGPUQuery>> m_Data;
private:
	CGPUQueryManager() {}
	CGPUQueryManager(const CGPUQueryManager&) {}
	~CGPUQueryManager() {}
private:
	static CGPUQueryManager* m_GPUQueryManager;
};