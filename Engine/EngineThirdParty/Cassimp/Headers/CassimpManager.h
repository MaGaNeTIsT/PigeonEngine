#pragma once

#include "../../../../Entry/EngineMain.h"
#include "../../../Sources/EngineRender/RenderBase/Headers/CRenderStructCommon.h"
#include "../../../Sources/EngineGame/Headers/CGameStructCommon.h"

class CassimpManager
{
public:
	enum CassimpReadFileState
	{
		ASSIMP_READ_FILE_STATE_SUCCEED,
		ASSIMP_READ_FILE_STATE_FAILED,
		ASSIMP_READ_FILE_STATE_ERROR
	};
public:
	static CassimpReadFileState		ReadDefaultMeshFile(const std::string& path, std::vector<CustomStruct::CSubMeshInfo>& subMesh, UINT& vertexStride, CHAR*& vertices, UINT& numVertices, std::vector<UINT>& indices, UINT& numIndices);
	static CassimpReadFileState		ReadSkeletonMeshAndBoneFile(const std::string& path, std::vector<CustomStruct::CSubMeshInfo>& subMesh, UINT& vertexStride, CHAR*& vertices, UINT& numVertices, std::vector<UINT>& indices, UINT& numIndices, std::vector<CustomStruct::CGameBoneNodeInfo>& skeleton, std::vector<UINT>& boneList, UINT& rootNode);
	static CassimpReadFileState		ReadSkeletonAnimationFile(const std::string& path, std::map<std::string, std::map<std::string, CustomStruct::CGameAnimationInfo>>& animationDatas);
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