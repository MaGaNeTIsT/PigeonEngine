#pragma once

#include "../../../../Entry/EngineMain.h"
#include "../../../Sources/EngineRender/RenderBase/Headers/CRenderStructCommon.h"

class CassimpManager
{
public:
	static BOOL		ReadDefaultMeshFile(const std::string& path, std::vector<CustomStruct::CSubMeshInfo>& subMesh, UINT& vertexStride, CHAR*& vertices, UINT& numVertices, std::vector<UINT>& indices, UINT& numIndices);
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