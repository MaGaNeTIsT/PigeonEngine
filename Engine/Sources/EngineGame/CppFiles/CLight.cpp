#include "../../../../Entry/EngineMain.h"
#include "../Headers/CLight.h"
#include "../Headers/CCamera.h"

CLightBase::CLightBase()
{
	this->m_Scale = 1.f;
	this->m_Color = CustomStruct::CColor(1.f, 1.f, 1.f, 1.f);
	this->m_Intensity = 1.f;
	this->m_ShadowInfo = nullptr;
}
CLightBase::CLightBase(const CLightBase& light)
{
	this->m_Position = light.m_Position;
	this->m_Rotation = light.m_Rotation;
	this->m_Scale = 1.f;
	this->m_Color = light.m_Color;
	this->m_Intensity = light.m_Intensity;
	this->m_ShadowInfo = light.m_ShadowInfo;
}
void CLightBase::GetColor(CustomStruct::CColor& color, FLOAT& intensity)
{
	color = this->m_Color;
	intensity = this->m_Intensity;
}
void CLightBase::SetColor(const CustomStruct::CColor& color, const FLOAT& intensity)
{
	this->m_Color = color;
	this->m_Intensity = intensity;
}
BOOL CLightBase::GetShadowSize(CustomType::Vector2Int& output)
{
	if (this->m_ShadowInfo != nullptr)
	{
		output = this->m_ShadowInfo->Size;
		return TRUE;
	}
	return FALSE;
}
BOOL CLightBase::SetShadowInfo(const CustomType::Vector2Int& shadowSize, const UINT& shadowDepth)
{
	if (this->m_ShadowInfo == nullptr)
	{
		this->m_ShadowInfo = std::shared_ptr<LightShadowInfo>(new LightShadowInfo());
	}
	else
	{
		if (this->m_ShadowInfo->Size == shadowSize && this->m_ShadowInfo->Depth == shadowDepth)
		{
			return TRUE;
		}
	}
	if (!CLightBase::CreateShadowTexture(this->m_ShadowInfo->Texture, shadowSize, shadowDepth))
	{
		this->m_ShadowInfo = nullptr;
		return FALSE;
	}
	this->m_ShadowInfo->Size = shadowSize;
	this->m_ShadowInfo->Depth = shadowDepth;
	return TRUE;
}
BOOL CLightBase::CreateShadowTexture(CRenderDevice::RenderTexture2DViewInfo& output, const CustomType::Vector2Int& shadowSize, const UINT& shadowDepth)
{
	return (CRenderDevice::CreateRenderTexture2D(
		output,
		CustomStruct::CRenderTextureDesc(
			shadowSize.X(),
			shadowSize.Y(),
			CustomStruct::CRenderBindFlag::BIND_NONE,
			CustomStruct::CRenderFormat::FORMAT_UNKNOWN,
			NULL, NULL, NULL,
			shadowDepth)));
}



CLightDirectional::CLightDirectional()
{
	this->m_CurrentCamera = NULL;
	this->m_FrameCounter = 0;
}
CLightDirectional::CLightDirectional(const CLightDirectional& light) : CLightBase(light)
{
	this->m_CurrentCamera = light.m_CurrentCamera;
	this->m_FrameCounter = light.m_FrameCounter;
	this->m_ViewMatrix[0] = light.m_ViewMatrix[0];
	this->m_ViewMatrix[1] = light.m_ViewMatrix[1];
}
void CLightDirectional::Update()
{
	this->m_FrameCounter = 1 - this->m_FrameCounter;

	if (this->m_CurrentCamera == NULL)
	{
		return;
	}
}
void CLightDirectional::SetCurrentCamera(class Camera* camera)
{
	this->m_CurrentCamera = camera;
}
CustomType::Matrix4x4 CLightDirectional::GetCurrentViewMatrix(const UINT& extraIndex)
{
	return (this->m_ViewMatrix[this->m_FrameCounter]);
}
CustomType::Matrix4x4 CLightDirectional::GetPreviousViewMatrix(const UINT& extraIndex)
{
	return (this->m_ViewMatrix[1 - this->m_FrameCounter]);
}



CLightPoint::CLightPoint()
{
	this->m_AttenuationExponent = 1.5f;
	this->m_Radius = 100.f;
}
CLightPoint::CLightPoint(const CLightPoint& light) : CLightBase(light)
{
	this->m_AttenuationExponent = light.m_AttenuationExponent;
	this->m_Radius = light.m_Radius;
}
CustomType::Matrix4x4 CLightPoint::GetCurrentViewMatrix(const UINT& extraIndex)
{
	return (CustomType::Matrix4x4::Identity());
}
CustomType::Matrix4x4 CLightPoint::GetPreviousViewMatrix(const UINT& extraIndex)
{
	return (CustomType::Matrix4x4::Identity());
}



CLightSpot::CLightSpot()
{
	this->m_Range = 100.f;
	this->m_HalfRadian = 30.f * CustomType::CMath::GetDegToRad();
	this->m_CosHalfRadian = CustomType::CMath::Cos(this->m_HalfRadian);
	this->m_FrameCounter = 0;
}
CLightSpot::CLightSpot(const CLightSpot& light) : CLightBase(light)
{
	this->m_Range = light.m_Range;
	this->m_HalfRadian = light.m_HalfRadian;
	this->m_CosHalfRadian = light.m_CosHalfRadian;
	this->m_ViewMatrix[0] = light.m_ViewMatrix[0];
	this->m_ViewMatrix[1] = light.m_ViewMatrix[1];
}
void CLightSpot::Update()
{
	this->m_FrameCounter = 1 - this->m_FrameCounter;

}
CustomType::Matrix4x4 CLightSpot::GetCurrentViewMatrix(const UINT& extraIndex)
{
	return (this->m_ViewMatrix[this->m_FrameCounter]);
}
CustomType::Matrix4x4 CLightSpot::GetPreviousViewMatrix(const UINT& extraIndex)
{
	return (this->m_ViewMatrix[1 - this->m_FrameCounter]);
}