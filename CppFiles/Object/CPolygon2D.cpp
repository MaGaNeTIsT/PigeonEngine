#include "../../Entry/MyMain.h"
#include "../../Headers/Object/CPolygon2D.h"
#include "../../Headers/Base/CInput.h"
#include "../../Headers/Base/CShader.h"
#include "../../Headers/Game/CCamera.h"

void CPolygon2D::Init()
{
	VERTEX_3D vertex[4];
	vertex[0].Position = XMFLOAT3(m_Position.x, m_Position.y, 0.f);
	vertex[0].Diffuse = XMFLOAT4(0.f, 0.f, 0.f, 1.f);
	vertex[0].TexCoord = XMFLOAT2(0.f, 0.f);
	vertex[1].Position = XMFLOAT3(m_Position.z, m_Position.y, 0.f);
	vertex[1].Diffuse = XMFLOAT4(0.f, 0.f, 0.f, 1.f);
	vertex[1].TexCoord = XMFLOAT2(1.f, 0.f);
	vertex[2].Position = XMFLOAT3(m_Position.x, m_Position.w, 0.f);
	vertex[2].Diffuse = XMFLOAT4(0.f, 0.f, 0.f, 1.f);
	vertex[2].TexCoord = XMFLOAT2(0.f, 1.f);
	vertex[3].Position = XMFLOAT3(m_Position.z, m_Position.w, 0.f);
	vertex[3].Diffuse = XMFLOAT4(0.f, 0.f, 0.f, 1.f);
	vertex[3].TexCoord = XMFLOAT2(1.f, 1.f);

	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(VERTEX_3D) * 4;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.pSysMem = vertex;
	CRenderDevice::GetDevice()->CreateBuffer(&bd, &sd, &m_VertexBuffer);

	m_Shader = new CShader();
	m_Shader->Init("./HLSLCSO/Polygon2DVS.cso", "./HLSLCSO/Polygon2DPS.cso");

	XMFLOAT4X4 identity;
	DirectX::XMStoreFloat4x4(&identity, XMMatrixIdentity());
	m_Shader->SetWorldMatrix(&identity);
	m_Shader->SetWorldInvTranspose(&identity);
	m_Shader->SetPrameter2(XMFLOAT4(static_cast<FLOAT>(SCREEN_WIDTH), static_cast<FLOAT>(SCREEN_HEIGHT), 0.f, 0.f));

	m_Time = 0.f;

	m_LightRotation = 0.f;
	m_LightDirection = XMFLOAT3(0.f, 0.f, 0.f);
}

void CPolygon2D::Uninit()
{
	m_Shader->Uninit();
	delete m_Shader;

	m_VertexBuffer->Release();
}

void CPolygon2D::Update()
{
	m_Time += 1.f / 60.f;

	if (CInput::GetKeyPress('W'))
		m_LightRotation += 0.01f;
	if (CInput::GetKeyPress('S'))
		m_LightRotation -= 0.01f;

	m_LightDirection = XMFLOAT3(0.f, -cosf(m_LightRotation), sinf(m_LightRotation));
}

void CPolygon2D::Draw()
{
	UINT stride = sizeof(VERTEX_3D);
	UINT offset = 0;
	CRenderDevice::GetDeviceContext()->IASetVertexBuffers(0, 1, &m_VertexBuffer, &stride, &offset);

	CCamera* camera = CCamera::GetInstance();
	m_Shader->SetViewMatrix(&camera->GetViewMatrix());
	m_Shader->SetProjectionMatrix(&camera->GetProjectionMatrix());
	m_Shader->SetCameraPosition(&camera->GetPosition());
	m_Shader->SetPrameter(XMFLOAT4(m_Time, m_LightDirection.x, m_LightDirection.y, m_LightDirection.z));

	m_Shader->Bind();

	CRenderDevice::GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	CRenderDevice::GetDeviceContext()->Draw(4, 0);
}

void CPolygon2D::SetScreenPosition(XMFLOAT4 rect)
{
	m_Position = rect;
}

XMFLOAT4 CPolygon2D::GetScreenPosition()
{
	return m_Position;
}

void DebugQuadAlpha::Init()
{
	VERTEX_3D vertex[4];
	vertex[0].Position = XMFLOAT3(m_Position.x, m_Position.y, 0.f);
	vertex[0].Diffuse = XMFLOAT4(0.f, 0.f, 0.f, 1.f);
	vertex[0].TexCoord = XMFLOAT2(0.f, 0.f);
	vertex[1].Position = XMFLOAT3(m_Position.z, m_Position.y, 0.f);
	vertex[1].Diffuse = XMFLOAT4(0.f, 0.f, 0.f, 1.f);
	vertex[1].TexCoord = XMFLOAT2(1.f, 0.f);
	vertex[2].Position = XMFLOAT3(m_Position.x, m_Position.w, 0.f);
	vertex[2].Diffuse = XMFLOAT4(0.f, 0.f, 0.f, 1.f);
	vertex[2].TexCoord = XMFLOAT2(0.f, 1.f);
	vertex[3].Position = XMFLOAT3(m_Position.z, m_Position.w, 0.f);
	vertex[3].Diffuse = XMFLOAT4(0.f, 0.f, 0.f, 1.f);
	vertex[3].TexCoord = XMFLOAT2(1.f, 1.f);

	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(VERTEX_3D) * 4;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.pSysMem = vertex;
	CRenderDevice::GetDevice()->CreateBuffer(&bd, &sd, &m_VertexBuffer);

	m_Shader = new CShader();
	m_Shader->Init("./HLSLCSO/Polygon2DVS.cso", "./HLSLCSO/DebugPolygon2DAlpha.cso");

	XMFLOAT4X4 identity;
	DirectX::XMStoreFloat4x4(&identity, XMMatrixIdentity());
	m_Shader->SetWorldMatrix(&identity);
	m_Shader->SetWorldInvTranspose(&identity);
	m_Shader->SetPrameter2(XMFLOAT4(static_cast<FLOAT>(SCREEN_WIDTH), static_cast<FLOAT>(SCREEN_HEIGHT), 0.f, 0.f));

	m_Time = 0.f;

	m_LightRotation = 0.f;
	m_LightDirection = XMFLOAT3(0.f, 0.f, 0.f);
}

void QuadBefore::Init()
{
	VERTEX_3D vertex[4];
	vertex[0].Position = XMFLOAT3(m_Position.x, m_Position.y, 0.f);
	vertex[0].Diffuse = XMFLOAT4(0.f, 0.f, 0.f, 1.f);
	vertex[0].TexCoord = XMFLOAT2(0.f, 0.f);
	vertex[1].Position = XMFLOAT3(m_Position.z, m_Position.y, 0.f);
	vertex[1].Diffuse = XMFLOAT4(0.f, 0.f, 0.f, 1.f);
	vertex[1].TexCoord = XMFLOAT2(1.f, 0.f);
	vertex[2].Position = XMFLOAT3(m_Position.x, m_Position.w, 0.f);
	vertex[2].Diffuse = XMFLOAT4(0.f, 0.f, 0.f, 1.f);
	vertex[2].TexCoord = XMFLOAT2(0.f, 1.f);
	vertex[3].Position = XMFLOAT3(m_Position.z, m_Position.w, 0.f);
	vertex[3].Diffuse = XMFLOAT4(0.f, 0.f, 0.f, 1.f);
	vertex[3].TexCoord = XMFLOAT2(1.f, 1.f);

	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(VERTEX_3D) * 4;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.pSysMem = vertex;
	CRenderDevice::GetDevice()->CreateBuffer(&bd, &sd, &m_VertexBuffer);

	m_Shader = new CShader();
	m_Shader->Init("./HLSLCSO/Polygon2DVS.cso", "./HLSLCSO/GBuffer.cso");

	XMFLOAT4X4 identity;
	DirectX::XMStoreFloat4x4(&identity, XMMatrixIdentity());
	m_Shader->SetWorldMatrix(&identity);
	m_Shader->SetWorldInvTranspose(&identity);
	m_Shader->SetPrameter2(XMFLOAT4(static_cast<FLOAT>(SCREEN_WIDTH), static_cast<FLOAT>(SCREEN_HEIGHT), 0.f, 0.f));

	m_Time = 0.f;

	m_LightRotation = 0.f;
	m_LightDirection = XMFLOAT3(0.f, 0.f, 0.f);
}

void QuadBefore::Draw()
{
	UINT stride = sizeof(VERTEX_3D);
	UINT offset = 0;
	CRenderDevice::GetDeviceContext()->IASetVertexBuffers(0, 1, &m_VertexBuffer, &stride, &offset);

	CCamera* camera = CCamera::GetInstance();
	m_Shader->SetViewMatrix(&camera->GetViewMatrix());
	m_Shader->SetProjectionMatrix(&camera->GetProjectionMatrix());
	m_Shader->SetCameraPosition(&camera->GetPosition());
	m_Shader->SetPrameter(XMFLOAT4(m_Time, m_LightDirection.x, m_LightDirection.y, m_LightDirection.z));
	
	XMVECTOR LightPos = -XMVector3Normalize(XMVectorSet(m_LightDirection.x, m_LightDirection.y, m_LightDirection.z, 0.f)) * 30.f;
	XMMATRIX matLightView = XMMatrixLookAtLH(LightPos,
		XMVectorSet(0.f, 0.f, 0.f, 0.f), XMVectorSet(1.f, 0.f, 0.f, 0.f));
	XMMATRIX matLightProj = XMMatrixOrthographicLH(80.f, 80.f, 1.f, 80.f);
	XMFLOAT4X4 mat, mat1;
	XMStoreFloat4x4(&mat, matLightView);
	XMStoreFloat4x4(&mat1, matLightProj);
	m_Shader->SetLightMatrix(&mat, &mat1);

	m_Shader->Bind();

	CRenderDevice::GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	CRenderDevice::GetDeviceContext()->Draw(4, 0);
}

void QuadAfter::Init()
{
	VERTEX_3D vertex[4];
	vertex[0].Position = XMFLOAT3(m_Position.x, m_Position.y, 0.f);
	vertex[0].Diffuse = XMFLOAT4(0.f, 0.f, 0.f, 1.f);
	vertex[0].TexCoord = XMFLOAT2(0.f, 0.f);
	vertex[1].Position = XMFLOAT3(m_Position.z, m_Position.y, 0.f);
	vertex[1].Diffuse = XMFLOAT4(0.f, 0.f, 0.f, 1.f);
	vertex[1].TexCoord = XMFLOAT2(1.f, 0.f);
	vertex[2].Position = XMFLOAT3(m_Position.x, m_Position.w, 0.f);
	vertex[2].Diffuse = XMFLOAT4(0.f, 0.f, 0.f, 1.f);
	vertex[2].TexCoord = XMFLOAT2(0.f, 1.f);
	vertex[3].Position = XMFLOAT3(m_Position.z, m_Position.w, 0.f);
	vertex[3].Diffuse = XMFLOAT4(0.f, 0.f, 0.f, 1.f);
	vertex[3].TexCoord = XMFLOAT2(1.f, 1.f);

	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(VERTEX_3D) * 4;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.pSysMem = vertex;
	CRenderDevice::GetDevice()->CreateBuffer(&bd, &sd, &m_VertexBuffer);

	m_Shader = new CShader();
	m_Shader->Init("./HLSLCSO/Polygon2DVS.cso", "./HLSLCSO/PostEffect.cso");

	XMFLOAT4X4 identity;
	DirectX::XMStoreFloat4x4(&identity, XMMatrixIdentity());
	m_Shader->SetWorldMatrix(&identity);
	m_Shader->SetWorldInvTranspose(&identity);
	m_Shader->SetPrameter2(XMFLOAT4(static_cast<FLOAT>(SCREEN_WIDTH), static_cast<FLOAT>(SCREEN_HEIGHT), 0.f, 0.f));

	m_Time = 0.f;

	m_LightRotation = 0.f;
	m_LightDirection = XMFLOAT3(0.f, 0.f, 0.f);
}