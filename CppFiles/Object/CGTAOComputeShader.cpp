#include "../../Headers/Object/CGTAOComputeShader.h"
#include "../../Headers/Base/CRenderDevice.h"
#include "../../Headers/Game/CDeferredBuffer.h"
#include "../../Headers/Game/CScene.h"
#include "../../Headers/Game/CCamera.h"
#include "../../Headers/Object/CScreenPolygon2D.h"

CGTAOComputeShader::CGTAOComputeShader()
{
	this->m_Polygon2D = NULL;
	this->m_UserArguments.NumAngles = 2u;
	this->m_UserArguments.FallOffEnd = 200.f;
	//this->m_UserArguments.FallOffStartRatio = 0.5f;
	this->m_UserArguments.ThicknessBlend = 0.5f;
	this->m_UserArguments.FadeRadius = 5000.f;
	this->m_UserArguments.FadeDistance = 8000.f;
	this->m_UserArguments.Intensity = 0.5f;
	this->m_UserArguments.Power = 2.f;

	this->m_DebugType = 0;
}
CGTAOComputeShader::~CGTAOComputeShader()
{
	if (this->m_Polygon2D != NULL)
	{
		delete (this->m_Polygon2D);
		this->m_Polygon2D = NULL;
	}
}
void CGTAOComputeShader::Init()
{
	CCamera* sceneCamera = this->m_Scene->GetGameObjectFirst<CCamera>(CScene::SCENELAYOUT_CAMERA);
	CustomType::Vector4 viewPort = sceneCamera->GetViewport();

	CRenderDevice::CreateConstantBuffer(this->m_ConstantBuffer, sizeof(GTAOConstantBuffer));

	CRenderDevice::CreateRenderTexture2D(this->m_IntegralBuffer, static_cast<UINT>(viewPort.Z()), static_cast<UINT>(viewPort.W()), DXGI_FORMAT_R8G8B8A8_UNORM, TRUE);
	CRenderDevice::CreateRenderTexture2D(this->m_FilterBuffer, static_cast<UINT>(viewPort.Z()), static_cast<UINT>(viewPort.W()), DXGI_FORMAT_R8_UNORM, TRUE);
	this->m_Polygon2D = new CScreenPolygon2D(ENGINE_SHADER_SCREEN_POLYGON_2D_VS, ENGINE_SHADER_SCREEN_POLYGON_2D_PS, viewPort);
	this->m_Polygon2D->SetScene(this->m_Scene);
	this->m_Polygon2D->SetParent(this);
	this->m_Polygon2D->Init();

	CRenderDevice::LoadComputeShader("./HLSLCSO/GTAOSpatialIntegral.cso", this->m_IntegralComputeShader);
	CRenderDevice::LoadComputeShader("./HLSLCSO/GTAOSpatialFilter.cso", this->m_FilterComputeShader);
	
	CRenderDevice::LoadComputeShader("./HLSLCSO/GTAODebugType.cso", this->m_DebugComputeShader);
	CRenderDevice::CreateRenderTexture2D(this->m_DebugBuffer, static_cast<UINT>(viewPort.Z()), static_cast<UINT>(viewPort.W()), DXGI_FORMAT_R8G8B8A8_UNORM, TRUE);
}
void CGTAOComputeShader::Uninit()
{
	if (this->m_Polygon2D != NULL)
	{
		delete (this->m_Polygon2D);
		this->m_Polygon2D = NULL;
	}
}
void CGTAOComputeShader::Update()
{
	{
		INT tempNumAngles = this->m_UserArguments.NumAngles;

		ImGui::Begin("GTAO shader parameters");
		ImGui::SliderInt("Debug type", &(this->m_DebugType), 0, 3);
		ImGui::SliderInt("Angles number", &(tempNumAngles), 1, 16);
		ImGui::SliderFloat("Fall off end", &(this->m_UserArguments.FallOffEnd), 0.f, 1000.f);
		//ImGui::SliderFloat("Fall off start ratio", &(this->m_UserArguments.FallOffStartRatio), 0.f, 0.999f);
		ImGui::SliderFloat("Thickness blend", &(this->m_UserArguments.ThicknessBlend), 0.f, 1.f);
		ImGui::SliderFloat("Fade radius", &(this->m_UserArguments.FadeRadius), 0.f, 200000.f);
		ImGui::SliderFloat("Fade distance", &(this->m_UserArguments.FadeDistance), 0.f, 200000.f);
		ImGui::SliderFloat("Intensity", &(this->m_UserArguments.Intensity), 0.f, 1.f);
		ImGui::SliderFloat("Power", &(this->m_UserArguments.Power), 0.1f, 8.f);
		ImGui::End();

		this->m_UserArguments.NumAngles = tempNumAngles;
	}

	CCamera* sceneCamera = this->m_Scene->GetGameObjectFirst<CCamera>(CScene::SCENELAYOUT_CAMERA);
	CustomType::Vector4Int viewPort(sceneCamera->GetViewport());
	FLOAT bufferSizeW = static_cast<FLOAT>(viewPort.Z() - viewPort.X());
	FLOAT bufferSizeH = static_cast<FLOAT>(viewPort.W() - viewPort.Y());

	FLOAT fallOffEnd = this->m_UserArguments.FallOffEnd;
	//FLOAT fallOffStartRatio = this->m_UserArguments.FallOffStartRatio;
	//FLOAT fallOffStart = fallOffEnd * fallOffStartRatio;
	//FLOAT fallOffStartSq = fallOffStart * fallOffStart;
	//FLOAT fallOffEndSq = fallOffEnd * fallOffEnd;

	//FLOAT fallOffScale = 1.f / (fallOffEndSq - fallOffStartSq);
	//FLOAT fallOffBias = -fallOffStartSq * fallOffScale;

	FLOAT thicknessBlend = this->m_UserArguments.ThicknessBlend;
	thicknessBlend = CustomType::CMath::Clamp(1.f - (thicknessBlend * thicknessBlend), 0.f, 0.99f);

	FLOAT numAngles = static_cast<FLOAT>(this->m_UserArguments.NumAngles);
	FLOAT sinDeltaAngle, cosDeltaAngle;
	CustomType::CMath::SinCos(sinDeltaAngle, cosDeltaAngle, CustomType::CMath::GetPI() / numAngles);

	this->m_ConstantData.AdjustAngleThicknessParams = XMFLOAT4(fallOffEnd * bufferSizeH * sceneCamera->GetProjectionMatrix().GetXMFLOAT4X4()._11, sinDeltaAngle, cosDeltaAngle, thicknessBlend);

	FLOAT fadeRadius = CustomType::CMath::Max(1.f, this->m_UserArguments.FadeRadius);
	FLOAT invFadeRadius = 1.f / fadeRadius;

	this->m_ConstantData.FadeAttenParams = XMFLOAT4(invFadeRadius, -(this->m_UserArguments.FadeDistance - fadeRadius) * invFadeRadius, this->m_UserArguments.FadeDistance, 2.f / (fallOffEnd * fallOffEnd));

	this->m_ConstantData.ResultBufferParams = XMFLOAT4(bufferSizeW, bufferSizeH, 1.f / bufferSizeW, 1.f / bufferSizeH);
	this->m_ConstantData.DepthBufferParams = XMFLOAT4(bufferSizeW, bufferSizeH, 1.f / bufferSizeW, 1.f / bufferSizeH);
	this->m_ConstantData.IntensityPowerParams = XMFLOAT4(this->m_UserArguments.Intensity, this->m_UserArguments.Power * 0.5f, static_cast<FLOAT>(this->m_DebugType), 0.f);
}
void CGTAOComputeShader::PrepareDraw()
{
	CCamera* sceneCamera = this->m_Scene->GetGameObjectFirst<CCamera>(CScene::SCENELAYOUT_CAMERA);
	CustomType::Vector4Int viewPort(sceneCamera->GetViewport());

	CRenderDevice::UploadConstantBuffer(this->m_ConstantBuffer, &(this->m_ConstantData));
	CRenderDevice::BindComputeShaderConstantBuffer(this->m_ConstantBuffer, 1u);

	CRenderDevice::BindComputeShaderResourceView(CRenderDevice::GetDeferredBuffer()->GetDepthStencilShaderResourceView(CDeferredBuffer::DEPTHSTENCILBUFFER_CAMERA), 0u);
	CRenderDevice::BindComputeUnorderedAccessView(this->m_IntegralBuffer.UnorderedAccessView, 0u);
	CRenderDevice::GetDeviceContext()->CSSetShader(this->m_IntegralComputeShader.Get(), NULL, 0u);
	CRenderDevice::GetDeviceContext()->Dispatch((viewPort.Z() + 7) / 8, (viewPort.W() + 7) / 8, 1u);
	CRenderDevice::BindComputeUnorderedAccessView(NULL, 0u);
	CRenderDevice::GetDeviceContext()->CSSetShader(NULL, NULL, 0u);

	CRenderDevice::BindComputeShaderResourceView(this->m_IntegralBuffer.ShaderResourceView, 0u);
	CRenderDevice::BindComputeUnorderedAccessView(this->m_FilterBuffer.UnorderedAccessView, 0u);
	CRenderDevice::GetDeviceContext()->CSSetShader(this->m_FilterComputeShader.Get(), NULL, 0u);
	CRenderDevice::GetDeviceContext()->Dispatch((viewPort.Z() + 7) / 8, (viewPort.W() + 7) / 8, 1u);
	CRenderDevice::BindComputeUnorderedAccessView(NULL, 0u);
	CRenderDevice::GetDeviceContext()->CSSetShader(NULL, NULL, 0u);

	//Debug
	{
		CRenderDevice::BindComputeShaderResourceView(this->m_IntegralBuffer.ShaderResourceView, 0u);
		CRenderDevice::BindComputeShaderResourceView(this->m_FilterBuffer.ShaderResourceView, 1u);
		CRenderDevice::BindComputeUnorderedAccessView(this->m_DebugBuffer.UnorderedAccessView, 0u);
		CRenderDevice::GetDeviceContext()->CSSetShader(this->m_DebugComputeShader.Get(), NULL, 0u);
		CRenderDevice::GetDeviceContext()->Dispatch((viewPort.Z() + 7) / 8, (viewPort.W() + 7) / 8, 1u);
		CRenderDevice::BindComputeUnorderedAccessView(NULL, 0u);
		CRenderDevice::GetDeviceContext()->CSSetShader(NULL, NULL, 0u);
	}

	CRenderDevice::BindTexture(this->m_DebugBuffer.ShaderResourceView, ENGINE_TEXTURE2D_ALBEDO_START_SLOT);
	//CRenderDevice::BindTexture(this->m_FilterBuffer.ShaderResourceView, ENGINE_TEXTURE2D_ALBEDO_START_SLOT);
}
void CGTAOComputeShader::Draw()
{
	if (this->m_DebugType != 0)
	{
		this->PrepareDraw();
		this->m_Polygon2D->Draw();
	}
}