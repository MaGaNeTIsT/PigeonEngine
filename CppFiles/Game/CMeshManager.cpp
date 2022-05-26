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
CMesh* CMeshManager::LoadOBJMesh(const std::string& name)
{
	//XMFLOAT3*	positionArray;
	//XMFLOAT3*	normalArray;
	//XMFLOAT2*	texcoordArray;

	//std::vector<CustomStruct::CVertex3D> vertex;

	//UINT	positionNum = 0;
	//UINT	normalNum = 0;
	//UINT	texcoordNum = 0;
	//UINT	vertexNum = 0;
	//UINT	indexNum = 0;
	//UINT	in = 0;
	//UINT	subsetNum = 0;

	//MODEL_MATERIAL* materialArray = NULL;
	//USHORT materialNum = 0;

	//CHAR str[256];
	//CHAR* s;
	//CHAR c;

	//FILE* file;
	//file = fopen(FileName, "rt");
	//assert(file);

	//while (true)
	//{
	//	fscanf(file, "%s", str);

	//	if (feof(file) != 0)
	//		break;

	//	if (strcmp(str, "v") == 0)
	//	{
	//		positionNum++;
	//	}
	//	else if (strcmp(str, "vn") == 0)
	//	{
	//		normalNum++;
	//	}
	//	else if (strcmp(str, "vt") == 0)
	//	{
	//		texcoordNum++;
	//	}
	//	else if (strcmp(str, "usemtl") == 0)
	//	{
	//		subsetNum++;
	//	}
	//	else if (strcmp(str, "f") == 0)
	//	{
	//		in = 0;

	//		do
	//		{
	//			fscanf(file, "%s", str);
	//			vertexNum++;
	//			in++;
	//			c = fgetc(file);
	//		} while (c != '\n' && c != '\r');

	//		if (in == 4)
	//			in = 6;

	//		indexNum += in;
	//	}
	//}

	//positionArray = new XMFLOAT3[positionNum];
	//normalArray = new XMFLOAT3[normalNum];
	//texcoordArray = new XMFLOAT2[texcoordNum];

	//Model->VertexArray = new CustomStruct::CVertex3DData[vertexNum];
	//Model->VertexNum = vertexNum;

	//Model->IndexArray = new USHORT[indexNum];
	//Model->IndexNum = indexNum;

	//Model->SubsetArray = new SUBSET[subsetNum];
	//Model->SubsetNum = subsetNum;

	//XMFLOAT3* position = positionArray;
	//XMFLOAT3* normal = normalArray;
	//XMFLOAT2* texcoord = texcoordArray;

	//USHORT vc = 0;
	//USHORT ic = 0;
	//USHORT sc = 0;

	//fseek(file, 0, SEEK_SET);

	//while (true)
	//{
	//	fscanf(file, "%s", str);

	//	if (feof(file) != 0)
	//		break;

	//	if (strcmp(str, "mtllib") == 0)
	//	{
	//		fscanf(file, "%s", str);

	//		CHAR path[256];
	//		strcpy(path, "data/model/");
	//		strcat(path, str);

	//		LoadMaterial(path, &materialArray, &materialNum);
	//	}
	//	else if (strcmp(str, "o") == 0)
	//	{
	//		fscanf(file, "%s", str);
	//	}
	//	else if (strcmp(str, "v") == 0)
	//	{
	//		fscanf(file, "%f", &position->x);
	//		fscanf(file, "%f", &position->y);
	//		fscanf(file, "%f", &position->z);
	//		//position->x *= SCALE_MODEL;
	//		//position->y *= SCALE_MODEL;
	//		//position->z *= SCALE_MODEL;
	//		position++;
	//	}
	//	else if (strcmp(str, "vn") == 0)
	//	{
	//		fscanf(file, "%f", &normal->x);
	//		fscanf(file, "%f", &normal->y);
	//		fscanf(file, "%f", &normal->z);
	//		normal++;
	//	}
	//	else if (strcmp(str, "vt") == 0)
	//	{
	//		fscanf(file, "%f", &texcoord->x);
	//		fscanf(file, "%f", &texcoord->y);
	//		//texcoord->y = 1.0f - texcoord->y;
	//		texcoord++;
	//	}
	//	else if (strcmp(str, "usemtl") == 0)
	//	{
	//		fscanf(file, "%s", str);

	//		if (sc != 0)
	//			Model->SubsetArray[sc - 1].IndexNum = ic - Model->SubsetArray[sc - 1].StartIndex;

	//		Model->SubsetArray[sc].StartIndex = ic;

	//		for (USHORT i = 0; i < materialNum; i++)
	//		{
	//			if (strcmp(str, materialArray[i].Name) == 0)
	//			{
	//				Model->SubsetArray[sc].Material.Material = materialArray[i].Material;
	//				strcpy(Model->SubsetArray[sc].Material.TextureName, materialArray[i].TextureName);
	//				strcpy(Model->SubsetArray[sc].Material.Name, materialArray[i].Name);

	//				break;
	//			}
	//		}

	//		sc++;
	//	}
	//	else if (strcmp(str, "f") == 0)
	//	{
	//		in = 0;

	//		do
	//		{
	//			fscanf(file, "%s", str);

	//			s = strtok(str, "/");
	//			Model->VertexArray[vc].Position = positionArray[atoi(s) - 1];
	//			if (s[strlen(s) + 1] != '/')
	//			{
	//				s = strtok(NULL, "/");
	//				Model->VertexArray[vc].UV0 = texcoordArray[atoi(s) - 1];
	//			}
	//			s = strtok(NULL, "/");
	//			Model->VertexArray[vc].Normal = normalArray[atoi(s) - 1];

	//			Model->VertexArray[vc].Color = XMFLOAT4(1.f, 1.f, 1.f, 1.f);

	//			Model->IndexArray[ic] = vc;
	//			ic++;
	//			vc++;

	//			in++;
	//			c = fgetc(file);
	//		} while (c != '\n' && c != '\r');

	//		if (in == 4)
	//		{
	//			Model->IndexArray[ic] = vc - 4;
	//			ic++;
	//			Model->IndexArray[ic] = vc - 2;
	//			ic++;
	//		}
	//	}
	//}

	//if (sc != 0)
	//	Model->SubsetArray[sc - 1].IndexNum = ic - Model->SubsetArray[sc - 1].StartIndex;

	//delete[] positionArray;
	//delete[] normalArray;
	//delete[] texcoordArray;
	//delete[] materialArray;

	return NULL;
}
CMesh* CMeshManager::LoadPlaneMesh(const CustomType::Vector2& length, const CustomType::Vector2Int& vertexCount, const CustomType::Vector2& uv)
{
	if (length.X() <= 0.f || length.Y() <= 0.f || vertexCount.X() < 2 || vertexCount.Y() < 2)
		return NULL;
	if (fabsf(uv.X()) < 0.01f || fabsf(uv.Y()) < 0.01f)
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
				u = CustomType::CMath::Lerp(fminf(0.f, uv.X()), fmaxf(0.f, uv.X()), tx);

				tz = static_cast<FLOAT>(z) / static_cast<FLOAT>(vertexCount.Y() - 1);
				pz = CustomType::CMath::Lerp(length.Y() * 0.5f, length.Y() * (-0.5f), tz);
				v = CustomType::CMath::Lerp(fminf(0.f, uv.Y()), fmaxf(0.f, uv.Y()), tz);

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
	CMesh* mesh = CMeshManager::CreateMeshObject(tempName, vertex, index);
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
		vertex[ 0].Position = XMFLOAT4(-0.5f,  0.5f, -0.5f, 1.f);
		vertex[ 1].Position = XMFLOAT4( 0.5f,  0.5f, -0.5f, 1.f);
		vertex[ 2].Position = XMFLOAT4(-0.5f, -0.5f, -0.5f, 1.f);
		vertex[ 3].Position = XMFLOAT4( 0.5f, -0.5f, -0.5f, 1.f);
		vertex[ 0].Normal = XMFLOAT4( 0.f,  0.f, -1.f, 0.f);
		vertex[ 1].Normal = XMFLOAT4( 0.f,  0.f, -1.f, 0.f);
		vertex[ 2].Normal = XMFLOAT4( 0.f,  0.f, -1.f, 0.f);
		vertex[ 3].Normal = XMFLOAT4( 0.f,  0.f, -1.f, 0.f);
		vertex[ 0].Tangent = XMFLOAT4( 1.f,  0.f,  0.f, 0.f);
		vertex[ 1].Tangent = XMFLOAT4( 1.f,  0.f,  0.f, 0.f);
		vertex[ 2].Tangent = XMFLOAT4( 1.f,  0.f,  0.f, 0.f);
		vertex[ 3].Tangent = XMFLOAT4( 1.f,  0.f,  0.f, 0.f);
		vertex[ 0].UV0 = XMFLOAT2(0.f, 0.f);
		vertex[ 1].UV0 = XMFLOAT2(1.f, 0.f);
		vertex[ 2].UV0 = XMFLOAT2(0.f, 1.f);
		vertex[ 3].UV0 = XMFLOAT2(1.f, 1.f);
		vertex[ 4].Position = XMFLOAT4( 0.5f,  0.5f, -0.5f, 1.f);
		vertex[ 5].Position = XMFLOAT4( 0.5f,  0.5f,  0.5f, 1.f);
		vertex[ 6].Position = XMFLOAT4( 0.5f, -0.5f, -0.5f, 1.f);
		vertex[ 7].Position = XMFLOAT4( 0.5f, -0.5f,  0.5f, 1.f);
		vertex[ 4].Normal = XMFLOAT4( 1.f,  0.f,  0.f, 0.f);
		vertex[ 5].Normal = XMFLOAT4( 1.f,  0.f,  0.f, 0.f);
		vertex[ 6].Normal = XMFLOAT4( 1.f,  0.f,  0.f, 0.f);
		vertex[ 7].Normal = XMFLOAT4( 1.f,  0.f,  0.f, 0.f);
		vertex[ 4].Tangent = XMFLOAT4( 0.f,  0.f,  1.f, 0.f);
		vertex[ 5].Tangent = XMFLOAT4( 0.f,  0.f,  1.f, 0.f);
		vertex[ 6].Tangent = XMFLOAT4( 0.f,  0.f,  1.f, 0.f);
		vertex[ 7].Tangent = XMFLOAT4( 0.f,  0.f,  1.f, 0.f);
		vertex[ 4].UV0 = XMFLOAT2(0.f, 0.f);
		vertex[ 5].UV0 = XMFLOAT2(1.f, 0.f);
		vertex[ 6].UV0 = XMFLOAT2(0.f, 1.f);
		vertex[ 7].UV0 = XMFLOAT2(1.f, 1.f);
		vertex[ 8].Position = XMFLOAT4( 0.5f,  0.5f,  0.5f, 1.f);
		vertex[ 9].Position = XMFLOAT4(-0.5f,  0.5f,  0.5f, 1.f);
		vertex[10].Position = XMFLOAT4( 0.5f, -0.5f,  0.5f, 1.f);
		vertex[11].Position = XMFLOAT4(-0.5f, -0.5f,  0.5f, 1.f);
		vertex[ 8].Normal = XMFLOAT4( 0.f,  0.f,  1.f, 0.f);
		vertex[ 9].Normal = XMFLOAT4( 0.f,  0.f,  1.f, 0.f);
		vertex[10].Normal = XMFLOAT4( 0.f,  0.f,  1.f, 0.f);
		vertex[11].Normal = XMFLOAT4( 0.f,  0.f,  1.f, 0.f);
		vertex[ 8].Tangent = XMFLOAT4(-1.f,  0.f,  0.f, 0.f);
		vertex[ 9].Tangent = XMFLOAT4(-1.f,  0.f,  0.f, 0.f);
		vertex[10].Tangent = XMFLOAT4(-1.f,  0.f,  0.f, 0.f);
		vertex[11].Tangent = XMFLOAT4(-1.f,  0.f,  0.f, 0.f);
		vertex[ 8].UV0 = XMFLOAT2(0.f, 0.f);
		vertex[ 9].UV0 = XMFLOAT2(1.f, 0.f);
		vertex[10].UV0 = XMFLOAT2(0.f, 1.f);
		vertex[11].UV0 = XMFLOAT2(1.f, 1.f);
		vertex[12].Position = XMFLOAT4(-0.5f,  0.5f,  0.5f, 1.f);
		vertex[13].Position = XMFLOAT4(-0.5f,  0.5f, -0.5f, 1.f);
		vertex[14].Position = XMFLOAT4(-0.5f, -0.5f,  0.5f, 1.f);
		vertex[15].Position = XMFLOAT4(-0.5f, -0.5f, -0.5f, 1.f);
		vertex[12].Normal = XMFLOAT4(-1.f,  0.f,  0.f, 0.f);
		vertex[13].Normal = XMFLOAT4(-1.f,  0.f,  0.f, 0.f);
		vertex[14].Normal = XMFLOAT4(-1.f,  0.f,  0.f, 0.f);
		vertex[15].Normal = XMFLOAT4(-1.f,  0.f,  0.f, 0.f);
		vertex[12].Tangent = XMFLOAT4( 0.f,  0.f, -1.f, 0.f);
		vertex[13].Tangent = XMFLOAT4( 0.f,  0.f, -1.f, 0.f);
		vertex[14].Tangent = XMFLOAT4( 0.f,  0.f, -1.f, 0.f);
		vertex[15].Tangent = XMFLOAT4( 0.f,  0.f, -1.f, 0.f);
		vertex[12].UV0 = XMFLOAT2(0.f, 0.f);
		vertex[13].UV0 = XMFLOAT2(1.f, 0.f);
		vertex[14].UV0 = XMFLOAT2(0.f, 1.f);
		vertex[15].UV0 = XMFLOAT2(1.f, 1.f);
		vertex[16].Position = XMFLOAT4(-0.5f,  0.5f,  0.5f, 1.f);
		vertex[17].Position = XMFLOAT4( 0.5f,  0.5f,  0.5f, 1.f);
		vertex[18].Position = XMFLOAT4(-0.5f,  0.5f, -0.5f, 1.f);
		vertex[19].Position = XMFLOAT4( 0.5f,  0.5f, -0.5f, 1.f);
		vertex[16].Normal = XMFLOAT4( 0.f,  1.f,  0.f, 0.f);
		vertex[17].Normal = XMFLOAT4( 0.f,  1.f,  0.f, 0.f);
		vertex[18].Normal = XMFLOAT4( 0.f,  1.f,  0.f, 0.f);
		vertex[19].Normal = XMFLOAT4( 0.f,  1.f,  0.f, 0.f);
		vertex[16].Tangent = XMFLOAT4( 1.f,  0.f,  0.f, 0.f);
		vertex[17].Tangent = XMFLOAT4( 1.f,  0.f,  0.f, 0.f);
		vertex[18].Tangent = XMFLOAT4( 1.f,  0.f,  0.f, 0.f);
		vertex[19].Tangent = XMFLOAT4( 1.f,  0.f,  0.f, 0.f);
		vertex[16].UV0 = XMFLOAT2(0.f, 0.f);
		vertex[17].UV0 = XMFLOAT2(1.f, 0.f);
		vertex[18].UV0 = XMFLOAT2(0.f, 1.f);
		vertex[19].UV0 = XMFLOAT2(1.f, 1.f);
		vertex[20].Position = XMFLOAT4(-0.5f, -0.5f, -0.5f, 1.f);
		vertex[21].Position = XMFLOAT4( 0.5f, -0.5f, -0.5f, 1.f);
		vertex[22].Position = XMFLOAT4(-0.5f, -0.5f,  0.5f, 1.f);
		vertex[23].Position = XMFLOAT4( 0.5f, -0.5f,  0.5f, 1.f);
		vertex[20].Normal = XMFLOAT4( 0.f, -1.f,  0.f, 0.f);
		vertex[21].Normal = XMFLOAT4( 0.f, -1.f,  0.f, 0.f);
		vertex[22].Normal = XMFLOAT4( 0.f, -1.f,  0.f, 0.f);
		vertex[23].Normal = XMFLOAT4( 0.f, -1.f,  0.f, 0.f);
		vertex[20].Tangent = XMFLOAT4( 1.f,  0.f,  0.f, 0.f);
		vertex[21].Tangent = XMFLOAT4( 1.f,  0.f,  0.f, 0.f);
		vertex[22].Tangent = XMFLOAT4( 1.f,  0.f,  0.f, 0.f);
		vertex[23].Tangent = XMFLOAT4( 1.f,  0.f,  0.f, 0.f);
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
	CMesh* mesh = CMeshManager::CreateMeshObject(tempName, vertex, index);
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
		vertex[0].Position = XMFLOAT4(-0.5f,  0.5f, 0.f, 1.f);
		vertex[1].Position = XMFLOAT4( 0.5f,  0.5f, 0.f, 1.f);
		vertex[2].Position = XMFLOAT4(-0.5f, -0.5f, 0.f, 1.f);
		vertex[3].Position = XMFLOAT4( 0.5f, -0.5f, 0.f, 1.f);
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
	CMesh* mesh = CMeshManager::CreateMeshObject(tempName, vertex, index);
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
	CMesh* mesh = CMeshManager::CreateMeshObject(tempName, vertex, index);
	return mesh;
}
template<typename vertexType, typename indexType>
CMesh* CMeshManager::CreateMeshObject(const std::string& name, std::vector<vertexType>& vertexData, std::vector<indexType>& indexData)
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
	CMesh* mesh = new CMesh(name, vertexData, indexData, vertexBuffer, indexBuffer);
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