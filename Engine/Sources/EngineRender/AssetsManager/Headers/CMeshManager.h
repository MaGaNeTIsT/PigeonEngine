#pragma once

#include "../../../../../Entry/EngineMain.h"
#include "../../../EngineBase/Headers/CBaseType.h"
#include "../../RenderBase/Headers/CStructCommon.h"

class CMeshManager
{
public:
	static const CMeshManager* const GetMeshManager() { return m_MeshManager; }
	static void					ShutDown();
public:
	static void					ClearMeshData();
public:
	static class CMesh*			LoadMeshFromFile(const std::string& name, const BOOL& recalculateTangent = FALSE);
	static class CMesh*			LoadPlaneMesh(const CustomType::Vector2& length, const CustomType::Vector2Int& vertexCount, const CustomType::Vector2& uv);
	static class CMesh*			LoadCubeMesh();
	static class CMesh*			LoadPolygonMesh();
	static class CMesh*			LoadPolygon2DWithTangentMesh(const CustomType::Vector4Int& customSize = CustomType::Vector4Int(0, 0, ENGINE_SCREEN_WIDTH, ENGINE_SCREEN_HEIGHT));
	static class CMesh*			LoadPolygon2DWithInputLayoutMesh(const CustomType::Vector4Int& customSize = CustomType::Vector4Int(0, 0, ENGINE_SCREEN_WIDTH, ENGINE_SCREEN_HEIGHT));
private:
	static CustomType::Vector3	CalculateTangentForTriangle(const CustomType::Vector3& p0, const CustomType::Vector3& p1, const CustomType::Vector3& p2, const CustomType::Vector2& uv0, const CustomType::Vector2& uv1, const CustomType::Vector2& uv2);
	static class CMesh*			LoadOBJMesh(const std::string& name, const BOOL& recalculateTangent);
	template<typename vertexType, typename indexType>
	static class CMesh*			CreateMeshObject(const std::string& name, std::vector<vertexType>& vertexData, std::vector<indexType>& indexData, std::vector<CustomStruct::CSubMeshInfo> submeshData);
private:
	static void					AddMeshData(const std::string& name, class CMesh* mesh);
	static class CMesh*			FindMeshData(const std::string& name);
private:
	std::map<std::string, class CMesh*> m_Data;
private:
	CMeshManager() {}
	CMeshManager(const CMeshManager&) {}
	~CMeshManager() {}
private:
	static CMeshManager* m_MeshManager;
};