#include "../../Entry/MyMain.h"
#include "../../Headers/Object/CField.h"
#include "../../Headers/Base/CInput.h"
#include "../../Headers/Base/CRenderDevice.h"
#include "../../Headers/Base/CTexture.h"
#include "../../Headers/Base/CShader.h"
#include "../../Headers/Game/CCamera.h"

void CField::Init()
{
	for (INT z = 0; z < FIELD_Z; z++)
	{
		for (INT x = 0; x < FIELD_X; x++)
		{
			m_Vertex[z * FIELD_X + x].Position.x = x / 2.f - FIELD_X / 2.f / 2;
			m_Vertex[z * FIELD_X + x].Position.z = -z / 2.f + FIELD_Z / 2.f / 2;
			m_Vertex[z * FIELD_X + x].Position.y = 0.f;
			m_Vertex[z * FIELD_X + x].Color = XMFLOAT4(1.f, 1.f, 1.f, 1.f);
			m_Vertex[z * FIELD_X + x].UV0 = XMFLOAT2(x / 2.f, z / 2.f);
			m_Vertex[z * FIELD_X + x].Normal = XMFLOAT3(0.f, 1.f, 0.f);
		}
	}

	{
		D3D11_BUFFER_DESC bd;
		ZeroMemory(&bd, sizeof(bd));
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = sizeof(CustomStruct::CVertex3D) * FIELD_X * FIELD_Z;
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bd.CPUAccessFlags = 0;

		D3D11_SUBRESOURCE_DATA sd;
		ZeroMemory(&sd, sizeof(sd));
		sd.pSysMem = m_Vertex;

		CRenderDevice::GetDevice()->CreateBuffer(&bd, &sd, &m_VertexBuffer);
	}

	UINT index[(FIELD_X * 2 + 2) * (FIELD_Z - 1) - 2];

	UINT i = 0;
	for (INT z = 0; z < FIELD_Z - 1; z++)
	{
		for (INT x = 0; x < FIELD_X; x++)
		{
			index[i] = (z + 1) * FIELD_X + x;
			i++;
			index[i] = z * FIELD_X + x;
			i++;
		}

		if (z == FIELD_Z - 2)
			break;

		index[i] = z * FIELD_X + FIELD_X - 1;
		i++;
		index[i] = (z + 2) * FIELD_X;
		i++;
	}

	{
		D3D11_BUFFER_DESC bd;
		ZeroMemory(&bd, sizeof(bd));
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = sizeof(unsigned int) * ((FIELD_X * 2 + 2) * (FIELD_Z - 1) - 2);
		bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		bd.CPUAccessFlags = 0;

		D3D11_SUBRESOURCE_DATA sd;
		ZeroMemory(&sd, sizeof(sd));
		sd.pSysMem = index;

		CRenderDevice::GetDevice()->CreateBuffer(&bd, &sd, &m_IndexBuffer);
	}

	m_Position = XMFLOAT3( 0.f, -2.f, 0.f );
	m_Rotation = XMFLOAT3( 0.f, 0.f, 0.f );
	m_Scale = XMFLOAT3( 1.f, 1.f, 1.f );

	m_Shader = new CShader[3];
	m_Shader[0].Init("./HLSLCSO/FieldVS.cso", "./HLSLCSO/FieldPS.cso");
	m_Shader[1].Init("./HLSLCSO/FieldVS.cso", "./HLSLCSO/FieldFrontPS.cso");
	m_Shader[2].Init("./HLSLCSO/FieldVS.cso", "./HLSLCSO/FieldDeferredPS.cso");

	m_LightRotation = 0.f;
	m_LightDirection = XMFLOAT3(0.f, 0.f, 0.f);
}

void CField::Uninit()
{
	m_Shader[0].Uninit();
	m_Shader[1].Uninit();
	m_Shader[2].Uninit();
	delete[]m_Shader;

	m_VertexBuffer->Release();
	m_IndexBuffer->Release();
}

void CField::Update()
{
	if (CInput::GetKeyPress('W'))
		m_LightRotation += 0.01f;
	if (CInput::GetKeyPress('S'))
		m_LightRotation -= 0.01f;

	m_LightDirection = XMFLOAT3(0.f, -cosf(m_LightRotation), sinf(m_LightRotation));
}

void CField::Draw()
{
	UINT stride = sizeof(CustomStruct::CVertex3D);
	UINT offset = 0;
	CRenderDevice::GetDeviceContext()->IASetVertexBuffers( 0, 1, &m_VertexBuffer, &stride, &offset );

	CRenderDevice::GetDeviceContext()->IASetIndexBuffer(m_IndexBuffer, DXGI_FORMAT_R32_UINT, 0);

	XMMATRIX world;
	world = XMMatrixScaling( m_Scale.x, m_Scale.y, m_Scale.z );
	world *= XMMatrixRotationRollPitchYaw( m_Rotation.x, m_Rotation.y, m_Rotation.z );
	world *= XMMatrixTranslation( m_Position.x, m_Position.y, m_Position.z );
	//CRenderer::SetWorldMatrix( &world );

	XMFLOAT4X4 worldf;
	DirectX::XMStoreFloat4x4(&worldf, world);

	m_Shader[0].SetWorldMatrix(&worldf);

	CCamera* camera = CCamera::GetInstance();
	m_Shader[0].SetViewMatrix(&camera->GetViewMatrix());
	m_Shader[0].SetProjectionMatrix(&camera->GetProjectionMatrix());
	m_Shader[0].SetCameraPosition(&camera->GetPosition());

	m_Shader[0].SetPrameter(XMFLOAT4(0.0f, m_LightDirection.x, m_LightDirection.y, m_LightDirection.z));

	m_Shader[0].Bind();

	CRenderDevice::GetDeviceContext()->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP );

	CRenderDevice::GetDeviceContext()->DrawIndexed(((FIELD_X * 2 + 2) * (FIELD_Z - 1) - 2), 0, 0);
}

void CField::DrawDeferred()
{
	UINT stride = sizeof(CustomStruct::CVertex3D);
	UINT offset = 0;
	CRenderDevice::GetDeviceContext()->IASetVertexBuffers(0, 1, &m_VertexBuffer, &stride, &offset);

	CRenderDevice::GetDeviceContext()->IASetIndexBuffer(m_IndexBuffer, DXGI_FORMAT_R32_UINT, 0);

	XMMATRIX world;
	world = XMMatrixScaling(m_Scale.x, m_Scale.y, m_Scale.z);
	world *= XMMatrixRotationRollPitchYaw(m_Rotation.x, m_Rotation.y, m_Rotation.z);
	world *= XMMatrixTranslation(m_Position.x, m_Position.y, m_Position.z);

	XMFLOAT4X4 worldf;
	DirectX::XMStoreFloat4x4(&worldf, world);

	m_Shader[2].SetWorldMatrix(&worldf);

	CCamera* camera = CCamera::GetInstance();
	m_Shader[2].SetViewMatrix(&camera->GetViewMatrix());
	m_Shader[2].SetProjectionMatrix(&camera->GetProjectionMatrix());
	m_Shader[2].SetCameraPosition(&camera->GetPosition());

	m_Shader[2].SetPrameter(XMFLOAT4(0.0f, m_LightDirection.x, m_LightDirection.y, m_LightDirection.z));

	m_Shader[2].Bind();

	CRenderDevice::GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	CRenderDevice::GetDeviceContext()->DrawIndexed(((FIELD_X * 2 + 2) * (FIELD_Z - 1) - 2), 0, 0);
}

void CField::DrawShadow()
{
	UINT stride = sizeof(CustomStruct::CVertex3D);
	UINT offset = 0;
	CRenderDevice::GetDeviceContext()->IASetVertexBuffers(0, 1, &m_VertexBuffer, &stride, &offset);

	CRenderDevice::GetDeviceContext()->IASetIndexBuffer(m_IndexBuffer, DXGI_FORMAT_R32_UINT, 0);

	XMMATRIX world;
	world = XMMatrixScaling(m_Scale.x, m_Scale.y, m_Scale.z);
	world *= XMMatrixRotationRollPitchYaw(m_Rotation.x, m_Rotation.y, m_Rotation.z);
	world *= XMMatrixTranslation(m_Position.x, m_Position.y, m_Position.z);

	XMFLOAT4X4 worldf;
	DirectX::XMStoreFloat4x4(&worldf, world);

	m_Shader[1].SetWorldMatrix(&worldf);

	XMVECTOR LightPos = -XMVector3Normalize(XMVectorSet(m_LightDirection.x, m_LightDirection.y, m_LightDirection.z, 0.0f)) * 30.0f;
	XMMATRIX matLightView = XMMatrixLookAtLH(LightPos,
		XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f), XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f));
	XMMATRIX matLightProj = XMMatrixOrthographicLH(80.0f, 80.0f, 1.0f, 80.0f);
	XMFLOAT4X4 mat;
	XMStoreFloat4x4(&mat, matLightView);
	m_Shader[1].SetViewMatrix(&mat);
	XMStoreFloat4x4(&mat, matLightProj);
	m_Shader[1].SetProjectionMatrix(&mat);

	m_Shader[1].Bind();

	CRenderDevice::GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	CRenderDevice::GetDeviceContext()->DrawIndexed(((FIELD_X * 2 + 2) * (FIELD_Z - 1) - 2), 0, 0);
}