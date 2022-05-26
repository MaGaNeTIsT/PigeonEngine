#include "../../Headers/Object/CDebugScreen.h"
#include "../../Headers/Base/CRenderDevice.h"
#include "../../Headers/Game/CDeferredBuffer.h"
#include "../../Headers/Game/CScene.h"
#include "../../Headers/Game/CCamera.h"
#include "../../Headers/Object/CScreenPolygon2D.h"

UINT CDebugScreen::DEBUGPOLYGON_COUNT = 6u;
CDebugScreen::CDebugScreen()
{
}
CDebugScreen::~CDebugScreen()
{
}
void CDebugScreen::Init()
{
	CCamera* sceneCamera = this->m_Scene->GetGameObjectFirst<CCamera>(CScene::SCENELAYOUT_CAMERA);
	CustomType::Vector4 sceneViewport = sceneCamera->GetViewport();
	INT length = static_cast<INT>((sceneViewport.W() - sceneViewport.Y()) / CDebugScreen::DEBUGPOLYGON_COUNT);
	INT polygonCount = static_cast<INT>(CDebugScreen::DEBUGPOLYGON_COUNT);
	for (INT i = 0; i < polygonCount; i++)
	{
		CustomType::Vector4 tempScreenAnchor = CustomType::Vector4(0, length * i, length, length * (i + 1));
		CScreenPolygon2D* tempScreenPolygon2D = new CScreenPolygon2D(ENGINE_SHADER_SCREEN_POLYGON_2D_VS, ENGINE_SHADER_SCREEN_POLYGON_2D_PS, tempScreenAnchor);
		tempScreenPolygon2D->SetScene(this->m_Scene);
		tempScreenPolygon2D->SetParent(this);
		tempScreenPolygon2D->Init();
		m_Polygons.push_back(tempScreenPolygon2D);
	}
}
void CDebugScreen::Uninit()
{
	UINT size = static_cast<UINT>(m_Polygons.size());
	if (size > 0u)
	{
		for (UINT i = 0u; i < size; i++)
		{
			if (m_Polygons[i] != NULL)
				delete (m_Polygons[i]);
		}
	}
	m_Polygons.clear();
	m_SRVs.clear();
}
void CDebugScreen::Update()
{
}
void CDebugScreen::PrepareDraw()
{
	if (static_cast<UINT>(m_SRVs.size()) != CDebugScreen::DEBUGPOLYGON_COUNT)
		m_SRVs.resize(static_cast<size_t>(CDebugScreen::DEBUGPOLYGON_COUNT));
	m_SRVs[0] = CRenderDevice::GetDeferredBuffer()->GetDeferredShaderResourceView(CDeferredBuffer::DEFERREDBUFFER_WORLDNORMAL);
	m_SRVs[1] = CRenderDevice::GetDeferredBuffer()->GetDeferredShaderResourceView(CDeferredBuffer::DEFERREDBUFFER_ALBEDO);
	m_SRVs[2] = CRenderDevice::GetDeferredBuffer()->GetDeferredShaderResourceView(CDeferredBuffer::DEFERREDBUFFER_PROPERTY);
	m_SRVs[3] = CRenderDevice::GetDeferredBuffer()->GetDeferredShaderResourceView(CDeferredBuffer::DEFERREDBUFFER_ID);
	m_SRVs[4] = CRenderDevice::GetDeferredBuffer()->GetDepthStencilShaderResourceView(CDeferredBuffer::DEPTHSTENCILBUFFER_LIGHT);
	m_SRVs[5] = CRenderDevice::GetDeferredBuffer()->GetDepthStencilShaderResourceView(CDeferredBuffer::DEPTHSTENCILBUFFER_CAMERA);
}
void CDebugScreen::Draw()
{
	this->PrepareDraw();
	for (UINT i = 0; i < CDebugScreen::DEBUGPOLYGON_COUNT; i++)
	{
		CRenderDevice::BindTexture(m_SRVs[i], ENGINE_TEXTURE2D_ALBEDO_START_SLOT);
		m_Polygons[i]->Draw();
	}
}