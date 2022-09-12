#include "../Headers/CHZBBuffer.h"
#include "../../RenderBase/Headers/CRenderDevice.h"
#include "../../RenderBase/Headers/CDeferredBuffer.h"
#include "../../../EngineGame/Headers/CCamera.h"
#include "../../../EngineGame/Headers/CScene.h"
#include "../../../EngineGame/Headers/CScreenPolygon2D.h"

CHZBBuffer::CHZBBuffer()
{
	this->m_Polygon2D = NULL;

	this->m_HZBMipLevels = 0;
}
CHZBBuffer::~CHZBBuffer()
{
	if (this->m_Polygon2D != NULL)
	{
		delete (this->m_Polygon2D);
		this->m_Polygon2D = NULL;
	}
}
void CHZBBuffer::Init()
{
	CCamera* sceneCamera = this->m_Scene->GetGameObjectFirst<CCamera>(CScene::SCENELAYOUT_CAMERA);
	CustomType::Vector4Int viewport = sceneCamera->GetViewport();

	this->m_HZBMipLevels = 1;

	CRenderDevice::CreateRenderTexture2D(this->m_HZBBuffer, static_cast<UINT>(viewport.Z()), static_cast<UINT>(viewport.W()), DXGI_FORMAT_R32_FLOAT, TRUE, D3D11_USAGE_DEFAULT, static_cast<UINT>(this->m_HZBMipLevels), 1u);

	this->m_Polygon2D = new CScreenPolygon2D(ENGINE_SHADER_SCREEN_POLYGON_2D_VS, ENGINE_SHADER_SCREEN_POLYGON_2D_PS, CustomType::Vector4(viewport));
	this->m_Polygon2D->SetScene(this->m_Scene);
	this->m_Polygon2D->SetParent(this);
	this->m_Polygon2D->Init();

	CRenderDevice::LoadComputeShader("./Engine/Assets/EngineShaders/BuildHZBComputeShader.cso", this->m_BuildHZBComputeShader);
}
void CHZBBuffer::Uninit()
{
	if (this->m_Polygon2D != NULL)
	{
		delete (this->m_Polygon2D);
		this->m_Polygon2D = NULL;
	}
}
void CHZBBuffer::Update()
{

}
void CHZBBuffer::PrepareDraw()
{
	CCamera* sceneCamera = this->m_Scene->GetGameObjectFirst<CCamera>(CScene::SCENELAYOUT_CAMERA);
	CustomType::Vector4Int viewport = sceneCamera->GetViewport();

	CRenderDevice::BindComputeShaderResourceView(CRenderDevice::GetDeferredBuffer()->GetDepthStencilShaderResourceView(CDeferredBuffer::DEPTHSTENCILBUFFER_CAMERA), 0u);
	CRenderDevice::BindComputeUnorderedAccessView(this->m_HZBBuffer.UnorderedAccessView, 0u);
	CRenderDevice::GetDeviceContext()->CSSetShader(this->m_BuildHZBComputeShader.Get(), NULL, 0u);
	CRenderDevice::GetDeviceContext()->Dispatch(static_cast<UINT>((viewport.Z() + 7) / 8), static_cast<UINT>((viewport.W() + 7) / 8), 1u);
	CRenderDevice::BindComputeUnorderedAccessView(NULL, 0u);
	CRenderDevice::GetDeviceContext()->CSSetShader(NULL, NULL, 0u);
}
void CHZBBuffer::Draw()
{
	this->PrepareDraw();
	this->m_Polygon2D->Draw();
}