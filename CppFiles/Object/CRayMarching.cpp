#include "../../Entry/MyMain.h"
#include "../../Headers/Object/CRayMarching.h"
#include "../../Headers/Base/CInput.h"
#include "../../Headers/Base/CShader.h"
#include "../../Headers/Game/CCamera.h"

void CRayMarching::Init()
{

	VERTEX_3D Vertex[24];
	UINT Indices[36];

	for (int i = 0; i < 24; ++i)
	{
		Vertex[i].Diffuse = XMFLOAT4(0.f, 0.f, 0.f, 1.f);
	}

	XMFLOAT3 Normal[8];
	Normal[0] = XMFLOAT3(-1.f, 1.f, -1.f);
	Normal[1] = XMFLOAT3(1.f, 1.f, -1.f);
	Normal[2] = XMFLOAT3(-1.f, -1.f, -1.f);
	Normal[3] = XMFLOAT3(1.f, -1.f, -1.f);
	Normal[4] = XMFLOAT3(1.f, 1.f, 1.f);
	Normal[5] = XMFLOAT3(-1.f, 1.f, 1.f);
	Normal[6] = XMFLOAT3(1.f, -1.f, 1.f);
	Normal[7] = XMFLOAT3(-1.f, -1.f, 1.f);
	XMVECTOR vecNormal = XMVectorSet(0.f, 0.f, 0.f, 0.f);
	for (int i = 0; i < 8; ++i)
	{
		vecNormal = XMLoadFloat3(&(Normal[i]));
		vecNormal = XMVector3Normalize(vecNormal);
		XMStoreFloat3(&(Normal[i]), vecNormal);
		vecNormal = XMVectorSet(0.f, 0.f, 0.f, 0.f);
	}

	FLOAT length = 5.f;

	Vertex[0].Position = XMFLOAT3(-length, length, -length);
	Vertex[1].Position = XMFLOAT3(length, length, -length);
	Vertex[2].Position = XMFLOAT3(-length, -length, -length);
	Vertex[3].Position = XMFLOAT3(length, -length, -length);
	Vertex[0].Normal = Normal[0];
	Vertex[1].Normal = Normal[1];
	Vertex[2].Normal = Normal[2];
	Vertex[3].Normal = Normal[3];
	Vertex[0].TexCoord = XMFLOAT2(0.f, 0.f);
	Vertex[1].TexCoord = XMFLOAT2(1.f, 0.f);
	Vertex[2].TexCoord = XMFLOAT2(0.f, 1.f);
	Vertex[3].TexCoord = XMFLOAT2(1.f, 1.f);

	Vertex[4].Position = XMFLOAT3(length, length, length);
	Vertex[5].Position = XMFLOAT3(-length, length, length);
	Vertex[6].Position = XMFLOAT3(length, -length, length);
	Vertex[7].Position = XMFLOAT3(-length, -length, length);
	Vertex[4].Normal = Normal[4];
	Vertex[5].Normal = Normal[5];
	Vertex[6].Normal = Normal[6];
	Vertex[7].Normal = Normal[7];
	Vertex[4].TexCoord = XMFLOAT2(0.f, 0.f);
	Vertex[5].TexCoord = XMFLOAT2(1.f, 0.f);
	Vertex[6].TexCoord = XMFLOAT2(0.f, 1.f);
	Vertex[7].TexCoord = XMFLOAT2(1.f, 1.f);

	Vertex[8].Position = Vertex[1].Position;
	Vertex[9].Position = Vertex[4].Position;
	Vertex[10].Position = Vertex[3].Position;
	Vertex[11].Position = Vertex[6].Position;
	Vertex[8].Normal = Normal[1];
	Vertex[9].Normal = Normal[4];
	Vertex[10].Normal = Normal[3];
	Vertex[11].Normal = Normal[6];
	Vertex[8].TexCoord = XMFLOAT2(0.f, 0.f);
	Vertex[9].TexCoord = XMFLOAT2(1.f, 0.f);
	Vertex[10].TexCoord = XMFLOAT2(0.f, 1.f);
	Vertex[11].TexCoord = XMFLOAT2(1.f, 1.f);

	Vertex[12].Position = Vertex[1].Position;
	Vertex[13].Position = Vertex[0].Position;
	Vertex[14].Position = Vertex[4].Position;
	Vertex[15].Position = Vertex[5].Position;
	Vertex[12].Normal = Normal[1];
	Vertex[13].Normal = Normal[0];
	Vertex[14].Normal = Normal[4];
	Vertex[15].Normal = Normal[5];
	Vertex[12].TexCoord = XMFLOAT2(0.f, 0.f);
	Vertex[13].TexCoord = XMFLOAT2(1.f, 0.f);
	Vertex[14].TexCoord = XMFLOAT2(0.f, 1.f);
	Vertex[15].TexCoord = XMFLOAT2(1.f, 1.f);

	Vertex[16].Position = Vertex[5].Position;
	Vertex[17].Position = Vertex[0].Position;
	Vertex[18].Position = Vertex[7].Position;
	Vertex[19].Position = Vertex[2].Position;
	Vertex[16].Normal = Normal[5];
	Vertex[17].Normal = Normal[0];
	Vertex[18].Normal = Normal[7];
	Vertex[19].Normal = Normal[2];
	Vertex[16].TexCoord = XMFLOAT2(0.f, 0.f);
	Vertex[17].TexCoord = XMFLOAT2(1.f, 0.f);
	Vertex[18].TexCoord = XMFLOAT2(0.f, 1.f);
	Vertex[19].TexCoord = XMFLOAT2(1.f, 1.f);

	Vertex[20].Position = Vertex[2].Position;
	Vertex[21].Position = Vertex[3].Position;
	Vertex[22].Position = Vertex[7].Position;
	Vertex[23].Position = Vertex[6].Position;
	Vertex[20].Normal = Normal[2];
	Vertex[21].Normal = Normal[3];
	Vertex[22].Normal = Normal[7];
	Vertex[23].Normal = Normal[6];
	Vertex[20].TexCoord = XMFLOAT2(0.f, 0.f);
	Vertex[21].TexCoord = XMFLOAT2(1.f, 0.f);
	Vertex[22].TexCoord = XMFLOAT2(0.f, 1.f);
	Vertex[23].TexCoord = XMFLOAT2(1.f, 1.f);

	Indices[0] = 0; Indices[1] = 1; Indices[2] = 2; Indices[3] = 1; Indices[4] = 3; Indices[5] = 2;
	Indices[6] = 4; Indices[7] = 5; Indices[8] = 6; Indices[9] = 5; Indices[10] = 7; Indices[11] = 6;
	Indices[12] = 8; Indices[13] = 9; Indices[14] = 10; Indices[15] = 9; Indices[16] = 11; Indices[17] = 10;
	Indices[18] = 12; Indices[19] = 13; Indices[20] = 14; Indices[21] = 13; Indices[22] = 15; Indices[23] = 14;
	Indices[24] = 16; Indices[25] = 17; Indices[26] = 18; Indices[27] = 17; Indices[28] = 19; Indices[29] = 18;
	Indices[30] = 20; Indices[31] = 21; Indices[32] = 22; Indices[33] = 21; Indices[34] = 23; Indices[35] = 22;

	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(VERTEX_3D) * 24;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.pSysMem = Vertex;

	CRenderDevice::GetDevice()->CreateBuffer(&bd, &sd, &m_VertexBuffer);

	D3D11_BUFFER_DESC ibd;
	ZeroMemory(&ibd, sizeof(ibd));
	ibd.Usage = D3D11_USAGE_DEFAULT;
	ibd.ByteWidth = sizeof(UINT) * 36;
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	ibd.MiscFlags = 0;
	ibd.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA isd;
	ZeroMemory(&isd, sizeof(isd));
	isd.pSysMem = Indices;
	CRenderDevice::GetDevice()->CreateBuffer(&ibd, &isd, &m_IndexBuffer);

	D3D11_RASTERIZER_DESC rsd0;
	rsd0.FillMode = D3D11_FILL_SOLID;
	rsd0.CullMode = D3D11_CULL_NONE;
	rsd0.FrontCounterClockwise = FALSE;
	rsd0.DepthBias = D3D11_DEFAULT_DEPTH_BIAS;
	rsd0.DepthBiasClamp = D3D11_DEFAULT_DEPTH_BIAS_CLAMP;
	rsd0.SlopeScaledDepthBias = D3D11_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	rsd0.DepthClipEnable = TRUE;
	rsd0.ScissorEnable = FALSE;
	rsd0.MultisampleEnable = FALSE;
	rsd0.AntialiasedLineEnable = FALSE;
	CRenderDevice::GetDevice()->CreateRasterizerState(&rsd0, &m_RenderStateNoCull);

	rsd0.CullMode = D3D11_CULL_BACK;
	CRenderDevice::GetDevice()->CreateRasterizerState(&rsd0, &m_RenderStateCull);

	m_Position = XMFLOAT3(0.f, 10.f, 0.f);
	m_Rotation = XMFLOAT3(0.f, 0.f, 0.f);
	m_Scale = XMFLOAT3(10.f, 0.5f, 10.f);

	m_Shader = new CShader();
	m_Shader->Init("./HLSLCSO/RayMarchingVS.cso", "./HLSLCSO/RayMarchingPS.cso");

	m_Time = 0.f;

	m_LightRotation = 0.f;
	m_LightDirection = XMFLOAT3(0.f, 0.f, 0.f);
}

void CRayMarching::Uninit()
{
	m_Shader->Uninit();
	delete m_Shader;

	m_VertexBuffer->Release();
	m_IndexBuffer->Release();
	m_RenderStateNoCull->Release();
	m_RenderStateCull->Release();
}

void CRayMarching::Update()
{
	m_Time += 1.f / 60.f;

	if (CInput::GetKeyPress('W'))
		m_LightRotation += 0.01f;
	if (CInput::GetKeyPress('S'))
		m_LightRotation -= 0.01f;

	m_LightDirection = XMFLOAT3(0.f, -cosf(m_LightRotation), sinf(m_LightRotation));
}

void CRayMarching::Draw()
{
	//CRenderer::GetDeviceContext()->RSSetState(m_RenderStateNoCull);

	UINT stride = sizeof(VERTEX_3D);
	UINT offset = 0;
	CRenderDevice::GetDeviceContext()->IASetVertexBuffers(0, 1, &m_VertexBuffer, &stride, &offset);
	CRenderDevice::GetDeviceContext()->IASetIndexBuffer(m_IndexBuffer, DXGI_FORMAT_R32_UINT, 0);

	XMMATRIX world;
	world = XMMatrixScaling(m_Scale.x, m_Scale.y, m_Scale.z);
	world *= XMMatrixRotationRollPitchYaw(m_Rotation.x, m_Rotation.y, m_Rotation.z);
	world *= XMMatrixTranslation(m_Position.x, m_Position.y, m_Position.z);

	XMFLOAT4X4 worldf;
	DirectX::XMStoreFloat4x4(&worldf, world);

	m_Shader->SetWorldMatrix(&worldf);

	CCamera* camera = CCamera::GetInstance();
	m_Shader->SetViewMatrix(&camera->GetViewMatrix());
	m_Shader->SetProjectionMatrix(&camera->GetProjectionMatrix());
	m_Shader->SetCameraPosition(&camera->GetPosition());

	XMFLOAT3 rotation = camera->GetRotation();
	XMMATRIX eyeMatrix = XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z);
	XMVECTOR eyeVec = XMVector3TransformNormal(XMVectorSet(0.f, 0.f, 1.f, 0.f), eyeMatrix);

	m_Shader->SetPrameter(XMFLOAT4(m_Time, m_LightDirection.x, m_LightDirection.y, m_LightDirection.z));

	XMVECTOR det;
	XMMATRIX worldInvTran = XMMatrixInverse(&det, world);
	worldInvTran = XMMatrixTranspose(worldInvTran);
	XMFLOAT4X4 worldInvTranf;
	DirectX::XMStoreFloat4x4(&worldInvTranf, worldInvTran);
	m_Shader->SetWorldInvTranspose(&worldInvTranf);

	m_Shader->Bind();

	CRenderDevice::GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	CRenderDevice::GetDeviceContext()->DrawIndexed(36, 0, 0);
	//CRenderer::GetDeviceContext()->RSSetState(m_RenderStateCull);
}