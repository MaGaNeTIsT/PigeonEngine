#pragma once

#include "../../../../Entry/EngineMain.h"
#include "../../../Sources/EngineRender/RenderBase/Headers/CRenderStructCommon.h"
#include "../../../Sources/EngineGame/Headers/CGameStructCommon.h"

class CassimpManager
{
public:
	static BOOL		ReadDefaultMeshFile(const std::string& path, std::vector<CustomStruct::CSubMeshInfo>& subMesh, UINT& vertexStride, CHAR*& vertices, UINT& numVertices, std::vector<UINT>& indices, UINT& numIndices);
	static BOOL		ReadSkeletonMeshAndBoneFile(const std::string& path, std::vector<CustomStruct::CSubMeshInfo>& subMesh, UINT& vertexStride, CHAR*& vertices, UINT& numVertices, std::vector<UINT>& indices, UINT& numIndices, std::vector<CustomStruct::CGameBoneNodeInfo>& skeleton, std::vector<UINT>& boneList, UINT& rootNode);
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