/*
#pragma once

#include "../../../../../Entry/EngineMain.h"
#include "../../../EngineBase/Headers/CBaseType.h"
#include "../../RenderBase/Headers/CRenderStructCommon.h"
#include "../../../EngineGame/Headers/CMeshComponent.h"

class CMeshManager
{
public:
	enum CEngineBaseModelType
	{
		ENGINE_BASE_NONE			= 0,
		ENGINE_BASE_NORMAL_CUBE		= 1,
		ENGINE_BASE_ROUNDED_CUBE	= 2,
		ENGINE_BASE_SMOOTH_SPHERE	= 3,
		ENGINE_BASE_UV_SPHERE		= 4,
		ENGINE_BASE_TORUS			= 5,
		ENGINE_BASE_PRISM			= 6,
		ENGINE_BASE_MATERIAL_SPHERE	= 7,
		ENGINE_BASE_MONKEY			= 8,
		ENGINE_BASE_COUNT
	};
public:
	static const CMeshManager* const	GetMeshManager() { return m_MeshManager; }
	static void							ShutDown();
public:
	static void							ClearMeshData();
public:
	static const CBaseMesh<UINT32>*		LoadDefaultMeshAsset(const std::string& path, const BOOL32& needVertexData = FALSE);
	static const CBaseMesh<UINT32>*		LoadSkeletonMeshAsset(const std::string& path, BOOL32& isOutputSkeleton, std::vector<CustomStruct::CGameBoneNodeInfo>& skeleton, std::map<std::string, SHORT>& boneIndexMap, std::vector<USHORT>& boneList, const BOOL32& needVertexData = FALSE);
	static const CBaseMesh<UINT32>*		LoadEngineBaseModel(CEngineBaseModelType baseType, const CustomStruct::CRenderInputLayoutDesc* inputLayoutDesc, const UINT32& inputLayoutNum, const BOOL32& needVertexData = FALSE);
	static const CBaseMesh<UINT32>*		LoadMeshFromFile(const std::string& name, const CustomStruct::CRenderInputLayoutDesc* inputLayoutDesc, const UINT32& inputLayoutNum, const BOOL32& needVertexData = FALSE);
	static const CBaseMesh<UINT32>*		LoadPlaneMesh(const CustomType::Vector2& length, const CustomType::Vector2Int& vertexCount, const CustomType::Vector2& uv, const CustomStruct::CRenderInputLayoutDesc* inputLayoutDesc, const UINT32& inputLayoutNum, const BOOL32& needVertexData = FALSE);
	static const CBaseMesh<UINT32>*		LoadPolygonMesh(const CustomStruct::CRenderInputLayoutDesc* inputLayoutDesc, const UINT32& inputLayoutNum, const BOOL32& needVertexData = FALSE);
	static const CBaseMesh<UINT32>*		LoadPolygon2D(const CustomType::Vector4Int& customSize, const CustomStruct::CRenderInputLayoutDesc* inputLayoutDesc, const UINT32& inputLayoutNum, const BOOL32& needVertexData = FALSE);
	static const CBaseMesh<UINT32>*		LoadCubeMesh(const CustomStruct::CRenderInputLayoutDesc* inputLayoutDesc, const UINT32& inputLayoutNum, const BOOL32& needVertexData = FALSE);
private:
	static CustomType::Vector3			CalculateTangentForTriangle(const CustomType::Vector3& p0, const CustomType::Vector3& p1, const CustomType::Vector3& p2, const CustomType::Vector2& uv0, const CustomType::Vector2& uv1, const CustomType::Vector2& uv2);
	static void							CalculateBoundMinMax(CustomType::Vector3& boundMin, CustomType::Vector3& boundMax, const CustomStruct::CRenderInputLayoutDesc* inputLayoutDesc, const UINT32& inputLayoutNum, void* vertexData, const UINT32& vertexNum);
	template<typename IndexType>
	static void							CalculateTangentForMesh(const std::vector<CustomStruct::CSubMeshInfo>& submesh, const std::vector<IndexType>& indices, void* vertices, const UINT32& vertexNum, const UINT32& vertexStride, const UINT32& offsetPosition, const UINT32& offsetTexcoord, const UINT32& offsetTangent);
	static void							CombineForVertexData(void* vertices, const UINT32& vertexNum, const UINT32& vertexStride, const CustomStruct::CRenderInputLayoutDesc* inputLayoutDesc, const UINT32& inputLayoutNum, const FLOAT* position, const FLOAT* normal, const FLOAT* tangent, const FLOAT* texcoord, const FLOAT* color);
	static CBaseMesh<UINT32>*				ImportOBJMesh(const std::string& name, const CustomStruct::CRenderInputLayoutDesc* inputLayoutDesc, const UINT32& inputLayoutNum, const BOOL32& needVertexData);
	static CBaseMesh<UINT32>*				ImportAssetDefaultMesh(const std::string& name, const BOOL32& needVertexData);
	static CBaseMesh<UINT32>*				ImportAssetSkeletonMesh(const std::string& name, std::vector<CustomStruct::CGameBoneNodeInfo>& skeleton, std::map<std::string, SHORT>& boneIndexMap, std::vector<USHORT>& boneList, const BOOL32& needVertexData);
	static std::string					TranslateInputLayoutDesc(const CustomStruct::CRenderInputLayoutDesc* inputLayoutDesc, const UINT32& inputLayoutNum);
	template<typename IndexType>
	static CBaseMesh<IndexType>*		CreateMeshObject(const std::string& name, const CustomStruct::CRenderInputLayoutDesc* inputLayoutDesc, const UINT32& inputLayoutNum, void* vertexData, const UINT32& vertexNum, std::vector<IndexType>& indexData, const std::vector<CustomStruct::CSubMeshInfo>& submeshData, const BOOL32& needVertexData);
private:
	static void							AddMeshData(const std::string& name, CBaseMesh<UINT32>* mesh);
	static const CBaseMesh<UINT32>*		FindMeshData(const std::string& name);
private:
	std::map<std::string, CBaseMesh<UINT32>*>			m_Data;
private:
	CMeshManager() {}
	CMeshManager(const CMeshManager&) {}
	~CMeshManager() {}
private:
	static CMeshManager* m_MeshManager;
};
*/