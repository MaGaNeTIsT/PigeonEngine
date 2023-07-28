/*
#include "CMeshManager.h"
#include "../../RenderBase/Headers/CRenderDevice.h"
#include "../../../../EngineThirdParty/Cassimp/Headers/CassimpManager.h"

CMeshManager* CMeshManager::m_MeshManager = new CMeshManager();
void CMeshManager::ShutDown()
{
	CMeshManager::ClearMeshData();
	delete m_MeshManager;
}
void CMeshManager::ClearMeshData()
{
	if (m_MeshManager->m_Data.size() > 0u)
	{
		for (auto& meshData : m_MeshManager->m_Data)
		{
			if (meshData.second != NULL)
			{
				delete (meshData.second);
			}
		}
		m_MeshManager->m_Data.clear();
	}
}
const CBaseMesh<UINT>* CMeshManager::LoadDefaultMeshAsset(const std::string& path, const BOOL& needVertexData)
{
	const CustomStruct::CRenderInputLayoutDesc* inputLayoutDesc; UINT inputLayoutNum;
	CustomStruct::CRenderInputLayoutDesc::GetEngineDefaultMeshInputLayouts(inputLayoutDesc, inputLayoutNum);
	std::string descName = CMeshManager::TranslateInputLayoutDesc(inputLayoutDesc, inputLayoutNum);
	{
		const CBaseMesh<UINT>* findResult = CMeshManager::FindMeshData(path + descName);
		if (findResult != NULL)
		{
			return findResult;
		}
	}

	{
		CBaseMesh<UINT>* resultMesh = CMeshManager::ImportAssetDefaultMesh(path, needVertexData);
		if (resultMesh == NULL)
		{
			return NULL;
		}
		CMeshManager::AddMeshData((path + descName), resultMesh);
		return resultMesh;
	}
}
const CBaseMesh<UINT>* CMeshManager::LoadSkeletonMeshAsset(const std::string& path, BOOL& isOutputSkeleton, std::vector<CustomStruct::CGameBoneNodeInfo>& skeleton, std::map<std::string, SHORT>& boneIndexMap, std::vector<USHORT>& boneList, const BOOL& needVertexData)
{
	isOutputSkeleton = FALSE;
	const CustomStruct::CRenderInputLayoutDesc* inputLayoutDesc; UINT inputLayoutNum;
	CustomStruct::CRenderInputLayoutDesc::GetEngineSkeletonMeshInputLayouts(inputLayoutDesc, inputLayoutNum);
	std::string descName = CMeshManager::TranslateInputLayoutDesc(inputLayoutDesc, inputLayoutNum);
	{
		const CBaseMesh<UINT>* findResult = CMeshManager::FindMeshData(path + descName);
		if (findResult != NULL)
		{
			return findResult;
		}
	}

	{
		CBaseMesh<UINT>* resultMesh = CMeshManager::ImportAssetSkeletonMesh(path, skeleton, boneIndexMap, boneList, needVertexData);
		if (resultMesh == NULL)
		{
			return NULL;
		}
		CMeshManager::AddMeshData((path + descName), resultMesh);
		isOutputSkeleton = TRUE;
		return resultMesh;
	}
}
const CBaseMesh<UINT>* CMeshManager::LoadEngineBaseModel(CEngineBaseModelType baseType, const CustomStruct::CRenderInputLayoutDesc* inputLayoutDesc, const UINT& inputLayoutNum, const BOOL& needVertexData)
{
	if (baseType == CEngineBaseModelType::ENGINE_BASE_NONE || baseType == CEngineBaseModelType::ENGINE_BASE_COUNT)
	{
		return NULL;
	}

	static std::map<CEngineBaseModelType, std::string> baseModelMap = {
		{ CEngineBaseModelType::ENGINE_BASE_NORMAL_CUBE, ENGINE_MESH_NORMAL_CUBE_NAME },
		{ CEngineBaseModelType::ENGINE_BASE_ROUNDED_CUBE, ENGINE_MESH_ROUNDED_CUBE_NAME },
		{ CEngineBaseModelType::ENGINE_BASE_SMOOTH_SPHERE, ENGINE_MESH_SMOOTH_SPHERE_NAME },
		{ CEngineBaseModelType::ENGINE_BASE_UV_SPHERE, ENGINE_MESH_UV_SPHERE_NAME },
		{ CEngineBaseModelType::ENGINE_BASE_TORUS, ENGINE_MESH_TORUS_NAME },
		{ CEngineBaseModelType::ENGINE_BASE_PRISM, ENGINE_MESH_PRISM_NAME },
		{ CEngineBaseModelType::ENGINE_BASE_MATERIAL_SPHERE, ENGINE_MESH_MATERIAL_SPHERE_NAME },
		{ CEngineBaseModelType::ENGINE_BASE_MONKEY, ENGINE_MESH_MONKEY_NAME } };

	std::string baseName = baseModelMap[baseType];
	std::string tempName = baseName + CMeshManager::TranslateInputLayoutDesc(inputLayoutDesc, inputLayoutNum);
	{
		const CBaseMesh<UINT>* findResult = CMeshManager::FindMeshData(tempName);
		if (findResult != NULL)
		{
			return findResult;
		}
	}
	//CBaseMesh<UINT>* resultMesh = CMeshManager::LoadOBJMesh(baseName, inputLayoutDesc, inputLayoutNum, needVertexData);
	CBaseMesh<UINT>* resultMesh = CMeshManager::ImportAssetDefaultMesh(baseName, needVertexData);
	if (resultMesh == NULL)
	{
		return NULL;
	}
	CMeshManager::AddMeshData(tempName, resultMesh);
	return resultMesh;
}
const CBaseMesh<UINT>* CMeshManager::LoadMeshFromFile(const std::string& name, const CustomStruct::CRenderInputLayoutDesc* inputLayoutDesc, const UINT& inputLayoutNum, const BOOL& needVertexData)
{
	std::string descName = CMeshManager::TranslateInputLayoutDesc(inputLayoutDesc, inputLayoutNum);
	{
		const CBaseMesh<UINT>* findResult = CMeshManager::FindMeshData(name + descName);
		if (findResult != NULL)
		{
			return findResult;
		}
	}

	{
		const static std::string _objName = "obj";

		CBaseMesh<UINT>* resultMesh = NULL;
		size_t pos = name.find_last_of('.');
		if (pos == std::string::npos)
		{
			return NULL;
		}
		std::string typeName = name.substr(pos + 1, name.length());

		if (typeName == _objName)
		{
			resultMesh = (CMeshManager::ImportOBJMesh(name, inputLayoutDesc, inputLayoutNum, needVertexData));
		}
		else
		{
			return NULL;
		}
		if (resultMesh == NULL)
		{
			return NULL;
		}

		CMeshManager::AddMeshData((name + descName), resultMesh);
		return resultMesh;
	}
}
const CBaseMesh<UINT>* CMeshManager::LoadPlaneMesh(const CustomType::Vector2& length, const CustomType::Vector2Int& vertexCount, const CustomType::Vector2& uv, const CustomStruct::CRenderInputLayoutDesc* inputLayoutDesc, const UINT& inputLayoutNum, const BOOL& needVertexData)
{
	if (inputLayoutNum < 1u || inputLayoutDesc == NULL)
	{
		return NULL;
	}
	if (length.X() <= 0.f || length.Y() <= 0.f || vertexCount.X() < 2 || vertexCount.Y() < 2)
	{
		return NULL;
	}
	if (CustomType::CMath::Abs(uv.X()) < 0.01f || CustomType::CMath::Abs(uv.Y()) < 0.01f)
	{
		return NULL;
	}
	std::string tempName = ENGINE_MESH_PLANE_NAME;
	tempName = tempName +
		"_cx_" + std::to_string(vertexCount.X()) +
		"_cz_" + std::to_string(vertexCount.Y()) +
		"_lx_" + std::to_string(length.X()) +
		"_lz_" + std::to_string(length.Y()) +
		"_u_" + std::to_string(uv.X()) +
		"_v_" + std::to_string(uv.Y());
	tempName += CMeshManager::TranslateInputLayoutDesc(inputLayoutDesc, inputLayoutNum);
	{
		const CBaseMesh<UINT>* findResult = CMeshManager::FindMeshData(tempName);
		if (findResult != NULL)
		{
			return findResult;
		}
	}
	void* vertices = nullptr;
	UINT vertexNum = 0u;
	{
		vertexNum = static_cast<UINT>(vertexCount.X() * vertexCount.Y());
		UINT vertexStride = 0u;
		for (UINT i = 0u; i < inputLayoutNum; i++)
		{
			vertexStride += inputLayoutDesc[i].GetSemanticSizeByByte();
		}
		vertices = new CHAR[vertexStride * vertexNum];
		//
		//                  Y
		//                  ^
		//                  |     Z
		//                  |    /
		//             0----|---/------1
		//            /     |  /      /
		//           /      | /      /
		//          /       0---------------->X
		//         /               /
		//        /               /
		//       2---------------3
		//
		const static FLOAT normal[4u]	= { 0.f, 1.f, 0.f, 0.f };
		const static FLOAT tangent[4u]	= { 1.f, 0.f, 0.f, 0.f };
		const static FLOAT color[4u]	= { 0.5f, 0.5f, 0.5f, 1.f };
		FLOAT* position = new FLOAT[4u * vertexNum];
		FLOAT* texcoord = new FLOAT[2u * vertexNum];
		FLOAT tx = 0.f, tz = 0.f, px = 0.f, pz = 0.f, u = 0.f, v = 0.f;
		for (INT z = 0; z < vertexCount.Y(); z++)
		{
			for (INT x = 0; x < vertexCount.X(); x++)
			{
				tx = static_cast<FLOAT>(x) / static_cast<FLOAT>(vertexCount.X() - 1);
				px = CustomType::CMath::Lerp(length.X() * (-0.5f), length.X() * 0.5f, tx);
				u = CustomType::CMath::Lerp(CustomType::CMath::Min(0.f, uv.X()), CustomType::CMath::Max(0.f, uv.X()), tx);

				tz = static_cast<FLOAT>(z) / static_cast<FLOAT>(vertexCount.Y() - 1);
				pz = CustomType::CMath::Lerp(length.Y() * 0.5f, length.Y() * (-0.5f), tz);
				v = CustomType::CMath::Lerp(CustomType::CMath::Min(0.f, uv.Y()), CustomType::CMath::Max(0.f, uv.Y()), tz);

				position[(z * vertexCount.X() + x) * 4 + 0] = px;
				position[(z * vertexCount.X() + x) * 4 + 1] = 0.f;
				position[(z * vertexCount.X() + x) * 4 + 2] = pz;
				position[(z * vertexCount.X() + x) * 4 + 3] = 1.f;
				texcoord[(z * vertexCount.X() + x) * 2 + 0] = u;
				texcoord[(z * vertexCount.X() + x) * 2 + 1] = v;
			}
		}
		for (UINT vertexIndex = 0u; vertexIndex < vertexNum; vertexIndex++)
		{
			CHAR* tempVertex = &((static_cast<CHAR*>(vertices))[vertexIndex * vertexStride]);
			for (UINT layoutIndex = 0u; layoutIndex < inputLayoutNum; layoutIndex++)
			{
				if (inputLayoutDesc[layoutIndex].SemanticName == CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_POSITION)
				{
					FLOAT* tempPosition = (FLOAT*)tempVertex;
					tempPosition[0u] = position[vertexIndex * 4u + 0u];
					tempPosition[1u] = position[vertexIndex * 4u + 1u];
					tempPosition[2u] = position[vertexIndex * 4u + 2u];
					tempPosition[3u] = position[vertexIndex * 4u + 3u];
					tempVertex = &(tempVertex[16]);
				}
				else if (inputLayoutDesc[layoutIndex].SemanticName == CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_TEXCOORD)
				{
					FLOAT* tempTexcoord = (FLOAT*)tempVertex;
					tempTexcoord[0u] = texcoord[vertexIndex * 2u + 0u];
					tempTexcoord[1u] = texcoord[vertexIndex * 2u + 1u];
					tempVertex = &(tempVertex[8]);
				}
				else if (inputLayoutDesc[layoutIndex].SemanticName == CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_NORMAL)
				{
					FLOAT* tempNormal = (FLOAT*)tempVertex;
					tempNormal[0u] = normal[0u];
					tempNormal[1u] = normal[1u];
					tempNormal[2u] = normal[2u];
					tempNormal[3u] = normal[3u];
					tempVertex = &(tempVertex[16]);
				}
				else if (inputLayoutDesc[layoutIndex].SemanticName == CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_TANGENT)
				{
					FLOAT* tempTangent = (FLOAT*)tempVertex;
					tempTangent[0u] = tangent[0u];
					tempTangent[1u] = tangent[1u];
					tempTangent[2u] = tangent[2u];
					tempTangent[3u] = tangent[3u];
					tempVertex = &(tempVertex[16]);
				}
				else if (inputLayoutDesc[layoutIndex].SemanticName == CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_COLOR)
				{
					FLOAT* tempColor = (FLOAT*)tempVertex;
					tempColor[0u] = color[0u];
					tempColor[1u] = color[1u];
					tempColor[2u] = color[2u];
					tempColor[3u] = color[3u];
					tempVertex = &(tempVertex[16]);
				}
			}
		}
		delete[]position;
		delete[]texcoord;
	}
	std::vector<UINT> index;
	{
		index.resize((vertexCount.X() - 1) * (vertexCount.Y() - 1) * 6);
		for (INT z = 0; z < (vertexCount.Y() - 1); z++)
		{
			for (INT x = 0; x < (vertexCount.X() - 1); x++)
			{
				index[(z * (vertexCount.X() - 1) + x) * 6 + 0] = (z + 0) * vertexCount.X() + (x + 0);
				index[(z * (vertexCount.X() - 1) + x) * 6 + 1] = (z + 0) * vertexCount.X() + (x + 1);
				index[(z * (vertexCount.X() - 1) + x) * 6 + 2] = (z + 1) * vertexCount.X() + (x + 0);

				index[(z * (vertexCount.X() - 1) + x) * 6 + 3] = (z + 0) * vertexCount.X() + (x + 1);
				index[(z * (vertexCount.X() - 1) + x) * 6 + 4] = (z + 1) * vertexCount.X() + (x + 1);
				index[(z * (vertexCount.X() - 1) + x) * 6 + 5] = (z + 1) * vertexCount.X() + (x + 0);
			}
		}
	}
	std::vector<CustomStruct::CSubMeshInfo> submesh(0);
	CBaseMesh<UINT>* resultMesh = CMeshManager::CreateMeshObject<UINT>(tempName, inputLayoutDesc, inputLayoutNum, vertices, vertexNum, index, submesh, needVertexData);
	CMeshManager::AddMeshData(tempName, resultMesh);
	return resultMesh;
}
const CBaseMesh<UINT>* CMeshManager::LoadPolygonMesh(const CustomStruct::CRenderInputLayoutDesc* inputLayoutDesc, const UINT& inputLayoutNum, const BOOL& needVertexData)
{
	std::string tempName = ENGINE_MESH_POLYGON_NAME;
	tempName += CMeshManager::TranslateInputLayoutDesc(inputLayoutDesc, inputLayoutNum);
	{
		const CBaseMesh<UINT>* findResult = CMeshManager::FindMeshData(tempName);
		if (findResult != NULL)
		{
			return findResult;
		}
	}
	void* vertices = nullptr;
	UINT vertexNum = 4u;
	{
		//
		//          Y
		//  (0, 0)  ^
		//     0----|---1
		//     |    |  /|
		//     | (1)| /	|
		//     |    |/	|
		// ---------0--------->X
		//     |   /    |
		//     |  /     |
		//     | /  (2) |
		//     |/       |
		//     2--------3
		//            (1, 1)
		//
		const static FLOAT position[4u * 4u] = {
			-0.5f, 0.5f, 0.f, 1.f, 0.5f, 0.5f, 0.f, 1.f,
			-0.5f, -0.5f, 0.f, 1.f, 0.5f, -0.5f, 0.f, 1.f };
		const static FLOAT normal[4u * 4u] = {
			0.f, 0.f, -1.f, 0.f, 0.f, 0.f, -1.f, 0.f,
			0.f, 0.f, -1.f, 0.f, 0.f, 0.f, -1.f, 0.f };
		const static FLOAT tangent[4u * 4u] = {
			1.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
			1.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f };
		const static FLOAT texcoord[4u * 2u] = { 0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 1.f, 1.f };
		UINT vertexStride = 0u;
		for (UINT i = 0u; i < inputLayoutNum; i++)
		{
			vertexStride += inputLayoutDesc[i].GetSemanticSizeByByte();
		}
		vertices = new CHAR[vertexStride * vertexNum];
		CMeshManager::CombineForVertexData(vertices, vertexNum, vertexStride, inputLayoutDesc,
			inputLayoutNum, position, normal, tangent, texcoord, NULL);
	}
	std::vector<UINT> index;
	{
		index.resize(6);
		index[0] = 0; index[1] = 1; index[2] = 2;
		index[3] = 1; index[4] = 3; index[5] = 2;
	}
	std::vector<CustomStruct::CSubMeshInfo> submesh(0);
	CBaseMesh<UINT>* resultMesh = CMeshManager::CreateMeshObject<UINT>(tempName, inputLayoutDesc, inputLayoutNum, vertices, vertexNum, index, submesh, needVertexData);
	CMeshManager::AddMeshData(tempName, resultMesh);
	return resultMesh;
}
const CBaseMesh<UINT>* CMeshManager::LoadPolygon2D(const CustomType::Vector4Int& customSize, const CustomStruct::CRenderInputLayoutDesc* inputLayoutDesc, const UINT& inputLayoutNum, const BOOL& needVertexData)
{
	std::string tempName = ENGINE_MESH_POLYGON_2D_NAME;
	tempName = tempName +
		"_x_" + std::to_string(customSize.X()) +
		"_y_" + std::to_string(customSize.Y()) +
		"_z_" + std::to_string(customSize.Z()) +
		"_w_" + std::to_string(customSize.W());
	tempName += CMeshManager::TranslateInputLayoutDesc(inputLayoutDesc, inputLayoutNum);
	{
		const CBaseMesh<UINT>* findResult = CMeshManager::FindMeshData(tempName);
		if (findResult != NULL)
		{
			return findResult;
		}
	}
	void* vertices = nullptr;
	UINT vertexNum = 4u;
	{
		//
		//  (x, y)
		//     0--------1
		//     |       /|
		//     | (1)  /	|
		//     |     /	|
		//     |    /	|
		//     |   /    |
		//     |  /     |
		//     | /  (2) |
		//     |/       |
		//     2--------3
		//            (z, w)
		//
		const static FLOAT normal[4u * 4u] = {
			0.f, 0.f, -1.f, 0.f, 0.f, 0.f, -1.f, 0.f,
			0.f, 0.f, -1.f, 0.f, 0.f, 0.f, -1.f, 0.f };
		const static FLOAT tangent[4u * 4u] = {
			1.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
			1.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f };
		const static FLOAT texcoord[4u * 2u] = { 0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 1.f, 1.f };
		FLOAT position[4u * 4u] = {
			static_cast<FLOAT>(customSize.X()), static_cast<FLOAT>(customSize.Y()), 0.f, 1.f,
			static_cast<FLOAT>(customSize.Z()), static_cast<FLOAT>(customSize.Y()), 0.f, 1.f,
			static_cast<FLOAT>(customSize.X()), static_cast<FLOAT>(customSize.W()), 0.f, 1.f,
			static_cast<FLOAT>(customSize.Z()), static_cast<FLOAT>(customSize.W()), 0.f, 1.f };
		UINT vertexStride = 0u;
		for (UINT i = 0u; i < inputLayoutNum; i++)
		{
			vertexStride += inputLayoutDesc[i].GetSemanticSizeByByte();
		}
		vertices = new CHAR[vertexStride * vertexNum];
		CMeshManager::CombineForVertexData(vertices, vertexNum, vertexStride, inputLayoutDesc,
			inputLayoutNum, position, normal, tangent, texcoord, NULL);
	}
	std::vector<UINT> index;
	{
		index.resize(6);
		index[0] = 0; index[1] = 1; index[2] = 2;
		index[3] = 1; index[4] = 3; index[5] = 2;
	}
	std::vector<CustomStruct::CSubMeshInfo> submesh(0);
	CBaseMesh<UINT>* resultMesh = CMeshManager::CreateMeshObject<UINT>(tempName, inputLayoutDesc, inputLayoutNum, vertices, vertexNum, index, submesh, needVertexData);
	CMeshManager::AddMeshData(tempName, resultMesh);
	return resultMesh;
}
const CBaseMesh<UINT>* CMeshManager::LoadCubeMesh(const CustomStruct::CRenderInputLayoutDesc* inputLayoutDesc, const UINT& inputLayoutNum, const BOOL& needVertexData)
{
	std::string tempName = ENGINE_MESH_CUBE_NAME;
	tempName += CMeshManager::TranslateInputLayoutDesc(inputLayoutDesc, inputLayoutNum);
	{
		const CBaseMesh<UINT>* findResult = CMeshManager::FindMeshData(tempName);
		if (findResult != NULL)
		{
			return findResult;
		}
	}
	void* vertices = nullptr;
	UINT vertexNum = 24u;
	{
		//
		// Back->Right->Forward->Left->Top->Bottom
		//                  Y
		//                  ^
		//                  |
		//     0--------1   |    Z
		//     |        |   |   /
		//     |        |   |  /
		//     |        |   | /
		//     2--------3   0---------------->X
		//
		const static FLOAT position[24u * 4u] = {
			-0.5f, 0.5f, -0.5f, 1.f, 0.5f, 0.5f, -0.5f, 1.f,
			-0.5f, -0.5f, -0.5f, 1.f, 0.5f, -0.5f, -0.5f, 1.f,

			0.5f, 0.5f, -0.5f, 1.f, 0.5f, 0.5f, 0.5f, 1.f,
			0.5f, -0.5f, -0.5f, 1.f, 0.5f, -0.5f, 0.5f, 1.f,

			0.5f, 0.5f, 0.5f, 1.f, -0.5f, 0.5f, 0.5f, 1.f,
			0.5f, -0.5f, 0.5f, 1.f, -0.5f, -0.5f, 0.5f, 1.f,

			-0.5f, 0.5f, 0.5f, 1.f, -0.5f, 0.5f, -0.5f, 1.f,
			-0.5f, -0.5f, 0.5f, 1.f, -0.5f, -0.5f, -0.5f, 1.f,

			-0.5f, 0.5f, 0.5f, 1.f, 0.5f, 0.5f, 0.5f, 1.f,
			-0.5f, 0.5f, -0.5f, 1.f, 0.5f, 0.5f, -0.5f, 1.f,

			-0.5f, -0.5f, -0.5f, 1.f, 0.5f, -0.5f, -0.5f, 1.f,
			-0.5f, -0.5f, 0.5f, 1.f, 0.5f, -0.5f, 0.5f, 1.f };
		const static FLOAT normal[24u * 4u] = {
			0.f, 0.f, -1.f, 0.f, 0.f, 0.f, -1.f, 0.f,
			0.f, 0.f, -1.f, 0.f, 0.f, 0.f, -1.f, 0.f,

			1.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
			1.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,

			0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 1.f, 0.f,
			0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 1.f, 0.f,

			-1.f, 0.f, 0.f, 0.f, -1.f, 0.f, 0.f, 0.f,
			-1.f, 0.f, 0.f, 0.f, -1.f, 0.f, 0.f, 0.f,

			0.f, 1.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f,
			0.f, 1.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f,

			0.f, -1.f, 0.f, 0.f, 0.f, -1.f, 0.f, 0.f,
			0.f, -1.f, 0.f, 0.f, 0.f, -1.f, 0.f, 0.f };
		const static FLOAT tangent[24u * 4u] = {
			1.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
			1.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,

			0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 1.f, 0.f,
			0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 1.f, 0.f,

			-1.f, 0.f, 0.f, 0.f, -1.f, 0.f, 0.f, 0.f,
			-1.f, 0.f, 0.f, 0.f, -1.f, 0.f, 0.f, 0.f,

			0.f, 0.f, -1.f, 0.f, 0.f, 0.f, -1.f, 0.f,
			0.f, 0.f, -1.f, 0.f, 0.f, 0.f, -1.f, 0.f,

			1.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
			1.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,

			1.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f,
			1.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f };
		const static FLOAT texcoord[24u * 2u] = {
			0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 1.f, 1.f,
			0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 1.f, 1.f,
			0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 1.f, 1.f,
			0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 1.f, 1.f,
			0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 1.f, 1.f,
			0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 1.f, 1.f };
		UINT vertexStride = 0u;
		for (UINT i = 0u; i < inputLayoutNum; i++)
		{
			vertexStride += inputLayoutDesc[i].GetSemanticSizeByByte();
		}
		vertices = new CHAR[vertexStride * vertexNum];
		CMeshManager::CombineForVertexData(vertices, vertexNum, vertexStride, inputLayoutDesc,
			inputLayoutNum, position, normal, tangent, texcoord, NULL);
	}
	std::vector<UINT> index;
	{
		index.resize(36);
		for (UINT i = 0u; i < 6u; i++)
		{
			index[i * 6u + 0u] = i * 4u + 0u;
			index[i * 6u + 1u] = i * 4u + 1u;
			index[i * 6u + 2u] = i * 4u + 2u;

			index[i * 6u + 3u] = i * 4u + 1u;
			index[i * 6u + 4u] = i * 4u + 3u;
			index[i * 6u + 5u] = i * 4u + 2u;
		}
	}
	std::vector<CustomStruct::CSubMeshInfo> submesh(0);
	CBaseMesh<UINT>* resultMesh = CMeshManager::CreateMeshObject<UINT>(tempName, inputLayoutDesc, inputLayoutNum, vertices, vertexNum, index, submesh, needVertexData);
	CMeshManager::AddMeshData(tempName, resultMesh);
	return resultMesh;
}
CustomType::Vector3 CMeshManager::CalculateTangentForTriangle(const CustomType::Vector3& p0, const CustomType::Vector3& p1, const CustomType::Vector3& p2, const CustomType::Vector2& uv0, const CustomType::Vector2& uv1, const CustomType::Vector2& uv2)
{
	FLOAT scale;
	CustomType::Vector3 q1 = p1;
	CustomType::Vector3 q2 = p2;
	CustomType::Vector2 st1 = uv1;
	CustomType::Vector2 st2 = uv2;
	q1 = q1 - p0;
	q2 = q2 - p0;
	st1 = st1 - uv0;
	st2 = st2 - uv0;
	scale = (st1.X() * st2.Y()) - (st2.X() * st1.Y());
	if (CustomType::CMath::Abs(scale) < 1e-6f)
	{
		scale = scale > 0.f ? 1e-6f : (-1e-6f);
	}
	scale = 1.f / scale;
	return (CustomType::Vector3(
		st2.Y() * q1.X() + (-st1.Y()) * q2.X(),
		st2.Y() * q1.Y() + (-st1.Y()) * q2.Y(),
		st2.Y() * q1.Z() + (-st1.Y()) * q2.Z()) * scale);
}
void CMeshManager::CalculateBoundMinMax(CustomType::Vector3& boundMin, CustomType::Vector3& boundMax, const CustomStruct::CRenderInputLayoutDesc* inputLayoutDesc, const UINT& inputLayoutNum, void* vertexData, const UINT& vertexNum)
{
	BOOL havePosition = FALSE;
	UINT vertexStride = 0u;
	UINT offsetPosition = 0u;

	for (UINT i = 0u; i < inputLayoutNum; i++)
	{
		if (inputLayoutDesc[i].SemanticName == CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_POSITION)
		{
			havePosition = TRUE;
			offsetPosition = vertexStride;
		}
		vertexStride += inputLayoutDesc[i].GetSemanticSizeByByte();
	}

	if (havePosition)
	{
		FLOAT minmax[6] = { ENGINE_FLOAT32_MAX, ENGINE_FLOAT32_MAX, ENGINE_FLOAT32_MAX, -ENGINE_FLOAT32_MAX, -ENGINE_FLOAT32_MAX, -ENGINE_FLOAT32_MAX };
		auto compareMinMax = [&](const FLOAT* input) {
			minmax[0 * 3 + 0] = CustomType::CMath::Min(input[0u], minmax[0 * 3 + 0]);
			minmax[0 * 3 + 1] = CustomType::CMath::Min(input[1u], minmax[0 * 3 + 1]);
			minmax[0 * 3 + 2] = CustomType::CMath::Min(input[2u], minmax[0 * 3 + 2]);
			minmax[1 * 3 + 0] = CustomType::CMath::Max(input[0u], minmax[1 * 3 + 0]);
			minmax[1 * 3 + 1] = CustomType::CMath::Max(input[1u], minmax[1 * 3 + 1]);
			minmax[1 * 3 + 2] = CustomType::CMath::Max(input[2u], minmax[1 * 3 + 2]); };
		for (UINT i = 0u; i < vertexNum; i++)
		{
			const CHAR* tempVertex = &(((const CHAR*)(vertexData))[i * vertexStride]);
			const FLOAT* tempPosition = (const FLOAT*)(&(tempVertex[offsetPosition]));
			compareMinMax(tempPosition);
		}
		boundMin = CustomType::Vector3(minmax[0], minmax[1], minmax[2]);
		boundMax = CustomType::Vector3(minmax[3], minmax[4], minmax[5]);
	}
	else
	{
		boundMin = CustomType::Vector3::Zero();
		boundMax = CustomType::Vector3::Zero();
	}
}
template<typename IndexType>
void CMeshManager::CalculateTangentForMesh(const std::vector<CustomStruct::CSubMeshInfo>& submesh, const std::vector<IndexType>& indices, void* vertices, const UINT& vertexNum, const UINT& vertexStride, const UINT& offsetPosition, const UINT& offsetTexcoord, const UINT& offsetTangent)
{
	for (UINT i = 0u; i < vertexNum; i++)
	{
		CHAR* tempVertex = (&(((CHAR*)vertices)[i * vertexStride]));
		FLOAT* tempTangent = (FLOAT*)(&(tempVertex[offsetTangent]));
		tempTangent[0u] = 0.f;
		tempTangent[1u] = 0.f;
		tempTangent[2u] = 0.f;
		tempTangent[3u] = 0.f;
	}
	if (submesh.size() > 1)
	{
		CustomType::Vector3 p[3u];
		CustomType::Vector2 uv[3u];
		CustomType::Vector3 newT, tempT;
		UINT triangleNum, triangleIndex, vertexIndex;
		CHAR* tempVertex; FLOAT* tempPosition; FLOAT* tempTexcoord; FLOAT* tempTangent;
		for (UINT subIndex = 0u; subIndex < submesh.size(); subIndex++)
		{
			const CustomStruct::CSubMeshInfo& subInfo = submesh[subIndex];
			triangleNum = subInfo.IndexCount / 3u;
			for (UINT i = 0u; i < triangleNum; i++)
			{
				for (UINT j = 0u; j < 3u; j++)
				{
					triangleIndex = subInfo.IndexStart + i * 3u + j;
					vertexIndex = subInfo.VertexStart + static_cast<UINT>(indices[triangleIndex]);
					tempVertex = (&(((CHAR*)vertices)[vertexIndex * vertexStride]));
					tempPosition = (FLOAT*)(&(tempVertex[offsetPosition]));
					tempTexcoord = (FLOAT*)(&(tempVertex[offsetTexcoord]));
					p[j] = CustomType::Vector3(tempPosition[0u], tempPosition[1u], tempPosition[2u]);
					uv[j] = CustomType::Vector2(tempTexcoord[0u], tempTexcoord[1u]);
				}
				newT = CMeshManager::CalculateTangentForTriangle(p[0u], p[1u], p[2u], uv[0u], uv[1u], uv[2u]);
				for (UINT j = 0u; j < 3u; j++)
				{
					triangleIndex = subInfo.IndexStart + i * 3u + j;
					vertexIndex = subInfo.VertexStart + static_cast<UINT>(indices[triangleIndex]);
					tempVertex = (&(((CHAR*)vertices)[vertexIndex * vertexStride]));
					tempTangent = (FLOAT*)(&(tempVertex[offsetTangent]));
					tempT = CustomType::Vector3(tempTangent[0u], tempTangent[1u], tempTangent[2u]);
					tempT += newT;
					tempTangent[0u] = tempT.X();
					tempTangent[1u] = tempT.Y();
					tempTangent[2u] = tempT.Z();
					tempTangent[3u] = 0.f;
				}
			}
		}
	}
	else
	{
		CustomType::Vector3 p[3u];
		CustomType::Vector2 uv[3u];
		CustomType::Vector3 newT, tempT;
		UINT triangleIndex, vertexIndex;
		CHAR* tempVertex; FLOAT* tempPosition; FLOAT* tempTexcoord; FLOAT* tempTangent;
		UINT triangleNum = static_cast<UINT>(indices.size()) / 3u;
		for (UINT i = 0u; i < triangleNum; i++)
		{
			for (UINT j = 0u; j < 3u; j++)
			{
				triangleIndex = i * 3u + j;
				vertexIndex = static_cast<UINT>(indices[triangleIndex]);
				tempVertex = (&(((CHAR*)vertices)[vertexIndex * vertexStride]));
				tempPosition = (FLOAT*)(&(tempVertex[offsetPosition]));
				tempTexcoord = (FLOAT*)(&(tempVertex[offsetTexcoord]));
				p[j] = CustomType::Vector3(tempPosition[0u], tempPosition[1u], tempPosition[2u]);
				uv[j] = CustomType::Vector2(tempTexcoord[0u], tempTexcoord[1u]);
			}
			newT = CMeshManager::CalculateTangentForTriangle(p[0u], p[1u], p[2u], uv[0u], uv[1u], uv[2u]);
			for (UINT j = 0u; j < 3u; j++)
			{
				triangleIndex = i * 3u + j;
				vertexIndex = static_cast<UINT>(indices[triangleIndex]);
				tempVertex = (&(((CHAR*)vertices)[vertexIndex * vertexStride]));
				tempTangent = (FLOAT*)(&(tempVertex[offsetTangent]));
				tempT = CustomType::Vector3(tempTangent[0u], tempTangent[1u], tempTangent[2u]);
				tempT += newT;
				tempTangent[0u] = tempT.X();
				tempTangent[1u] = tempT.Y();
				tempTangent[2u] = tempT.Z();
				tempTangent[3u] = 0.f;
			}
		}
	}

	{
		auto swapValue = [](std::vector<UINT>& pool, const UINT& index0, const UINT& index1) {
			UINT tempV = pool[index0];
			pool[index0] = pool[index1];
			pool[index1] = tempV; };
		std::vector<UINT> tempVertexPool;
		UINT tempVertexPoolNum = vertexNum;
		tempVertexPool.resize(vertexNum);
		for (UINT i = 0u; i < vertexNum; i++)
		{
			tempVertexPool[i] = i;
		}
		while (tempVertexPoolNum > 0)
		{
			UINT tempCurrentIndex = tempVertexPool[0];

			CHAR* tempCurrentVertex = (&(((CHAR*)vertices)[tempCurrentIndex * vertexStride]));
			FLOAT* tempCurrentPosition = (FLOAT*)(&(tempCurrentVertex[offsetPosition]));

			FLOAT* tempCurrentTangent = (FLOAT*)(&(tempCurrentVertex[offsetTangent]));
			CustomType::Vector3 totalTangent(tempCurrentTangent[0], tempCurrentTangent[1], tempCurrentTangent[2]);

			std::vector<UINT> tempRemoveList;
			for (UINT indexVertexPool = 1u; indexVertexPool < tempVertexPoolNum; indexVertexPool++)
			{
				CHAR* tempPooledVertex = (&(((CHAR*)vertices)[(tempVertexPool[indexVertexPool]) * vertexStride]));
				FLOAT* tempPooledPosition = (FLOAT*)(&(tempPooledVertex[offsetPosition]));
				if (tempCurrentPosition[0] == tempPooledPosition[0] && tempCurrentPosition[1] == tempPooledPosition[1] && tempCurrentPosition[2] == tempPooledPosition[2])
				{
					FLOAT* tempPooledTangent = (FLOAT*)(&(tempPooledVertex[offsetTangent]));
					totalTangent += CustomType::Vector3(tempPooledTangent[0], tempPooledTangent[1], tempPooledTangent[2]);
					tempRemoveList.push_back(indexVertexPool);
				}
			}
			totalTangent.Normalize();
			tempCurrentTangent[0] = totalTangent.X();
			tempCurrentTangent[1] = totalTangent.Y();
			tempCurrentTangent[2] = totalTangent.Z();
			tempCurrentTangent[3] = 0.f;
			for (size_t indexRemoveList = 0u; indexRemoveList < tempRemoveList.size(); indexRemoveList++)
			{
				CHAR* tempPooledVertex = (&(((CHAR*)vertices)[(tempVertexPool[tempRemoveList[indexRemoveList]]) * vertexStride]));
				FLOAT* tempPooledTangent = (FLOAT*)(&(tempPooledVertex[offsetTangent]));
				tempPooledTangent[0] = totalTangent.X();
				tempPooledTangent[1] = totalTangent.Y();
				tempPooledTangent[2] = totalTangent.Z();
				tempPooledTangent[3] = 0.f;
			}
			for (size_t indexRemoveList = 0u; indexRemoveList < tempRemoveList.size(); indexRemoveList++)
			{
				if (tempVertexPoolNum > 0)
				{
					UINT tempRemoveIndex = tempRemoveList[indexRemoveList];
					UINT tempEndIndex = tempVertexPoolNum - 1;
					swapValue(tempVertexPool, tempRemoveIndex, tempEndIndex);
					tempVertexPoolNum = tempVertexPoolNum - 1;
				}
			}
			if (tempVertexPoolNum > 0)
			{
				UINT tempRemoveIndex = 0;
				UINT tempEndIndex = tempVertexPoolNum - 1;
				swapValue(tempVertexPool, tempRemoveIndex, tempEndIndex);
				tempVertexPoolNum = tempVertexPoolNum - 1;
			}
		}
	}
}
void CMeshManager::CombineForVertexData(void* vertices, const UINT& vertexNum, const UINT& vertexStride, const CustomStruct::CRenderInputLayoutDesc* inputLayoutDesc, const UINT& inputLayoutNum, const FLOAT* position, const FLOAT* normal, const FLOAT* tangent, const FLOAT* texcoord, const FLOAT* color)
{
	const static FLOAT defaultPosition[4] = { 0.f, 0.f, 0.f, 1.f };
	const static FLOAT defaultTexcoord[2] = { 0.f, 0.f };
	const static FLOAT defaultNormal[4] = { 0.f, 1.f, 0.f, 0.f };
	const static FLOAT defaultTangent[4] = { 1.f, 0.f, 0.f, 0.f };
	const static FLOAT defaultColor[4] = { 0.5f, 0.5f, 0.5f, 1.f };
	for (UINT vertexIndex = 0u; vertexIndex < vertexNum; vertexIndex++)
	{
		CHAR* tempVertex = &((static_cast<CHAR*>(vertices))[vertexIndex * vertexStride]);
		for (UINT layoutIndex = 0u; layoutIndex < inputLayoutNum; layoutIndex++)
		{
			if (inputLayoutDesc[layoutIndex].SemanticName == CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_POSITION)
			{
				FLOAT* tempPosition = (FLOAT*)tempVertex;
				if (position != NULL)
				{
					tempPosition[0u] = position[vertexIndex * 4u + 0u];
					tempPosition[1u] = position[vertexIndex * 4u + 1u];
					tempPosition[2u] = position[vertexIndex * 4u + 2u];
					tempPosition[3u] = position[vertexIndex * 4u + 3u];
				}
				else
				{
					tempPosition[0u] = defaultPosition[0u];
					tempPosition[1u] = defaultPosition[1u];
					tempPosition[2u] = defaultPosition[2u];
					tempPosition[3u] = defaultPosition[3u];
				}
				tempVertex = &(tempVertex[16]);
			}
			else if (inputLayoutDesc[layoutIndex].SemanticName == CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_TEXCOORD)
			{
				FLOAT* tempTexcoord = (FLOAT*)tempVertex;
				if (texcoord != NULL)
				{
					tempTexcoord[0u] = texcoord[vertexIndex * 2u + 0u];
					tempTexcoord[1u] = texcoord[vertexIndex * 2u + 1u];
				}
				else
				{
					tempTexcoord[0u] = defaultTexcoord[0u];
					tempTexcoord[1u] = defaultTexcoord[1u];
				}
				tempVertex = &(tempVertex[8]);
			}
			else if (inputLayoutDesc[layoutIndex].SemanticName == CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_NORMAL)
			{
				FLOAT* tempNormal = (FLOAT*)tempVertex;
				if (normal != NULL)
				{
					tempNormal[0u] = normal[vertexIndex * 4u + 0u];
					tempNormal[1u] = normal[vertexIndex * 4u + 1u];
					tempNormal[2u] = normal[vertexIndex * 4u + 2u];
					tempNormal[3u] = normal[vertexIndex * 4u + 3u];
				}
				else
				{
					tempNormal[0u] = defaultNormal[0u];
					tempNormal[1u] = defaultNormal[1u];
					tempNormal[2u] = defaultNormal[2u];
					tempNormal[3u] = defaultNormal[3u];
				}
				tempVertex = &(tempVertex[16]);
			}
			else if (inputLayoutDesc[layoutIndex].SemanticName == CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_TANGENT)
			{
				FLOAT* tempTangent = (FLOAT*)tempVertex;
				if (tangent != NULL)
				{
					tempTangent[0u] = tangent[vertexIndex * 4u + 0u];
					tempTangent[1u] = tangent[vertexIndex * 4u + 1u];
					tempTangent[2u] = tangent[vertexIndex * 4u + 2u];
					tempTangent[3u] = tangent[vertexIndex * 4u + 3u];
				}
				else
				{
					tempTangent[0u] = defaultTangent[0u];
					tempTangent[1u] = defaultTangent[1u];
					tempTangent[2u] = defaultTangent[2u];
					tempTangent[3u] = defaultTangent[3u];
				}
				tempVertex = &(tempVertex[16]);
			}
			else if (inputLayoutDesc[layoutIndex].SemanticName == CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_COLOR)
			{
				FLOAT* tempColor = (FLOAT*)tempVertex;
				if (color != NULL)
				{
					tempColor[0u] = color[vertexIndex * 4u + 0u];
					tempColor[1u] = color[vertexIndex * 4u + 1u];
					tempColor[2u] = color[vertexIndex * 4u + 2u];
					tempColor[3u] = color[vertexIndex * 4u + 3u];
				}
				else
				{
					tempColor[0u] = defaultColor[0u];
					tempColor[1u] = defaultColor[1u];
					tempColor[2u] = defaultColor[2u];
					tempColor[3u] = defaultColor[3u];
				}
				tempVertex = &(tempVertex[16]);
			}
		}
	}
}
CBaseMesh<UINT>* CMeshManager::ImportOBJMesh(const std::string& name, const CustomStruct::CRenderInputLayoutDesc* inputLayoutDesc, const UINT& inputLayoutNum, const BOOL& needVertexData)
{
	void* vertices = nullptr;
	UINT verticesNum = 0u;
	std::vector<UINT> index;
	std::vector<CustomStruct::CSubMeshInfo> submesh;
	{
		UINT	positionNum	= 0u;
		UINT	normalNum	= 0u;
		UINT	texcoordNum = 0u;
		UINT	vertexNum	= 0u;
		UINT	indexNum	= 0u;
		UINT	in			= 0u;
		UINT	subMeshNum	= 0u;

		CHAR	str[256];
		CHAR*	s;
		CHAR*	tok = NULL;
		CHAR	c;
		FILE*	file;

		fopen_s(&file, name.c_str(), "rt");
		if (file == NULL)
			return NULL;

		while (TRUE)
		{
			fscanf_s(file, "%s", str, 256);

			if (feof(file) != 0)
				break;

			if (strcmp(str, "v") == 0)
			{
				positionNum++;
			}
			else if (strcmp(str, "vn") == 0)
			{
				normalNum++;
			}
			else if (strcmp(str, "vt") == 0)
			{
				texcoordNum++;
			}
			else if (strcmp(str, "usemtl") == 0)
			{
				subMeshNum++;
			}
			else if (strcmp(str, "f") == 0)
			{
				in = 0u;

				do
				{
					fscanf_s(file, "%s", str, 256);
					vertexNum++;
					in++;
					c = fgetc(file);
				} while (c != '\n' && c != '\r');

				if (in == 4u)
				{
					in = 6u;
				}

				indexNum += in;
			}
		}

		std::vector<CustomType::Vector3> positionArray(positionNum);
		std::vector<CustomType::Vector3> normalArray(normalNum);
		std::vector<CustomType::Vector2> texcoordArray(texcoordNum);

		verticesNum = vertexNum;
		UINT vertexStride = 0u;
		BOOL needPosition = FALSE, needTexcoord = FALSE, needNormal = FALSE, needTangent = FALSE, needColor = FALSE;
		UINT offsetPosition = 0u, offsetTexcoord = 0u, offsetNormal = 0u, offsetTangent = 0u, offsetColor = 0u;
		for (UINT i = 0u; i < inputLayoutNum; i++)
		{
			if (inputLayoutDesc[i].SemanticName == CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_POSITION)
			{
				needPosition = TRUE;
				offsetPosition = vertexStride;
			}
			else if (inputLayoutDesc[i].SemanticName == CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_TEXCOORD)
			{
				needTexcoord = TRUE;
				offsetTexcoord = vertexStride;
			}
			else if (inputLayoutDesc[i].SemanticName == CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_NORMAL)
			{
				needNormal = TRUE;
				offsetNormal = vertexStride;
			}
			else if (inputLayoutDesc[i].SemanticName == CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_TANGENT)
			{
				needTangent = TRUE;
				offsetTangent = vertexStride;
			}
			else if (inputLayoutDesc[i].SemanticName == CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_COLOR)
			{
				needColor = TRUE;
				offsetColor = vertexStride;
			}
			vertexStride += inputLayoutDesc[i].GetSemanticSizeByByte();
		}
		vertices = new CHAR[vertexStride * vertexNum];
		index.resize(indexNum);
		submesh.resize(subMeshNum);

		UINT indexPos		= 0u;
		UINT indexNormal	= 0u;
		UINT indexUV		= 0u;

		UINT vc	= 0u;
		UINT ic = 0u;
		UINT sc = 0u;

		fseek(file, 0, SEEK_SET);

		while (TRUE)
		{
			fscanf_s(file, "%s", str, 256);

			if (feof(file) != 0)
			{
				break;
			}

			if (strcmp(str, "mtllib") == 0)
			{
				fscanf_s(file, "%s", str, 256);

				CHAR path[256];
				strncpy_s(path, 256, "data/model/", 12);
				strncat_s(path, 256, str, 256);
				//TODO or not Load material textures
			}
			else if (strcmp(str, "o") == 0)
			{
				fscanf_s(file, "%s", str, 256);
			}
			else if (strcmp(str, "v") == 0)
			{
				FLOAT x, y, z;
				fscanf_s(file, "%f", &x);
				fscanf_s(file, "%f", &y);
				fscanf_s(file, "%f", &z);
				positionArray[indexPos] = CustomType::Vector3(x, y, z);
				indexPos++;
			}
			else if (strcmp(str, "vn") == 0)
			{
				FLOAT x, y, z;
				fscanf_s(file, "%f", &x);
				fscanf_s(file, "%f", &y);
				fscanf_s(file, "%f", &z);
				normalArray[indexNormal] = CustomType::Vector3(x, y, z);
				indexNormal++;
			}
			else if (strcmp(str, "vt") == 0)
			{
				FLOAT x, y;
				fscanf_s(file, "%f", &x);
				fscanf_s(file, "%f", &y);
				//For some DCC, there need transform uv from 0~1 to 1~0.
				//y = 1.f - y;
				texcoordArray[indexUV] = CustomType::Vector2(x, y);
				indexUV++;
			}
			else if (strcmp(str, "usemtl") == 0)
			{
				fscanf_s(file, "%s", str, 256);

				if (sc != 0u)
					submesh[sc - 1u].IndexCount = ic - submesh[sc - 1u].IndexStart;

				submesh[sc].IndexStart = ic;

				//TODO do cache the textures for every submesh, but we don't do that for now.

				sc++;
			}
			else if (strcmp(str, "f") == 0)
			{
				in = 0u;

				do
				{
					fscanf_s(file, "%s", str, 256);

					CHAR* tempVertex = &(((CHAR*)vertices)[vc * vertexStride]);

					s = strtok_s(str, "/", &tok);
					if (needPosition)
					{
						FLOAT* tempPosition = (FLOAT*)(&(tempVertex[offsetPosition]));
						tempPosition[0u] = positionArray[atoi(s) - 1].X();
						tempPosition[1u] = positionArray[atoi(s) - 1].Y();
						tempPosition[2u] = positionArray[atoi(s) - 1].Z();
						tempPosition[3u] = 1.f;
					}
					if (s[strnlen_s(s, 256) + 1] != '/')
					{
						s = strtok_s(NULL, "/", &tok);
						if (needTexcoord)
						{
							FLOAT* tempTexcoord = (FLOAT*)(&(tempVertex[offsetTexcoord]));
							tempTexcoord[0u] = texcoordArray[atoi(s) - 1].X();
							tempTexcoord[1u] = texcoordArray[atoi(s) - 1].Y();
						}
					}
					s = strtok_s(NULL, "/", &tok);
					if (needNormal)
					{
						FLOAT* tempNormal = (FLOAT*)(&(tempVertex[offsetNormal]));
						tempNormal[0u] = normalArray[atoi(s) - 1].X();
						tempNormal[1u] = normalArray[atoi(s) - 1].Y();
						tempNormal[2u] = normalArray[atoi(s) - 1].Z();
						tempNormal[3u] = 0.f;
					}
					if (needColor)
					{
						FLOAT* tempColor = (FLOAT*)(&(tempVertex[offsetColor]));
						tempColor[0u] = 0.5f;
						tempColor[1u] = 0.5f;
						tempColor[2u] = 0.5f;
						tempColor[3u] = 1.f;
					}
#if 0
					if (needTangent)
					{
						FLOAT* tempTangent = (FLOAT*)(&(tempVertex[offsetTangent]));
						tempTangent[0u] = 1.f;
						tempTangent[1u] = 0.f;
						tempTangent[2u] = 0.f;
						tempTangent[3u] = 0.f;
					}
#endif

					index[ic] = vc;

					ic++;
					vc++;

					in++;
					c = fgetc(file);
				} while (c != '\n' && c != '\r');

				if (in == 4u)
				{
					index[ic] = vc - 4u;
					ic++;
					index[ic] = vc - 2u;
					ic++;
				}
			}
		}
		if (sc != 0u)
		{
			submesh[sc - 1u].IndexCount = ic - submesh[sc - 1u].IndexStart;
		}

		if (needTangent)
		{
			CMeshManager::CalculateTangentForMesh<UINT>(submesh, index, vertices, verticesNum, vertexStride, offsetPosition, offsetTexcoord, offsetTangent);
		}
	}
	return (CMeshManager::CreateMeshObject<UINT>(name, inputLayoutDesc, inputLayoutNum, vertices, verticesNum, index, submesh, needVertexData));
}
CBaseMesh<UINT>* CMeshManager::ImportAssetDefaultMesh(const std::string& name, const BOOL& needVertexData)
{
	CHAR* vertices = nullptr; std::vector<UINT> indices; std::vector<CustomStruct::CSubMeshInfo> subMesh;
	UINT numVertices, numIndices, vertexStride;
	if (CassimpManager::ReadDefaultMeshFile(name, subMesh, vertexStride, vertices, numVertices, indices, numIndices) != CassimpManager::CassimpReadFileState::ASSIMP_READ_FILE_STATE_SUCCEED)
	{
		return NULL;
	}

	const CustomStruct::CRenderInputLayoutDesc* inputLayoutDesc; UINT inputLayoutNum;
	CustomStruct::CRenderInputLayoutDesc::GetEngineDefaultMeshInputLayouts(inputLayoutDesc, inputLayoutNum);

	return (CMeshManager::CreateMeshObject<UINT>(name, inputLayoutDesc, inputLayoutNum, (void*)(vertices), numVertices, indices, subMesh, needVertexData));
}
CBaseMesh<UINT>* CMeshManager::ImportAssetSkeletonMesh(const std::string& name, std::vector<CustomStruct::CGameBoneNodeInfo>& skeleton, std::map<std::string, SHORT>& boneIndexMap, std::vector<USHORT>& boneList, const BOOL& needVertexData)
{
	CHAR* vertices = nullptr; std::vector<UINT> indices; std::vector<CustomStruct::CSubMeshInfo> subMesh;
	UINT numVertices, numIndices, vertexStride;
	if (CassimpManager::ReadSkeletonMeshAndBoneFile(name, subMesh, vertexStride, vertices, numVertices, indices, numIndices, skeleton, boneIndexMap, boneList) != CassimpManager::CassimpReadFileState::ASSIMP_READ_FILE_STATE_SUCCEED)
	{
		return NULL;
	}

	const CustomStruct::CRenderInputLayoutDesc* inputLayoutDesc; UINT inputLayoutNum;
	CustomStruct::CRenderInputLayoutDesc::GetEngineSkeletonMeshInputLayouts(inputLayoutDesc, inputLayoutNum);

	return (CMeshManager::CreateMeshObject<UINT>(name, inputLayoutDesc, inputLayoutNum, (void*)(vertices), numVertices, indices, subMesh, needVertexData));
}
std::string CMeshManager::TranslateInputLayoutDesc(const CustomStruct::CRenderInputLayoutDesc* inputLayoutDesc, const UINT& inputLayoutNum)
{
	static std::map<CustomStruct::CRenderShaderSemantic, std::string> semanticStringMap = {
		{ CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_POSITION, "Position" },
		{ CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_TEXCOORD, "Texcoord" },
		{ CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_NORMAL, "Normal" },
		{ CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_TANGENT, "Tangent" },
		{ CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_COLOR, "Color" },
		{ CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_BLENDINDICES, "BlendIndices" },
		{ CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_BLENDWEIGHT, "BlendWeight" },
		{ CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_POSITIONT, "PositionT" },
		{ CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_PSIZE, "PSize" },
		{ CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_BINORMAL, "Binormal" } };

	std::string result = "_";
	result += std::to_string(inputLayoutNum);
	for (UINT i = 0u; i < inputLayoutNum; i++)
	{
		result += "_";
		result += semanticStringMap[inputLayoutDesc[i].SemanticName];
	}
	return result;
}
template<typename IndexType>
CBaseMesh<IndexType>* CMeshManager::CreateMeshObject(const std::string& name, const CustomStruct::CRenderInputLayoutDesc* inputLayoutDesc, const UINT& inputLayoutNum, void* vertexData, const UINT& vertexNum, std::vector<IndexType>& indexData, const std::vector<CustomStruct::CSubMeshInfo>& submeshData, const BOOL& needVertexData)
{
	Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer = nullptr;
	{
		UINT vertexSize = 0u;
		for (UINT i = 0u; i < inputLayoutNum; i++)
		{
			vertexSize += inputLayoutDesc[i].GetSemanticSizeByByte();
		}
		if (!CRenderDevice::CreateBuffer(
			vertexBuffer,
			CustomStruct::CRenderBufferDesc(
				static_cast<UINT>(vertexSize * vertexNum),
				CustomStruct::CRenderBindFlag::BIND_VERTEX_BUFFER, 0u),
			&CustomStruct::CRenderSubresourceData(vertexData, 0u, 0u)))
		{
			return nullptr;
		}
	}
	Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer = nullptr;
	{
		if (!CRenderDevice::CreateBuffer(
			indexBuffer,
			CustomStruct::CRenderBufferDesc(
				static_cast<UINT>(sizeof(IndexType) * indexData.size()),
				CustomStruct::CRenderBindFlag::BIND_INDEX_BUFFER, 0u),
			&CustomStruct::CRenderSubresourceData(static_cast<const void*>(indexData.data()), 0u, 0u)))
		{
			return nullptr;
		}
	}
	CustomType::Vector3 boundMin, boundMax;
	CMeshManager::CalculateBoundMinMax(boundMin, boundMax, inputLayoutDesc, inputLayoutNum, vertexData, vertexNum);
	if (needVertexData)
	{
		CBaseMesh<IndexType>* mesh = new CBaseMesh<IndexType>(name, inputLayoutDesc, inputLayoutNum, vertexData, vertexNum, indexData, submeshData, vertexBuffer, indexBuffer, boundMin, boundMax);
		return mesh;
	}
	else
	{
		CBaseMesh<IndexType>* mesh = new CBaseMesh<IndexType>(name, inputLayoutDesc, inputLayoutNum, nullptr, vertexNum, indexData, submeshData, vertexBuffer, indexBuffer, boundMin, boundMax);
		delete[]vertexData;
		return mesh;
	}
}
void CMeshManager::AddMeshData(const std::string& name, CBaseMesh<UINT>* mesh)
{
	m_MeshManager->m_Data.insert_or_assign(name, mesh);
}
const CBaseMesh<UINT>* CMeshManager::FindMeshData(const std::string& name)
{
	if (m_MeshManager->m_Data.size() > 0)
	{
		auto& element = m_MeshManager->m_Data.find(name);
		if (element != m_MeshManager->m_Data.end())
		{
			return (element->second);
		}
	}
	return NULL;
}
*/