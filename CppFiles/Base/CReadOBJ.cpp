#define _CRT_SECURE_NO_WARNINGS
#include "../../Entry/MyMain.h"
#include "../../Headers/Base/CReadOBJ.h"
//#include "../../Headers/Base/CInput.h"
//#include "../../Headers/Base/CShader.h"
//#include "../../Headers/Base/CTexture2D.h"
//#include "../../Headers/Base/CManager.h"
//#include "../../Headers/Game/CCamera.h"
//
//void CReadOBJ::Init()
//{
//	m_Position = XMFLOAT3( 0.f, 0.f, 0.f );
//	m_Rotation = XMFLOAT3( 0.f, 0.f, 0.f );
//	m_Scale = XMFLOAT3( 1.f, 1.f, 1.f );
//
//	//Load("data/MODEL/cube.obj");
//	//Load("data/MODEL/sphere_smooth.obj");
//	Load("data/MODEL/torus.obj");
//
//	m_Shader = new CShader();
//	m_Shader->Init("./HLSLCSO/3DTextureVS.cso", "./HLSLCSO/3DTexturePS.cso");
//
//	m_Texture = new CTexture();
//	m_Texture->Load("./Assets/Textures/Fur/fur.tga");
//}
//
//void CReadOBJ::Uninit()
//{
//	m_Shader->Uninit();
//	delete m_Shader;
//
//	Unload();
//}
//
//void CReadOBJ::Update()
//{
//	if (CInput::GetKeyPress('A'))
//		m_Position.x -= 0.1f;
//	if (CInput::GetKeyPress('D'))
//		m_Position.x += 0.1f;
//	if (CInput::GetKeyPress('W'))
//		m_Position.z += 0.1f;
//	if (CInput::GetKeyPress('S'))
//		m_Position.z -= 0.1f;
//
//	if (CInput::GetKeyPress('Q'))
//		m_Rotation.y -= 0.05f;
//	if (CInput::GetKeyPress('E'))
//		m_Rotation.y += 0.05f;
//	if (CInput::GetKeyPress('R'))
//		m_Rotation.x -= 0.05f;
//	if (CInput::GetKeyPress('F'))
//		m_Rotation.x += 0.05f;
//}
//
//void CReadOBJ::Draw()
//{
//	XMMATRIX world;
//	world = XMMatrixScaling(m_Scale.x, m_Scale.y, m_Scale.z);
//	world *= XMMatrixRotationRollPitchYaw(m_Rotation.x, m_Rotation.y, m_Rotation.z);
//	world *= XMMatrixTranslation(m_Position.x, m_Position.y, m_Position.z);
//	//CRenderer::SetWorldMatrix( &world );
//
//	XMFLOAT4X4 worldf;
//	DirectX::XMStoreFloat4x4(&worldf, world);
//
//	m_Shader->SetWorldMatrix(&worldf);
//
//	CCamera* camera = CCamera::GetInstance();
//	m_Shader->SetViewMatrix(&camera->GetViewMatrix());
//	m_Shader->SetProjectionMatrix(&camera->GetProjectionMatrix());
//	m_Shader->SetCameraPosition(&camera->GetPosition());
//
//	XMFLOAT4 parameter;
//	parameter.x = 0.f;
//	parameter.y = 0.5f;
//	parameter.z = 1.f;
//	m_Shader->SetPrameter(parameter);
//
//	CRenderDevice::SetTexture(m_Texture, 1);
//
//	CRenderDevice::SetVertexBuffers(m_VertexBuffer);
//
//	CRenderDevice::SetIndexBuffer(m_IndexBuffer);
//
//	m_Shader->Bind();
//
//	for (USHORT i = 0; i < m_SubsetNum; i++)
//	{
//		//CRenderDevice::SetMaterial( m_SubsetArray[i].Material.Material );
//
//		CRenderDevice::SetTexture(m_SubsetArray[i].Material.Texture);
//
//		CRenderDevice::DrawIndexed(m_SubsetArray[i].IndexNum, m_SubsetArray[i].StartIndex, 0);
//	}
//
//	for (USHORT l = 0; l < 20; l++)
//	{
//		XMFLOAT4 parameter;
//		parameter.x = l * 0.02f;
//		parameter.y = l / 19.f;
//		parameter.z = 0.f;
//		m_Shader->SetPrameter(parameter);
//		m_Shader->Bind();
//
//		for (USHORT i = 0; i < m_SubsetNum; i++)
//		{
//			//CRenderDevice::SetMaterial( m_SubsetArray[i].Material.Material );
//
//			CRenderDevice::SetTexture(m_SubsetArray[i].Material.Texture);
//
//			CRenderDevice::DrawIndexed(m_SubsetArray[i].IndexNum, m_SubsetArray[i].StartIndex, 0);
//		}
//	}
//}
//
//void CReadOBJ::DrawShadow()
//{
//	XMMATRIX world;
//	world = XMMatrixScaling(m_Scale.x, m_Scale.y, m_Scale.z);
//	world *= XMMatrixRotationRollPitchYaw(m_Rotation.x, m_Rotation.y, m_Rotation.z);
//	world *= XMMatrixTranslation(m_Position.x, m_Position.y, m_Position.z);
//	//CRenderer::SetWorldMatrix( &world );
//
//	XMFLOAT4X4 worldf;
//	DirectX::XMStoreFloat4x4(&worldf, world);
//
//	m_Shader->SetWorldMatrix(&worldf);
//
//	CCamera* camera = CCamera::GetInstance();
//	m_Shader->SetViewMatrix(&camera->GetViewMatrix());
//	m_Shader->SetProjectionMatrix(&camera->GetProjectionMatrix());
//	m_Shader->SetCameraPosition(&camera->GetPosition());
//
//	XMFLOAT4 parameter;
//	parameter.x = 0.f;
//	parameter.y = 0.5f;
//	parameter.z = 1.f;
//	m_Shader->SetPrameter(parameter);
//
//	CRenderDevice::SetTexture(m_Texture, 1);
//
//	CRenderDevice::SetVertexBuffers(m_VertexBuffer);
//
//	CRenderDevice::SetIndexBuffer(m_IndexBuffer);
//
//	m_Shader->Bind();
//
//	for (USHORT i = 0; i < m_SubsetNum; i++)
//	{
//		//CRenderer::SetMaterial( m_SubsetArray[i].Material.Material );
//
//		CRenderDevice::SetTexture(m_SubsetArray[i].Material.Texture);
//
//		CRenderDevice::DrawIndexed(m_SubsetArray[i].IndexNum, m_SubsetArray[i].StartIndex, 0);
//	}
//}
//
//void CReadOBJ::Load(const CHAR* FileName)
//{
//	MODEL model;
//	LoadObj( FileName, &model );
//
//	{
//		D3D11_BUFFER_DESC bd;
//		ZeroMemory(&bd, sizeof(bd));
//		bd.Usage = D3D11_USAGE_DEFAULT;
//		bd.ByteWidth = sizeof(CustomStruct::CVertex3DData) * model.VertexNum;
//		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
//		bd.CPUAccessFlags = 0;
//
//		D3D11_SUBRESOURCE_DATA sd;
//		ZeroMemory(&sd, sizeof(sd));
//		sd.pSysMem = model.VertexArray;
//
//		CRenderDevice::GetDevice()->CreateBuffer(&bd, &sd, &m_VertexBuffer);
//	}
//
//	{
//		D3D11_BUFFER_DESC bd;
//		ZeroMemory(&bd, sizeof(bd));
//		bd.Usage = D3D11_USAGE_DEFAULT;
//		bd.ByteWidth = sizeof(USHORT) * model.IndexNum;
//		bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
//		bd.CPUAccessFlags = 0;
//
//		D3D11_SUBRESOURCE_DATA sd;
//		ZeroMemory(&sd, sizeof(sd));
//		sd.pSysMem = model.IndexArray;
//
//		CRenderDevice::GetDevice()->CreateBuffer(&bd, &sd, &m_IndexBuffer);
//	}
//
//	{
//		m_SubsetArray = new DX11_SUBSET[model.SubsetNum];
//		m_SubsetNum = model.SubsetNum;
//
//		for (USHORT i = 0; i < model.SubsetNum; i++)
//		{
//			m_SubsetArray[i].StartIndex = model.SubsetArray[i].StartIndex;
//			m_SubsetArray[i].IndexNum = model.SubsetArray[i].IndexNum;
//
//			m_SubsetArray[i].Material.Material = model.SubsetArray[i].Material.Material;
//
//			m_SubsetArray[i].Material.Texture = new CTexture();
//			m_SubsetArray[i].Material.Texture->Load(model.SubsetArray[i].Material.TextureName);
//		}
//	}
//
//	delete[] model.VertexArray;
//	delete[] model.IndexArray;
//	delete[] model.SubsetArray;
//}
//
//void CReadOBJ::Unload()
//{
//	m_VertexBuffer->Release();
//	m_IndexBuffer->Release();
//
//	delete[] m_SubsetArray;
//}
//
//void CReadOBJ::LoadObj(const CHAR* FileName, MODEL* Model)
//{
//	XMFLOAT3* positionArray;
//	XMFLOAT3* normalArray;
//	XMFLOAT2* texcoordArray;
//
//	USHORT	positionNum = 0;
//	USHORT	normalNum = 0;
//	USHORT	texcoordNum = 0;
//	USHORT	vertexNum = 0;
//	USHORT	indexNum = 0;
//	USHORT	in = 0;
//	USHORT	subsetNum = 0;
//
//	MODEL_MATERIAL* materialArray = NULL;
//	USHORT materialNum = 0;
//
//	CHAR str[256];
//	CHAR* s;
//	CHAR c;
//
//	FILE* file;
//	file = fopen(FileName, "rt");
//	assert(file);
//
//	while (true)
//	{
//		fscanf(file, "%s", str);
//
//		if (feof(file) != 0)
//			break;
//
//		if (strcmp(str, "v") == 0)
//		{
//			positionNum++;
//		}
//		else if (strcmp(str, "vn") == 0)
//		{
//			normalNum++;
//		}
//		else if (strcmp(str, "vt") == 0)
//		{
//			texcoordNum++;
//		}
//		else if (strcmp(str, "usemtl") == 0)
//		{
//			subsetNum++;
//		}
//		else if (strcmp(str, "f") == 0)
//		{
//			in = 0;
//
//			do
//			{
//				fscanf(file, "%s", str);
//				vertexNum++;
//				in++;
//				c = fgetc(file);
//			} while (c != '\n' && c != '\r');
//
//			if (in == 4)
//				in = 6;
//
//			indexNum += in;
//		}
//	}
//
//	positionArray = new XMFLOAT3[positionNum];
//	normalArray = new XMFLOAT3[normalNum];
//	texcoordArray = new XMFLOAT2[texcoordNum];
//
//	Model->VertexArray = new CustomStruct::CVertex3DData[vertexNum];
//	Model->VertexNum = vertexNum;
//
//	Model->IndexArray = new USHORT[indexNum];
//	Model->IndexNum = indexNum;
//
//	Model->SubsetArray = new SUBSET[subsetNum];
//	Model->SubsetNum = subsetNum;
//
//	XMFLOAT3* position = positionArray;
//	XMFLOAT3* normal = normalArray;
//	XMFLOAT2* texcoord = texcoordArray;
//
//	USHORT vc = 0;
//	USHORT ic = 0;
//	USHORT sc = 0;
//
//	fseek(file, 0, SEEK_SET);
//
//	while (true)
//	{
//		fscanf(file, "%s", str);
//
//		if (feof(file) != 0)
//			break;
//
//		if (strcmp(str, "mtllib") == 0)
//		{
//			fscanf(file, "%s", str);
//
//			CHAR path[256];
//			strcpy(path, "data/model/");
//			strcat(path, str);
//
//			LoadMaterial(path, &materialArray, &materialNum);
//		}
//		else if (strcmp(str, "o") == 0)
//		{
//			fscanf(file, "%s", str);
//		}
//		else if (strcmp(str, "v") == 0)
//		{
//			fscanf(file, "%f", &position->x);
//			fscanf(file, "%f", &position->y);
//			fscanf(file, "%f", &position->z);
//			//position->x *= SCALE_MODEL;
//			//position->y *= SCALE_MODEL;
//			//position->z *= SCALE_MODEL;
//			position++;
//		}
//		else if (strcmp(str, "vn") == 0)
//		{
//			fscanf(file, "%f", &normal->x);
//			fscanf(file, "%f", &normal->y);
//			fscanf(file, "%f", &normal->z);
//			normal++;
//		}
//		else if (strcmp(str, "vt") == 0)
//		{
//			fscanf(file, "%f", &texcoord->x);
//			fscanf(file, "%f", &texcoord->y);
//			//texcoord->y = 1.0f - texcoord->y;
//			texcoord++;
//		}
//		else if (strcmp(str, "usemtl") == 0)
//		{
//			fscanf(file, "%s", str);
//
//			if (sc != 0)
//				Model->SubsetArray[sc - 1].IndexNum = ic - Model->SubsetArray[sc - 1].StartIndex;
//
//			Model->SubsetArray[sc].StartIndex = ic;
//
//			for (USHORT i = 0; i < materialNum; i++)
//			{
//				if (strcmp(str, materialArray[i].Name) == 0)
//				{
//					Model->SubsetArray[sc].Material.Material = materialArray[i].Material;
//					strcpy(Model->SubsetArray[sc].Material.TextureName, materialArray[i].TextureName);
//					strcpy(Model->SubsetArray[sc].Material.Name, materialArray[i].Name);
//
//					break;
//				}
//			}
//
//			sc++;
//		}
//		else if (strcmp(str, "f") == 0)
//		{
//			in = 0;
//
//			do
//			{
//				fscanf(file, "%s", str);
//
//				s = strtok(str, "/");
//				Model->VertexArray[vc].Position = positionArray[atoi(s) - 1];
//				if (s[strlen(s) + 1] != '/')
//				{
//					s = strtok(NULL, "/");
//					Model->VertexArray[vc].UV0 = texcoordArray[atoi(s) - 1];
//				}
//				s = strtok(NULL, "/");
//				Model->VertexArray[vc].Normal = normalArray[atoi(s) - 1];
//
//				Model->VertexArray[vc].Color = XMFLOAT4(1.f, 1.f, 1.f, 1.f);
//
//				Model->IndexArray[ic] = vc;
//				ic++;
//				vc++;
//
//				in++;
//				c = fgetc(file);
//			} while (c != '\n' && c != '\r');
//
//			if (in == 4)
//			{
//				Model->IndexArray[ic] = vc - 4;
//				ic++;
//				Model->IndexArray[ic] = vc - 2;
//				ic++;
//			}
//		}
//	}
//
//	if (sc != 0)
//		Model->SubsetArray[sc - 1].IndexNum = ic - Model->SubsetArray[sc - 1].StartIndex;
//
//	delete[] positionArray;
//	delete[] normalArray;
//	delete[] texcoordArray;
//	delete[] materialArray;
//}
//
//void CReadOBJ::LoadMaterial(const CHAR* FileName, MODEL_MATERIAL** MaterialArray, USHORT* MaterialNum)
//{
//	CHAR str[256];
//
//	FILE* file;
//	file = fopen(FileName, "rt");
//	if (file == NULL)
//	{
//		assert(false);
//		return;
//	}
//
//	MODEL_MATERIAL* materialArray;
//	USHORT materialNum = 0;
//
//	while (true)
//	{
//		fscanf(file, "%s", str);
//
//		if (feof(file) != 0)
//			break;
//
//		if (strcmp(str, "newmtl") == 0)
//		{
//			materialNum++;
//		}
//	}
//
//	materialArray = new MODEL_MATERIAL[materialNum];
//
//	INT mc = -1;
//
//	fseek(file, 0, SEEK_SET);
//
//	while (true)
//	{
//		fscanf(file, "%s", str);
//
//		if (feof(file) != 0)
//			break;
//
//		if (strcmp(str, "newmtl") == 0)
//		{
//			mc++;
//			fscanf(file, "%s", materialArray[mc].Name);
//			strcpy(materialArray[mc].TextureName, "");
//		}
//		else if (strcmp(str, "Ka") == 0)
//		{
//			fscanf(file, "%f", &materialArray[mc].Material.Ambient.r);
//			fscanf(file, "%f", &materialArray[mc].Material.Ambient.g);
//			fscanf(file, "%f", &materialArray[mc].Material.Ambient.b);
//			materialArray[mc].Material.Ambient.a = 1.f;
//		}
//		else if (strcmp(str, "Kd") == 0)
//		{
//			fscanf(file, "%f", &materialArray[mc].Material.Diffuse.r);
//			fscanf(file, "%f", &materialArray[mc].Material.Diffuse.g);
//			fscanf(file, "%f", &materialArray[mc].Material.Diffuse.b);
//			materialArray[mc].Material.Diffuse.a = 1.f;
//		}
//		else if (strcmp(str, "Ks") == 0)
//		{
//			fscanf(file, "%f", &materialArray[mc].Material.Specular.r);
//			fscanf(file, "%f", &materialArray[mc].Material.Specular.g);
//			fscanf(file, "%f", &materialArray[mc].Material.Specular.b);
//			materialArray[mc].Material.Specular.a = 1.f;
//		}
//		else if (strcmp(str, "Ns") == 0)
//		{
//			fscanf(file, "%f", &materialArray[mc].Material.Shininess);
//		}
//		else if (strcmp(str, "d") == 0)
//		{
//			fscanf(file, "%f", &materialArray[mc].Material.Diffuse.a);
//		}
//		else if (strcmp(str, "map_Kd") == 0)
//		{
//			fscanf(file, "%s", str);
//
//			CHAR path[256];
//			strcpy(path, "data/model/");
//			strcat(path, str);
//
//			strcat(materialArray[mc].TextureName, path);
//		}
//	}
//
//	*MaterialArray = materialArray;
//	*MaterialNum = materialNum;
//}