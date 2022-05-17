#pragma once

#include "../../Headers/Game/CMeshManager.h"

CMeshManager* CMeshManager::m_MeshManager = new CMeshManager();

void CMeshManager::ClearMeshData()
{
	m_MeshManager->m_Data.clear();
}
shared_ptr<CMesh> CMeshManager::LoadCubeMesh(const std::string& name)
{

}
BOOL CMeshManager::AddMeshData(const std::string& name, const shared_ptr<CMesh>& mesh)
{

}
shared_ptr<CMesh> CMeshManager::FindMeshData(const std::string& name)
{
	std::map<std::string, shared_ptr<CMesh>>::iterator element = m_MeshManager->m_Data.find(name);
	if (element == m_MeshManager->m_Data.end())
		return NULL;
	return (element->second);
}