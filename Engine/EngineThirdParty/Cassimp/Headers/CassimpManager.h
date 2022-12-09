#pragma once

#include "../../../../Entry/EngineMain.h"
#include "../../../Sources/EngineRender/RenderBase/Headers/CRenderStructCommon.h"
#include "../../../Sources/EngineRender/AssetsManager/Headers/CMeshComponent.h"
#include "../../../Sources/EngineRender/AssetsManager/Headers/CMeshManager.h"

class CassimpManager
{
public:
	static BOOL		ReadDefaultMeshFile(const std::string& path, CHAR*& meshVertices, UINT& numVertices, std::vector<UINT>& meshIndices, UINT& numIndices);
public:
	static void		Initialize();
	static void		ShutDown();
private:
	CassimpManager() {}
	CassimpManager(const CassimpManager&) {}
	~CassimpManager() {}
private:
	static CassimpManager* m_AssimpManager;
};