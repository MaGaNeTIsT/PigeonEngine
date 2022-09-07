#include "../../Headers/Game/CMeshManager.h"
#include "../../Headers/Base/CRenderDevice.h"
#include "../../Headers/Game/CMesh.h"

CMeshManager* CMeshManager::m_MeshManager = new CMeshManager();
void CMeshManager::Uninit()
{
	CMeshManager::ClearMeshData();
	delete m_MeshManager;
}
void CMeshManager::ClearMeshData()
{
	if (m_MeshManager->m_Data.size() > 0)
	{
		for (const auto& data : (m_MeshManager->m_Data))
		{
			if ((data.second) != NULL)
			{
				delete (data.second);
			}
		}
		m_MeshManager->m_Data.clear();
	}
}
CMesh* CMeshManager::LoadMeshFromFile(const std::string& name, const BOOL& recalculateTangent)
{
	const static std::string _objName = "obj";

	CMesh* resultMesh = CMeshManager::FindMeshData(name);
	if (resultMesh != NULL)
		return resultMesh;
	{
		size_t pos = name.find_last_of('.');
		if (pos == std::string::npos)
			return NULL;
		std::string typeName = name.substr(pos + 1, name.length());

		if (typeName == _objName)
			resultMesh = (CMeshManager::LoadOBJMesh(name, recalculateTangent));
		else
			return NULL;

		if (resultMesh == NULL)
			return NULL;

		CMeshManager::AddMeshData(name, resultMesh);
	}
	return resultMesh;
}
CMesh* CMeshManager::LoadPlaneMesh(const CustomType::Vector2& length, const CustomType::Vector2Int& vertexCount, const CustomType::Vector2& uv)
{
	if (length.X() <= 0.f || length.Y() <= 0.f || vertexCount.X() < 2 || vertexCount.Y() < 2)
		return NULL;
	if (CustomType::CMath::Abs(uv.X()) < 0.01f || CustomType::CMath::Abs(uv.Y()) < 0.01f)
		return NULL;
	std::string tempName = ENGINE_MESH_PLANE_NAME;
	tempName = tempName +
		"_cx_" + std::to_string(vertexCount.X()) +
		"_cz_" + std::to_string(vertexCount.Y()) +
		"_lx_" + std::to_string(length.X()) +
		"_lz_" + std::to_string(length.Y()) +
		"_u_" + std::to_string(uv.X()) +
		"_v_" + std::to_string(uv.Y());
	CMesh* findResult = CMeshManager::FindMeshData(tempName);
	if (findResult != NULL)
		return findResult;
	std::vector<CustomStruct::CVertex3D> vertex;
	{
		vertex.resize(vertexCount.X() * vertexCount.Y());
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
		XMFLOAT4 normal(0.f, 1.f, 0.f, 0.f);
		XMFLOAT4 tangent(1.f, 0.f, 0.f, 0.f);
		XMFLOAT4 color(1.f, 1.f, 1.f, 1.f);
		for (INT i = 0; i < vertex.size(); i++)
		{
			vertex[i].Normal = normal;
			vertex[i].Tangent = tangent;
			vertex[i].Color = color;
		}
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

				vertex[z * vertexCount.X() + x].Position = XMFLOAT4(px, 0.f, pz, 1.f);
				vertex[z * vertexCount.X() + x].UV0 = XMFLOAT2(u, v);
			}
		}
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
	CMesh* mesh = CMeshManager::CreateMeshObject(tempName, vertex, index, submesh);
	return mesh;
}
CMesh* CMeshManager::LoadCubeMesh()
{
	std::string tempName = ENGINE_MESH_CUBE_NAME;
	CMesh* findResult = CMeshManager::FindMeshData(tempName);
	if (findResult != NULL)
		return findResult;
	std::vector<CustomStruct::CVertex3D> vertex;
	{
		vertex.resize(24);
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
		XMFLOAT4 white(1.f, 1.f, 1.f, 1.f);
		for (UINT i = 0u; i < 24u; i++)
			vertex[i].Color = white;
		vertex[0].Position = XMFLOAT4(-0.5f, 0.5f, -0.5f, 1.f);
		vertex[1].Position = XMFLOAT4(0.5f, 0.5f, -0.5f, 1.f);
		vertex[2].Position = XMFLOAT4(-0.5f, -0.5f, -0.5f, 1.f);
		vertex[3].Position = XMFLOAT4(0.5f, -0.5f, -0.5f, 1.f);
		vertex[0].Normal = XMFLOAT4(0.f, 0.f, -1.f, 0.f);
		vertex[1].Normal = XMFLOAT4(0.f, 0.f, -1.f, 0.f);
		vertex[2].Normal = XMFLOAT4(0.f, 0.f, -1.f, 0.f);
		vertex[3].Normal = XMFLOAT4(0.f, 0.f, -1.f, 0.f);
		vertex[0].Tangent = XMFLOAT4(1.f, 0.f, 0.f, 0.f);
		vertex[1].Tangent = XMFLOAT4(1.f, 0.f, 0.f, 0.f);
		vertex[2].Tangent = XMFLOAT4(1.f, 0.f, 0.f, 0.f);
		vertex[3].Tangent = XMFLOAT4(1.f, 0.f, 0.f, 0.f);
		vertex[0].UV0 = XMFLOAT2(0.f, 0.f);
		vertex[1].UV0 = XMFLOAT2(1.f, 0.f);
		vertex[2].UV0 = XMFLOAT2(0.f, 1.f);
		vertex[3].UV0 = XMFLOAT2(1.f, 1.f);
		vertex[4].Position = XMFLOAT4(0.5f, 0.5f, -0.5f, 1.f);
		vertex[5].Position = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.f);
		vertex[6].Position = XMFLOAT4(0.5f, -0.5f, -0.5f, 1.f);
		vertex[7].Position = XMFLOAT4(0.5f, -0.5f, 0.5f, 1.f);
		vertex[4].Normal = XMFLOAT4(1.f, 0.f, 0.f, 0.f);
		vertex[5].Normal = XMFLOAT4(1.f, 0.f, 0.f, 0.f);
		vertex[6].Normal = XMFLOAT4(1.f, 0.f, 0.f, 0.f);
		vertex[7].Normal = XMFLOAT4(1.f, 0.f, 0.f, 0.f);
		vertex[4].Tangent = XMFLOAT4(0.f, 0.f, 1.f, 0.f);
		vertex[5].Tangent = XMFLOAT4(0.f, 0.f, 1.f, 0.f);
		vertex[6].Tangent = XMFLOAT4(0.f, 0.f, 1.f, 0.f);
		vertex[7].Tangent = XMFLOAT4(0.f, 0.f, 1.f, 0.f);
		vertex[4].UV0 = XMFLOAT2(0.f, 0.f);
		vertex[5].UV0 = XMFLOAT2(1.f, 0.f);
		vertex[6].UV0 = XMFLOAT2(0.f, 1.f);
		vertex[7].UV0 = XMFLOAT2(1.f, 1.f);
		vertex[8].Position = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.f);
		vertex[9].Position = XMFLOAT4(-0.5f, 0.5f, 0.5f, 1.f);
		vertex[10].Position = XMFLOAT4(0.5f, -0.5f, 0.5f, 1.f);
		vertex[11].Position = XMFLOAT4(-0.5f, -0.5f, 0.5f, 1.f);
		vertex[8].Normal = XMFLOAT4(0.f, 0.f, 1.f, 0.f);
		vertex[9].Normal = XMFLOAT4(0.f, 0.f, 1.f, 0.f);
		vertex[10].Normal = XMFLOAT4(0.f, 0.f, 1.f, 0.f);
		vertex[11].Normal = XMFLOAT4(0.f, 0.f, 1.f, 0.f);
		vertex[8].Tangent = XMFLOAT4(-1.f, 0.f, 0.f, 0.f);
		vertex[9].Tangent = XMFLOAT4(-1.f, 0.f, 0.f, 0.f);
		vertex[10].Tangent = XMFLOAT4(-1.f, 0.f, 0.f, 0.f);
		vertex[11].Tangent = XMFLOAT4(-1.f, 0.f, 0.f, 0.f);
		vertex[8].UV0 = XMFLOAT2(0.f, 0.f);
		vertex[9].UV0 = XMFLOAT2(1.f, 0.f);
		vertex[10].UV0 = XMFLOAT2(0.f, 1.f);
		vertex[11].UV0 = XMFLOAT2(1.f, 1.f);
		vertex[12].Position = XMFLOAT4(-0.5f, 0.5f, 0.5f, 1.f);
		vertex[13].Position = XMFLOAT4(-0.5f, 0.5f, -0.5f, 1.f);
		vertex[14].Position = XMFLOAT4(-0.5f, -0.5f, 0.5f, 1.f);
		vertex[15].Position = XMFLOAT4(-0.5f, -0.5f, -0.5f, 1.f);
		vertex[12].Normal = XMFLOAT4(-1.f, 0.f, 0.f, 0.f);
		vertex[13].Normal = XMFLOAT4(-1.f, 0.f, 0.f, 0.f);
		vertex[14].Normal = XMFLOAT4(-1.f, 0.f, 0.f, 0.f);
		vertex[15].Normal = XMFLOAT4(-1.f, 0.f, 0.f, 0.f);
		vertex[12].Tangent = XMFLOAT4(0.f, 0.f, -1.f, 0.f);
		vertex[13].Tangent = XMFLOAT4(0.f, 0.f, -1.f, 0.f);
		vertex[14].Tangent = XMFLOAT4(0.f, 0.f, -1.f, 0.f);
		vertex[15].Tangent = XMFLOAT4(0.f, 0.f, -1.f, 0.f);
		vertex[12].UV0 = XMFLOAT2(0.f, 0.f);
		vertex[13].UV0 = XMFLOAT2(1.f, 0.f);
		vertex[14].UV0 = XMFLOAT2(0.f, 1.f);
		vertex[15].UV0 = XMFLOAT2(1.f, 1.f);
		vertex[16].Position = XMFLOAT4(-0.5f, 0.5f, 0.5f, 1.f);
		vertex[17].Position = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.f);
		vertex[18].Position = XMFLOAT4(-0.5f, 0.5f, -0.5f, 1.f);
		vertex[19].Position = XMFLOAT4(0.5f, 0.5f, -0.5f, 1.f);
		vertex[16].Normal = XMFLOAT4(0.f, 1.f, 0.f, 0.f);
		vertex[17].Normal = XMFLOAT4(0.f, 1.f, 0.f, 0.f);
		vertex[18].Normal = XMFLOAT4(0.f, 1.f, 0.f, 0.f);
		vertex[19].Normal = XMFLOAT4(0.f, 1.f, 0.f, 0.f);
		vertex[16].Tangent = XMFLOAT4(1.f, 0.f, 0.f, 0.f);
		vertex[17].Tangent = XMFLOAT4(1.f, 0.f, 0.f, 0.f);
		vertex[18].Tangent = XMFLOAT4(1.f, 0.f, 0.f, 0.f);
		vertex[19].Tangent = XMFLOAT4(1.f, 0.f, 0.f, 0.f);
		vertex[16].UV0 = XMFLOAT2(0.f, 0.f);
		vertex[17].UV0 = XMFLOAT2(1.f, 0.f);
		vertex[18].UV0 = XMFLOAT2(0.f, 1.f);
		vertex[19].UV0 = XMFLOAT2(1.f, 1.f);
		vertex[20].Position = XMFLOAT4(-0.5f, -0.5f, -0.5f, 1.f);
		vertex[21].Position = XMFLOAT4(0.5f, -0.5f, -0.5f, 1.f);
		vertex[22].Position = XMFLOAT4(-0.5f, -0.5f, 0.5f, 1.f);
		vertex[23].Position = XMFLOAT4(0.5f, -0.5f, 0.5f, 1.f);
		vertex[20].Normal = XMFLOAT4(0.f, -1.f, 0.f, 0.f);
		vertex[21].Normal = XMFLOAT4(0.f, -1.f, 0.f, 0.f);
		vertex[22].Normal = XMFLOAT4(0.f, -1.f, 0.f, 0.f);
		vertex[23].Normal = XMFLOAT4(0.f, -1.f, 0.f, 0.f);
		vertex[20].Tangent = XMFLOAT4(1.f, 0.f, 0.f, 0.f);
		vertex[21].Tangent = XMFLOAT4(1.f, 0.f, 0.f, 0.f);
		vertex[22].Tangent = XMFLOAT4(1.f, 0.f, 0.f, 0.f);
		vertex[23].Tangent = XMFLOAT4(1.f, 0.f, 0.f, 0.f);
		vertex[20].UV0 = XMFLOAT2(0.f, 0.f);
		vertex[21].UV0 = XMFLOAT2(1.f, 0.f);
		vertex[22].UV0 = XMFLOAT2(0.f, 1.f);
		vertex[23].UV0 = XMFLOAT2(1.f, 1.f);
	}
	std::vector<UINT> index;
	{
		index.resize(36);
		for (UINT i = 0u; i < 6u; i++)
		{
			index[i * 6u + 0u] = i * 4u + 0;
			index[i * 6u + 1u] = i * 4u + 1;
			index[i * 6u + 2u] = i * 4u + 2;

			index[i * 6u + 3u] = i * 4u + 1;
			index[i * 6u + 4u] = i * 4u + 3;
			index[i * 6u + 5u] = i * 4u + 2;
		}
	}
	std::vector<CustomStruct::CSubMeshInfo> submesh(0);
	CMesh* mesh = CMeshManager::CreateMeshObject(tempName, vertex, index, submesh);
	return mesh;
}
CMesh* CMeshManager::LoadPolygonMesh()
{
	std::string tempName = ENGINE_MESH_POLYGON_NAME;
	CMesh* findResult = CMeshManager::FindMeshData(tempName);
	if (findResult != NULL)
		return findResult;
	std::vector<CustomStruct::CVertex3D> vertex;
	{
		vertex.resize(4);
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
		XMFLOAT4 white(1.f, 1.f, 1.f, 1.f);
		for (UINT i = 0u; i < 4; i++)
			vertex[i].Color = white;
		vertex[0].Position = XMFLOAT4(-0.5f, 0.5f, 0.f, 1.f);
		vertex[1].Position = XMFLOAT4(0.5f, 0.5f, 0.f, 1.f);
		vertex[2].Position = XMFLOAT4(-0.5f, -0.5f, 0.f, 1.f);
		vertex[3].Position = XMFLOAT4(0.5f, -0.5f, 0.f, 1.f);
		vertex[0].Normal = XMFLOAT4(0.f, 0.f, -1.f, 0.f);
		vertex[1].Normal = XMFLOAT4(0.f, 0.f, -1.f, 0.f);
		vertex[2].Normal = XMFLOAT4(0.f, 0.f, -1.f, 0.f);
		vertex[3].Normal = XMFLOAT4(0.f, 0.f, -1.f, 0.f);
		vertex[0].Tangent = XMFLOAT4(1.f, 0.f, 0.f, 0.f);
		vertex[1].Tangent = XMFLOAT4(1.f, 0.f, 0.f, 0.f);
		vertex[2].Tangent = XMFLOAT4(1.f, 0.f, 0.f, 0.f);
		vertex[3].Tangent = XMFLOAT4(1.f, 0.f, 0.f, 0.f);
		vertex[0].UV0 = XMFLOAT2(0.f, 0.f);
		vertex[1].UV0 = XMFLOAT2(1.f, 0.f);
		vertex[2].UV0 = XMFLOAT2(0.f, 1.f);
		vertex[3].UV0 = XMFLOAT2(1.f, 1.f);
	}
	std::vector<UINT> index;
	{
		index.resize(6);
		index[0] = 0; index[1] = 1; index[2] = 2;
		index[3] = 1; index[4] = 3; index[5] = 2;
	}
	std::vector<CustomStruct::CSubMeshInfo> submesh(0);
	CMesh* mesh = CMeshManager::CreateMeshObject(tempName, vertex, index, submesh);
	return mesh;
}
CMesh* CMeshManager::LoadPolygon2DMesh(const CustomType::Vector4Int& customSize)
{
	std::string tempName = ENGINE_MESH_POLYGON_2D_NAME;
	tempName = tempName +
		"_x_" + std::to_string(customSize.X()) +
		"_y_" + std::to_string(customSize.Y()) +
		"_z_" + std::to_string(customSize.Z()) +
		"_w_" + std::to_string(customSize.W());
	CMesh* findResult = CMeshManager::FindMeshData(tempName);
	if (findResult != NULL)
		return findResult;
	std::vector<CustomStruct::CVertex3D> vertex;
	{
		vertex.resize(4);
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
		XMFLOAT4 white(1.f, 1.f, 1.f, 1.f);
		for (UINT i = 0u; i < 4; i++)
			vertex[i].Color = white;
		vertex[0].Position = XMFLOAT4(static_cast<FLOAT>(customSize.X()), static_cast<FLOAT>(customSize.Y()), 0.f, 1.f);
		vertex[1].Position = XMFLOAT4(static_cast<FLOAT>(customSize.Z()), static_cast<FLOAT>(customSize.Y()), 0.f, 1.f);
		vertex[2].Position = XMFLOAT4(static_cast<FLOAT>(customSize.X()), static_cast<FLOAT>(customSize.W()), 0.f, 1.f);
		vertex[3].Position = XMFLOAT4(static_cast<FLOAT>(customSize.Z()), static_cast<FLOAT>(customSize.W()), 0.f, 1.f);
		vertex[0].Normal = XMFLOAT4(0.f, 0.f, -1.f, 0.f);
		vertex[1].Normal = XMFLOAT4(0.f, 0.f, -1.f, 0.f);
		vertex[2].Normal = XMFLOAT4(0.f, 0.f, -1.f, 0.f);
		vertex[3].Normal = XMFLOAT4(0.f, 0.f, -1.f, 0.f);
		vertex[0].Tangent = XMFLOAT4(1.f, 0.f, 0.f, 0.f);
		vertex[1].Tangent = XMFLOAT4(1.f, 0.f, 0.f, 0.f);
		vertex[2].Tangent = XMFLOAT4(1.f, 0.f, 0.f, 0.f);
		vertex[3].Tangent = XMFLOAT4(1.f, 0.f, 0.f, 0.f);
		vertex[0].UV0 = XMFLOAT2(0.f, 0.f);
		vertex[1].UV0 = XMFLOAT2(1.f, 0.f);
		vertex[2].UV0 = XMFLOAT2(0.f, 1.f);
		vertex[3].UV0 = XMFLOAT2(1.f, 1.f);
	}
	std::vector<UINT> index;
	{
		index.resize(6);
		index[0] = 0; index[1] = 1; index[2] = 2;
		index[3] = 1; index[4] = 3; index[5] = 2;
	}
	std::vector<CustomStruct::CSubMeshInfo> submesh(0);
	CMesh* mesh = CMeshManager::CreateMeshObject(tempName, vertex, index, submesh);
	return mesh;
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
		scale = scale > 0.f ? 1e-6f : (-1e-6f);
	scale = 1.f / scale;
	return (CustomType::Vector3(
		st2.Y() * q1.X() + (-st1.Y()) * q2.X(),
		st2.Y() * q1.Y() + (-st1.Y()) * q2.Y(),
		st2.Y() * q1.Z() + (-st1.Y()) * q2.Z()) * scale);
}
CMesh* CMeshManager::LoadOBJMesh(const std::string& name, const BOOL& recalculateTangent)
{
	std::vector<CustomStruct::CVertex3D> vertex;
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
					in = 6u;

				indexNum += in;
			}
		}

		std::vector<CustomType::Vector3> positionArray(positionNum);
		std::vector<CustomType::Vector3> normalArray(normalNum);
		std::vector<CustomType::Vector2> texcoordArray(texcoordNum);

		vertex.resize(vertexNum);
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
				break;

			if (strcmp(str, "mtllib") == 0)
			{
				fscanf_s(file, "%s", str, 256);

				CHAR path[256];
				strncpy_s(path, 256, "data/model/", 256);
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

					s = strtok_s(str, "/", &tok);
					vertex[vc].Position = positionArray[atoi(s) - 1].GetXMFLOAT4();
					if (s[strnlen_s(s, 256) + 1] != '/')
					{
						s = strtok_s(NULL, "/", &tok);
						vertex[vc].UV0 = texcoordArray[atoi(s) - 1].GetXMFLOAT2();
					}
					s = strtok_s(NULL, "/", &tok);
					vertex[vc].Normal = normalArray[atoi(s) - 1].GetXMFLOAT4();

					vertex[vc].Color = XMFLOAT4(1.f, 1.f, 1.f, 1.f);
					vertex[vc].Tangent = XMFLOAT4(1.f, 0.f, 0.f, 0.f);

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
			submesh[sc - 1u].IndexCount = ic - submesh[sc - 1u].IndexStart;
	}
	if (recalculateTangent)
	{
		for (INT i = 0; i < vertex.size(); i++)
		{
			vertex[i].Tangent = XMFLOAT4(0.f, 0.f, 0.f, 0.f);
		}
		if (submesh.size() > 1)
		{
			CustomStruct::CSubMeshInfo subInfo;
			CustomType::Vector3 p0, p1, p2;
			CustomType::Vector2 uv0, uv1, uv2;
			CustomType::Vector3 newT, oldT;
			INT triangleNum, triangleIndex;
			for (INT subIndex = 0; subIndex < submesh.size(); subIndex++)
			{
				subInfo = submesh[subIndex];
				triangleNum = subInfo.IndexCount / 3;
				for (INT i = 0; i < triangleNum; i++)
				{
					triangleIndex = subInfo.IndexStart + i * 3;
					p0 = CustomType::Vector3(vertex[subInfo.VertexStart + index[triangleIndex]].Position);
					uv0 = CustomType::Vector2(vertex[subInfo.VertexStart + index[triangleIndex]].UV0);
					triangleIndex = subInfo.IndexStart + i * 3 + 1;
					p1 = CustomType::Vector3(vertex[subInfo.VertexStart + index[triangleIndex]].Position);
					uv1 = CustomType::Vector2(vertex[subInfo.VertexStart + index[triangleIndex]].UV0);
					triangleIndex = subInfo.IndexStart + i * 3 + 2;
					p2 = CustomType::Vector3(vertex[subInfo.VertexStart + index[triangleIndex]].Position);
					uv2 = CustomType::Vector2(vertex[subInfo.VertexStart + index[triangleIndex]].UV0);
					newT = CMeshManager::CalculateTangentForTriangle(p0, p1, p2, uv0, uv1, uv2);
					triangleIndex = subInfo.IndexStart + i * 3;
					oldT = CustomType::Vector3(vertex[subInfo.VertexStart + index[triangleIndex]].Tangent);
					vertex[subInfo.VertexStart + index[triangleIndex]].Tangent = (oldT + newT).GetXMFLOAT4();
					triangleIndex = subInfo.IndexStart + i * 3 + 1;
					oldT = CustomType::Vector3(vertex[subInfo.VertexStart + index[triangleIndex]].Tangent);
					vertex[subInfo.VertexStart + index[triangleIndex]].Tangent = (oldT + newT).GetXMFLOAT4();
					triangleIndex = subInfo.IndexStart + i * 3 + 2;
					oldT = CustomType::Vector3(vertex[subInfo.VertexStart + index[triangleIndex]].Tangent);
					vertex[subInfo.VertexStart + index[triangleIndex]].Tangent = (oldT + newT).GetXMFLOAT4();
				}
			}
		}
		else
		{
			CustomType::Vector3 p0, p1, p2;
			CustomType::Vector2 uv0, uv1, uv2;
			CustomType::Vector3 newT, oldT;
			INT triangleIndex;
			INT triangleNum = static_cast<INT>(index.size()) / 3;
			for (INT i = 0; i < triangleNum; i++)
			{
				triangleIndex = i * 3;
				p0 = CustomType::Vector3(vertex[index[triangleIndex]].Position);
				uv0 = CustomType::Vector2(vertex[index[triangleIndex]].UV0);
				triangleIndex = i * 3 + 1;
				p1 = CustomType::Vector3(vertex[index[triangleIndex]].Position);
				uv1 = CustomType::Vector2(vertex[index[triangleIndex]].UV0);
				triangleIndex = i * 3 + 2;
				p2 = CustomType::Vector3(vertex[index[triangleIndex]].Position);
				uv2 = CustomType::Vector2(vertex[index[triangleIndex]].UV0);
				newT = CMeshManager::CalculateTangentForTriangle(p0, p1, p2, uv0, uv1, uv2);
				triangleIndex = i * 3;
				oldT = CustomType::Vector3(vertex[index[triangleIndex]].Tangent);
				vertex[index[triangleIndex]].Tangent = (oldT + newT).GetXMFLOAT4();
				triangleIndex = i * 3 + 1;
				oldT = CustomType::Vector3(vertex[index[triangleIndex]].Tangent);
				vertex[index[triangleIndex]].Tangent = (oldT + newT).GetXMFLOAT4();
				triangleIndex = i * 3 + 2;
				oldT = CustomType::Vector3(vertex[index[triangleIndex]].Tangent);
				vertex[index[triangleIndex]].Tangent = (oldT + newT).GetXMFLOAT4();
			}
		}
		for (INT i = 0; i < vertex.size(); i++)
		{
			vertex[i].Tangent = CustomType::Vector3::Normalize(CustomType::Vector3(vertex[i].Tangent)).GetXMFLOAT4();
		}
	}
	CMesh* mesh = CMeshManager::CreateMeshObject(name, vertex, index, submesh);
	return mesh;
}
template<typename vertexType, typename indexType>
CMesh* CMeshManager::CreateMeshObject(const std::string& name, std::vector<vertexType>& vertexData, std::vector<indexType>& indexData, std::vector<CustomStruct::CSubMeshInfo> submeshData)
{
	Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer = nullptr;
	{
		if (CRenderDevice::CreateD3DBuffer(
			vertexBuffer,
			vertexData.data(),
			sizeof(vertexType),
			static_cast<UINT>(vertexData.size()),
			D3D11_BIND_VERTEX_BUFFER) == false)
			return NULL;
	}
	Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer = nullptr;
	{
		if (CRenderDevice::CreateD3DBuffer(
			indexBuffer,
			indexData.data(),
			sizeof(indexType),
			static_cast<UINT>(indexData.size()),
			D3D11_BIND_INDEX_BUFFER) == false)
			return NULL;
	}
	CMesh* mesh = new CMesh(name, vertexData, indexData, submeshData, vertexBuffer, indexBuffer);
	CMeshManager::AddMeshData(name, mesh);
	return mesh;
}
void CMeshManager::AddMeshData(const std::string& name, CMesh* mesh)
{
	m_MeshManager->m_Data[name] = mesh;
}
CMesh* CMeshManager::FindMeshData(const std::string& name)
{
	std::map<std::string, CMesh*>::iterator element = m_MeshManager->m_Data.find(name);
	if (element == m_MeshManager->m_Data.end())
		return NULL;
	return (element->second);
}