#include "../../Entry/MyMain.h"
#include "../../Headers/Object/CSky.h"
#include "../../Headers/Base/CInput.h"
#include "../../Headers/Base/CRenderDevice.h"
#include "../../Headers/Base/CShader.h"
#include "../../Headers/Base/CTexture.h"
#include "../../Headers/Game/CCamera.h"

void CSky::Init()
{
	SetTransparent(GAMEOBJECT_SKY);

	FLOAT sizeXZ = 75.f;
	FLOAT sizeY = 9.5f;

	for (INT z = 0; z < (INT)SKY_Z; z++)
	{
		for (INT x = 0; x < (INT)SKY_X; x++)
		{
			m_Vertex[z * SKY_X + x].Position.x = sinf((FLOAT)x / (SKY_X - 1) * XM_2PI) * (sinf((FLOAT)z / (SKY_Z - 1) * XM_PI)) * sizeXZ;
			m_Vertex[z * SKY_X + x].Position.z = cosf((FLOAT)x / (SKY_X - 1) * XM_2PI) * (sinf((FLOAT)z / (SKY_Z - 1) * XM_PI)) * sizeXZ;
			m_Vertex[z * SKY_X + x].Position.y = cosf((FLOAT)z / (SKY_Z - 1) * XM_PI) * sizeY;
			m_Vertex[z * SKY_X + x].Color = XMFLOAT4(0.f, 0.f, 0.f, 1.f);
			m_Vertex[z * SKY_X + x].UV0 = XMFLOAT2((FLOAT)x, (FLOAT)z);
			m_Vertex[z * SKY_X + x].Normal.x = m_Vertex[z * SKY_X + x].Position.x / sizeXZ;
			m_Vertex[z * SKY_X + x].Normal.y = m_Vertex[z * SKY_X + x].Position.y / sizeXZ;
			m_Vertex[z * SKY_X + x].Normal.z = m_Vertex[z * SKY_X + x].Position.z / sizeY;
		}
	}

	{
		D3D11_BUFFER_DESC bd;
		ZeroMemory(&bd, sizeof(bd));
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = sizeof(CustomStruct::CVertex3DData) * SKY_X * SKY_Z;
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bd.CPUAccessFlags = 0;

		D3D11_SUBRESOURCE_DATA sd;
		ZeroMemory(&sd, sizeof(sd));
		sd.pSysMem = m_Vertex;

		CRenderDevice::GetDevice()->CreateBuffer(&bd, &sd, &m_VertexBuffer);
	}

	UINT index[(SKY_X * 2 + 2) * (SKY_Z - 1) - 2];

	UINT i = 0;
	for (INT z = 0; z < SKY_Z - 1; z++)
	{
		for (INT x = 0; x < SKY_X; x++)
		{
			index[i] = (z + 1) * SKY_X + x;
			i++;
			index[i] = z * SKY_X + x;
			i++;
		}

		if (z == SKY_Z - 2)
			break;

		index[i] = z * SKY_X + SKY_X - 1;
		i++;
		index[i] = (z + 2) * SKY_X;
		i++;
	}

	{
		D3D11_BUFFER_DESC bd;
		ZeroMemory(&bd, sizeof(bd));
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = sizeof(UINT) * ((SKY_X * 2 + 2) * (SKY_Z - 1) - 2);
		bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		bd.CPUAccessFlags = 0;

		D3D11_SUBRESOURCE_DATA sd;
		ZeroMemory(&sd, sizeof(sd));
		sd.pSysMem = index;

		CRenderDevice::GetDevice()->CreateBuffer(&bd, &sd, &m_IndexBuffer);
	}

	m_Position = XMFLOAT3( 0.f, 0.f, 0.f );
	m_Rotation = XMFLOAT3( 0.f, 0.f, 0.f );
	m_Scale = XMFLOAT3( 1.f, 1.f, 1.f );

	m_Shader = new CShader();
	m_Shader->Init("./HLSLCSO/SkyVS.cso", "./HLSLCSO/SkyPS.cso");

	m_Time = 0.f;

	m_LightRotation = 0.f;
	m_LightDirection = XMFLOAT3(0.f, 0.f, 0.f);
}

void CSky::Uninit()
{
	m_Shader->Uninit();
	delete m_Shader;

	m_VertexBuffer->Release();
	m_IndexBuffer->Release();
}

void CSky::Update()
{
	m_Time += 1.f / 60.f;

	if (CInput::GetKeyPress('W'))
		m_LightRotation += 0.01f;
	if (CInput::GetKeyPress('S'))
		m_LightRotation -= 0.01f;

	m_LightDirection = XMFLOAT3(0.f, -cosf(m_LightRotation), sinf(m_LightRotation));
}

void CSky::Draw()
{
	UINT stride = sizeof(CustomStruct::CVertex3DData);
	UINT offset = 0;
	CRenderDevice::GetDeviceContext()->IASetVertexBuffers( 0, 1, &m_VertexBuffer, &stride, &offset );

	CRenderDevice::GetDeviceContext()->IASetIndexBuffer(m_IndexBuffer, DXGI_FORMAT_R32_UINT, 0);

	XMMATRIX world;
	world = XMMatrixScaling( m_Scale.x, m_Scale.y, m_Scale.z );
	world *= XMMatrixRotationRollPitchYaw( m_Rotation.x, m_Rotation.y, m_Rotation.z );
	world *= XMMatrixTranslation( m_Position.x, m_Position.y, m_Position.z );

	XMFLOAT4X4 worldf;
	DirectX::XMStoreFloat4x4(&worldf, world);

	m_Shader->SetWorldMatrix(&worldf);

	CCamera* camera = CCamera::GetInstance();
	m_Shader->SetViewMatrix(&camera->GetViewMatrix());
	m_Shader->SetProjectionMatrix(&camera->GetProjectionMatrix());
	m_Shader->SetCameraPosition(&camera->GetPosition());

	m_Shader->SetPrameter(XMFLOAT4(m_Time, m_LightDirection.x, m_LightDirection.y, m_LightDirection.z));

	m_Shader->Bind();

	CRenderDevice::GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	CRenderDevice::GetDeviceContext()->DrawIndexed(((SKY_X * 2 + 2) * (SKY_Z - 1) - 2), 0, 0);
}