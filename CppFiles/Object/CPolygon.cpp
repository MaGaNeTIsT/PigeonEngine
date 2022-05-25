#include "../../Entry/MyMain.h"
#include "../../Headers/Object/CPolygon.h"
#include "../../Headers/Base/CInput.h"
#include "../../Headers/Base/CRenderDevice.h"
#include "../../Headers/Base/CShader.h"
#include "../../Headers/Base/CTexture2D.h"

//void CPolygon::Init()
//{
//	CustomStruct::CVertex3DData vertex[4];
//
//	//vertex[0].Position = XMFLOAT3( 100.f, 100.f, 0.f );
//	//vertex[0].Color  = XMFLOAT4( 1.f, 0.f, 0.f, 1.f );
//
//	//vertex[1].Position = XMFLOAT3( 300.f, 100.f, 0.f );
//	//vertex[1].Color  = XMFLOAT4( 0.f, 1.f, 0.f, 1.f );
//
//	//vertex[2].Position = XMFLOAT3( 100.f, 300.f, 0.f );
//	//vertex[2].Color  = XMFLOAT4( 0.f, 0.f, 1.f, 1.f );
//
//	//vertex[3].Position = XMFLOAT3( 300.f, 300.f, 0.f );
//	//vertex[3].Color  = XMFLOAT4( 1.f, 1.f, 1.f, 1.f );
//
//	vertex[0].Position = XMFLOAT3(0.f, 0.f, 0.f);
//	vertex[0].Color = XMFLOAT4(1.f, 1.f, 1.f, 1.f);
//	vertex[0].UV0 = XMFLOAT2(0.f, 0.f);
//
//	vertex[1].Position = XMFLOAT3(200.f, 0.f, 0.f);
//	vertex[1].Color = XMFLOAT4(1.f, 1.f, 1.f, 1.f);
//	vertex[1].UV0 = XMFLOAT2(10.f, 0.f);
//
//	vertex[2].Position = XMFLOAT3(0.f, 200.f, 0.f);
//	vertex[2].Color = XMFLOAT4(1.f, 1.f, 1.f, 1.f);
//	vertex[2].UV0 = XMFLOAT2(0.f, 10.f);
//
//	vertex[3].Position = XMFLOAT3(200.f, 200.f, 0.f);
//	vertex[3].Color = XMFLOAT4(1.f, 1.f, 1.f, 1.f);
//	vertex[3].UV0 = XMFLOAT2(10.f, 10.f);
//
//	D3D11_BUFFER_DESC bd;
//	ZeroMemory( &bd, sizeof(bd) );
//	bd.Usage = D3D11_USAGE_DEFAULT;
//	bd.ByteWidth = sizeof(CustomStruct::CVertex3DData) * 4;
//	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
//	bd.CPUAccessFlags = 0;
//
//	D3D11_SUBRESOURCE_DATA sd;
//	ZeroMemory( &sd, sizeof(sd) );
//	sd.pSysMem = vertex;
//
//	CRenderDevice::GetDevice()->CreateBuffer( &bd, &sd, &m_VertexBuffer );
//
//	m_Shader = new CShader();
//	m_Shader->Init( "./HLSLCSO/2DTextureVS.cso", "./HLSLCSO/2DTexturePS.cso" );
//	
//	m_Texture = new CTexture();
//	m_Texture->Load( "./Assets/Textures/Sky/lutSky.tga" );
//
//	m_Blend = XMFLOAT4( 0.f, 0.f, 0.f, 0.f );
//}
//
//void CPolygon::Uninit()
//{
//	m_Shader->Uninit();
//	delete m_Shader;
//
//	m_Texture->Unload();
//	delete m_Texture;
//
//	m_VertexBuffer->Release();
//}
//
//void CPolygon::Update()
//{
//	if (CInput::GetKeyPress(VK_UP))
//		m_Blend.x += 0.01f;
//	if (CInput::GetKeyPress(VK_DOWN))
//		m_Blend.x -= 0.01f;
//}
//
//void CPolygon::Draw()
//{
//	UINT stride = sizeof(CustomStruct::CVertex3DData);
//	UINT offset = 0;
//	CRenderDevice::GetDeviceContext()->IASetVertexBuffers( 0, 1, &m_VertexBuffer, &stride, &offset );
//
//	XMFLOAT4X4 identity;
//	DirectX::XMStoreFloat4x4(&identity, XMMatrixIdentity());
//
//	m_Shader->SetWorldMatrix(&identity);
//	m_Shader->SetViewMatrix(&identity);
//
//	XMFLOAT4X4 projection;
//	DirectX::XMStoreFloat4x4(&projection, XMMatrixOrthographicOffCenterLH(0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, 0.0f, 0.0f, 1.0f));
//	m_Shader->SetProjectionMatrix(&projection);
//
//	m_Shader->SetPrameter( m_Blend );
//
//	m_Shader->Bind();
//
//	CRenderDevice::SetTexture( m_Texture, 0 );
//	//CRenderer::SetDepthTexture(1);
//
//	CRenderDevice::GetDeviceContext()->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP );
//
//	CRenderDevice::GetDeviceContext()->Draw( 4, 0 );
//}