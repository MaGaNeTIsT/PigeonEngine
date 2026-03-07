/*
#include "../Headers/CGPUQueryManager.h"
#include "../Headers/CRenderDevice.h"
#include "../Headers/CGPUQuery.h"

CGPUQueryManager* CGPUQueryManager::m_GPUQueryManager = new CGPUQueryManager();
void CGPUQueryManager::ShutDown()
{
	delete m_GPUQueryManager;
}
void CGPUQueryManager::ClearQueryData()
{
	CGPUQueryManager::m_GPUQueryManager->m_Data.clear();
}
std::shared_ptr<CGPUQuery> CGPUQueryManager::CreateQuery(const std::string& name, const CustomStruct::CRenderQueryDesc& desc)
{
	std::shared_ptr<CGPUQuery> result = CGPUQueryManager::FindQueryData(name);
	if (result != nullptr)
		return result;
	Microsoft::WRL::ComPtr<ID3D11Query> query;
	if (CRenderDevice::CreateQuery(query, desc))
	{
		result = std::shared_ptr<CGPUQuery>(new CGPUQuery(name, desc, query));
		CGPUQueryManager::AddQueryData(name, result);
		return result;
	}
	return nullptr;
}
void CGPUQueryManager::AddQueryData(const std::string& name, std::shared_ptr<CGPUQuery> query)
{
	CGPUQueryManager::m_GPUQueryManager->m_Data[name] = query;
}
std::shared_ptr<CGPUQuery> CGPUQueryManager::FindQueryData(const std::string& name)
{
	std::map<std::string, std::shared_ptr<CGPUQuery>>::iterator element = CGPUQueryManager::m_GPUQueryManager->m_Data.find(name);
	if (element == CGPUQueryManager::m_GPUQueryManager->m_Data.end())
		return nullptr;
	return (element->second);
}
*/