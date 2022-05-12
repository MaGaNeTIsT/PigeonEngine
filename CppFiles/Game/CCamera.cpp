#include "../../Entry/MyMain.h"
#include "../../Headers/Game/CCamera.h"
#include "../../Headers/Base/CInput.h"
#include "../../Headers/Base/CRenderDevice.h"

CCamera* CCamera::m_Instance = nullptr;

void CCamera::Init()
{
	m_Position = XMFLOAT3(0.f, 5.f, 0.f);
	m_Rotation = XMFLOAT3(0.f, 0.f, 0.f);

	m_Viewport.left = 0;
	m_Viewport.top = 0;
	m_Viewport.right = SCREEN_WIDTH;
	m_Viewport.bottom = SCREEN_HEIGHT;
}

void CCamera::Uninit()
{
}

void CCamera::Update()
{
	XMFLOAT3 left, front;

	left.x = cosf( m_Rotation.y );
	left.z = -sinf( m_Rotation.y );

	front.x = sinf( m_Rotation.y );
	front.z = cosf( m_Rotation.y );

	if (CInput::GetKeyPress('H'))
	{
		m_Position.x -= left.x * 0.1f;
		m_Position.z -= left.z * 0.1f;
	}
	if (CInput::GetKeyPress('K'))
	{
		m_Position.x += left.x * 0.1f;
		m_Position.z += left.z * 0.1f;
	}

	if (CInput::GetKeyPress('U'))
	{
		m_Position.x += front.x * 0.1f;
		m_Position.z += front.z * 0.1f;
	}
	if (CInput::GetKeyPress('J'))
	{
		m_Position.x -= front.x * 0.1f;
		m_Position.z -= front.z * 0.1f;
	}

	if (CInput::GetKeyPress('T'))
		m_Position.y += 0.05f;
	if (CInput::GetKeyPress('G'))
		m_Position.y -= 0.05f;

	if (CInput::GetKeyPress('Y'))
		m_Rotation.y -= 0.05f;
	if (CInput::GetKeyPress('I'))
		m_Rotation.y += 0.05f;

	if (CInput::GetKeyPress('O'))
		m_Rotation.x -= 0.05f;
	if (CInput::GetKeyPress('L'))
		m_Rotation.x += 0.05f;

	XMMATRIX invViewMatrix = XMMatrixRotationRollPitchYaw(m_Rotation.x, m_Rotation.y, m_Rotation.z);
	invViewMatrix *= XMMatrixTranslation(m_Position.x, m_Position.y, m_Position.z);

	XMVECTOR det;
	XMMATRIX viewMatrix = XMMatrixInverse(&det, invViewMatrix);

	DirectX::XMStoreFloat4x4(&m_InvViewMatrix, invViewMatrix);
	DirectX::XMStoreFloat4x4(&m_ViewMatrix, viewMatrix);

	XMMATRIX projectionMatrix = XMMatrixPerspectiveFovLH(60.f / 180.f * 3.1415926f, SCREEN_WIDTH / SCREEN_HEIGHT, 0.1f, 1000.f);

	DirectX::XMStoreFloat4x4(&m_ProjectionMatrix, projectionMatrix);
}

void CCamera::Draw()
{
}

void CCamera::DrawShadow()
{
	//D3D11_VIEWPORT dxViewport;
	//dxViewport.Width = (float)(m_Viewport.right - m_Viewport.left);
	//dxViewport.Height = (float)(m_Viewport.bottom - m_Viewport.top);
	//dxViewport.MinDepth = 0.f;
	//dxViewport.MaxDepth = 1.f;
	//dxViewport.TopLeftX = (float)m_Viewport.left;
	//dxViewport.TopLeftY = (float)m_Viewport.top;

	////CRenderer::GetDeviceContext()->RSSetViewports(1, &dxViewport);


	//XMMATRIX invViewMatrix = XMMatrixRotationRollPitchYaw(m_Rotation.x, m_Rotation.y, m_Rotation.z);
	//invViewMatrix *= XMMatrixTranslation(m_Position.x, m_Position.y, m_Position.z);

	//XMVECTOR det;
	//XMMATRIX viewMatrix = XMMatrixInverse(&det, invViewMatrix);

	//DirectX::XMStoreFloat4x4(&m_InvViewMatrix, invViewMatrix);
	//DirectX::XMStoreFloat4x4(&m_ViewMatrix, viewMatrix);


	//XMMATRIX projectionMatrix = XMMatrixPerspectiveFovLH(1.f, dxViewport.Width / dxViewport.Height, 1.f, 10.f);

	//DirectX::XMStoreFloat4x4(&m_ProjectionMatrix, projectionMatrix);
}