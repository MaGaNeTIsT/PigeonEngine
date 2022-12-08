#pragma once

#include "../../../../Entry/EngineMain.h"
#include "../../../Sources/EngineRender/RenderBase/Headers/CRenderStructCommon.h"
#include "../../../Sources/EngineRender/AssetsManager/Headers/CMeshComponent.h"
#include "../../../Sources/EngineRender/AssetsManager/Headers/CMeshManager.h"

class CassimpManager
{
public:
	static BOOL		ReadDefaultMeshFile(const std::string& path, CHAR*& meshVertices, UINT& numVertices, UINT*& meshIndices, UINT& numIndices);
public:
	static void		Initialize();
	static void		ShutDown();
private:
	template<typename T>
	static BOOL		TranslateMeshData(const T* sceneMesh, const CustomStruct::CRenderInputLayoutDesc* inputLayoutDesc, const UINT& inputLayoutNum, CHAR*& verticesData, UINT& numVertices, UINT*& indicesData, UINT& numIndices);
private:
	CassimpManager() {}
	CassimpManager(const CassimpManager&) {}
	~CassimpManager() {}
private:
	static CassimpManager* m_AssimpManager;
};