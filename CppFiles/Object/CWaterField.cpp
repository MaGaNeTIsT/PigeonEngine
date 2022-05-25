#include "../../Entry/MyMain.h"
#include "../../Headers/Object/CWaterField.h"
#include "../../Headers/Base/CInput.h"
#include "../../Headers/Base/CRenderDevice.h"
#include "../../Headers/Base/CShader.h"
#include "../../Headers/Base/CTexture2D.h"
#include "../../Headers/Game/CCamera.h"

//BOOL Linear_Interpolation(const FLOAT& x0, const FLOAT& y0, const FLOAT& x1, const FLOAT& y1, const FLOAT& t, FLOAT& Phi)
//{
//	if (t < x0 || t > x1)
//		return false;
//	Phi = ((FLOAT)(t - x1)) / ((FLOAT)(x0 - x1)) * y0 + ((FLOAT)(t - x0)) / ((FLOAT)(x1 - x0)) * y1;
//	return true;
//}
//
//CustomStruct::CVertex3DData* CreateWaterField(FLOAT wid, FLOAT len, UINT w, UINT l, INT u, INT v)
//{
//	FLOAT posx, posz, posy;
//	FLOAT tu, tv;
//	CustomStruct::CVertex3DData* data = new CustomStruct::CVertex3DData[w * l];
//
//	for (UINT y = 0; y < l; ++y)
//	{
//		for (UINT x = 0; x < w; ++x)
//		{
//			posx = 0.f; posz = 0.f; posy = 0.f;
//			Linear_Interpolation(0.f, -1.f * 0.5f * wid, (FLOAT)(w - 1), 0.5f * wid, (FLOAT)x, posx);
//			Linear_Interpolation(0.f, 0.5f * len, (FLOAT)(l - 1), -1.f * 0.5f * len, (FLOAT)y, posz);
//			data[y * w + x].Position = XMFLOAT3(posx, posy, posz);
//			data[y * w + x].Color = XMFLOAT4(1.f, 1.f, 1.f, 1.f);
//			data[y * w + x].Normal = XMFLOAT3(0.f, 1.f, 0.f);
//		}
//	}
//
//	for (UINT y = 0; y < l; ++y)
//	{
//		for (UINT x = 0; x < w; ++x)
//		{
//			tu = 0.f; tv = 0.f;
//			Linear_Interpolation(0.f, 0.f, (FLOAT)(w - 1), (FLOAT)u, (FLOAT)x, tu);
//			Linear_Interpolation(0.f, 0.f, (FLOAT)(l - 1), (FLOAT)v, (FLOAT)y, tv);
//			data[y * w + x].UV0 = XMFLOAT2(tu, tv);
//		}
//	}
//
//	return data;
//}
//
//UINT* GetWaterFieldIndices(UINT w, UINT l)
//{
//	UINT deltaY;
//	UINT x = w - 1;
//	UINT y = l - 1;
//	UINT Count = x * y * 2 * 3;
//	UINT verNum = 0;
//	UINT* Indices = new UINT[Count];
//	Count = x * y * 2;
//	x = x * 2;
//	for (UINT idx = 0; idx < Count; ++idx)
//	{
//		deltaY = idx / x;
//		if (idx % 2)
//		{
//			Indices[verNum] = (idx + 1) / 2 + deltaY;
//			verNum = verNum + 1;
//			Indices[verNum] = (idx + 1) / 2 + w + deltaY;
//			verNum = verNum + 1;
//			Indices[verNum] = (idx + 1) / 2 + w - 1 + deltaY;
//			verNum = verNum + 1;
//		}
//		else
//		{
//			Indices[verNum] = idx / 2 + deltaY;
//			verNum = verNum + 1;
//			Indices[verNum] = idx / 2 + 1 + deltaY;
//			verNum = verNum + 1;
//			Indices[verNum] = idx / 2 + w + deltaY;
//			verNum = verNum + 1;
//		}
//	}
//
//	return Indices;
//}
//
//void CWater::Init()
//{
//	SetTransparent();
//
//	CustomStruct::CVertex3DData* vertex;
//	UINT* Indices;
//	UINT w = 50, l = 50;
//	vertex = CreateWaterField(51.2f, 51.2f, w, l, 10, 10);
//	Indices = GetWaterFieldIndices(w, l);
//
//	D3D11_BUFFER_DESC bd;
//	ZeroMemory(&bd, sizeof(bd));
//	bd.Usage = D3D11_USAGE_DEFAULT;
//	bd.ByteWidth = sizeof(CustomStruct::CVertex3DData) * w * l;
//	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
//	bd.CPUAccessFlags = 0;
//
//	D3D11_SUBRESOURCE_DATA sd;
//	ZeroMemory(&sd, sizeof(sd));
//	sd.pSysMem = vertex;
//
//	CRenderDevice::GetDevice()->CreateBuffer(&bd, &sd, &m_VertexBuffer);
//	delete[]vertex;
//
//	D3D11_BUFFER_DESC ibd;
//	ZeroMemory(&ibd, sizeof(ibd));
//	ibd.Usage = D3D11_USAGE_DEFAULT;
//	ibd.ByteWidth = sizeof(UINT) * (w - 1) * (l - 1) * 2 * 3;
//	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
//	ibd.CPUAccessFlags = 0;
//	ibd.MiscFlags = 0;
//	ibd.StructureByteStride = 0;
//
//	D3D11_SUBRESOURCE_DATA isd;
//	ZeroMemory(&isd, sizeof(isd));
//	isd.pSysMem = Indices;
//	CRenderDevice::GetDevice()->CreateBuffer(&ibd, &isd, &m_IndexBuffer);
//	delete[]Indices;
//
//	m_Texture = new CTexture();
//	m_Texture->Load("./Assets/Textures/Water/lutWater.tga");
//
//	m_Position = XMFLOAT3( 0.f, 0.f, 0.f );
//	m_Rotation = XMFLOAT3( 0.f, 0.f, 0.f );
//	m_Scale = XMFLOAT3( 1.f, 1.f, 1.f );
//
//	m_Shader = new CShader();
//	m_Shader->Init("./HLSLCSO/WaterFieldVS.cso", "./HLSLCSO/WaterFieldPS.cso");
//
//	m_Time = 0.f;
//
//	m_LightRotation = 0.f;
//	m_LightDirection = XMFLOAT3(0.f, 0.f, 0.f);
//}
//
//void CWater::Uninit()
//{
//	m_Shader->Uninit();
//	delete m_Shader;
//
//	m_VertexBuffer->Release();
//	m_IndexBuffer->Release();
//
//	m_Texture->Unload();
//	delete m_Texture;
//}
//
//void CWater::Update()
//{
//	m_Time += 1.f / 60.f;
//
//	if (CInput::GetKeyPress('W'))
//		m_LightRotation += 0.01f;
//	if (CInput::GetKeyPress('S'))
//		m_LightRotation -= 0.01f;
//
//	m_LightDirection = XMFLOAT3(0.f, -cosf(m_LightRotation), sinf(m_LightRotation));
//}
//
//void CWater::Draw()
//{
//	UINT stride = sizeof(CustomStruct::CVertex3DData);
//	UINT offset = 0;
//	CRenderDevice::GetDeviceContext()->IASetVertexBuffers(0, 1, &m_VertexBuffer, &stride, &offset);
//	CRenderDevice::GetDeviceContext()->IASetIndexBuffer(m_IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
//
//	CRenderDevice::SetTexture(m_Texture, 6);
//
//	XMMATRIX world;
//	world = XMMatrixScaling( m_Scale.x, m_Scale.y, m_Scale.z );
//	world *= XMMatrixRotationRollPitchYaw( m_Rotation.x, m_Rotation.y, m_Rotation.z );
//	world *= XMMatrixTranslation( m_Position.x, m_Position.y, m_Position.z );
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
//	XMFLOAT3 rotation = camera->GetRotation();
//	XMMATRIX eyeMatrix = XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z);
//	XMVECTOR eyeVec = XMVector3TransformNormal(XMVectorSet(0.f, 0.f, 1.f, 0.f), eyeMatrix);
//
//	m_Shader->SetPrameter(XMFLOAT4(m_Time, m_LightDirection.x, m_LightDirection.y, m_LightDirection.z));
//
//	XMVECTOR det;
//	XMMATRIX worldInvTran = XMMatrixInverse(&det, world);
//	worldInvTran = XMMatrixTranspose(worldInvTran);
//	XMFLOAT4X4 worldInvTranf;
//	DirectX::XMStoreFloat4x4(&worldInvTranf, worldInvTran);
//	m_Shader->SetWorldInvTranspose(&worldInvTranf);
//
//	m_Shader->SetPrameter2(XMFLOAT4(static_cast<FLOAT>(SCREEN_WIDTH), static_cast<FLOAT>(SCREEN_HEIGHT), 0.f, 0.f));
//
//	XMVECTOR LightPos = -XMVector3Normalize(XMVectorSet(m_LightDirection.x, m_LightDirection.y, m_LightDirection.z, 0.f)) * 30.f;
//	XMMATRIX matLightView = XMMatrixLookAtLH(LightPos,
//		XMVectorSet(0.f, 0.f, 0.f, 0.f), XMVectorSet(1.f, 0.f, 0.f, 0.f));
//	XMMATRIX matLightProj = XMMatrixOrthographicLH(80.f, 80.f, 1.f, 80.f);
//	XMFLOAT4X4 mat, mat1;
//	XMStoreFloat4x4(&mat, matLightView);
//	XMStoreFloat4x4(&mat1, matLightProj);
//	m_Shader->SetLightMatrix(&mat, &mat1);
//
//	m_Shader->Bind();
//
//	CRenderDevice::GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
//
//	INT w = 50, l = 50;
//
//	//CRenderer::SetRasterizerState(RSE_FRONT);
//
//	//CRenderer::GetDeviceContext()->DrawIndexed((w - 1) * (l - 1) * 2 * 3, 0, 0);
//
//	//CRenderer::SetRasterizerState(RSE_BACK);
//
//	CRenderDevice::GetDeviceContext()->DrawIndexed((w - 1) * (l - 1) * 2 * 3, 0, 0);
//}