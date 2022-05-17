#pragma once

#include "../../Entry/MyMain.h"

class CMesh;

class CMeshManager
{
public:
	static const CMeshManager* const GetMeshManager() { return m_MeshManager; }
	static void					Uninit() { delete m_MeshManager; }
public:
	static void					ClearMeshData();
	static shared_ptr<CMesh>	LoadCubeMesh(const std::string& name);
private:
	static BOOL					AddMeshData(const std::string& name, const shared_ptr<CMesh>& mesh);
	static shared_ptr<CMesh>	FindMeshData(const std::string& name);
private:
	std::map<std::string, shared_ptr<CMesh>> m_Data;
private:
	CMeshManager() {}
	CMeshManager(const CMeshManager&) {}
	~CMeshManager() {}
private:
	static CMeshManager* m_MeshManager;
};