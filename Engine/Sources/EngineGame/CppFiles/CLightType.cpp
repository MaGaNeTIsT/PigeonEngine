#include "../../../../Entry/EngineMain.h"
#include "../Headers/CLightType.h"
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
	this->SetLightType(LightType::LIGHT_TYPE_DIRECTIONAL);
	this->m_CurrentCamera = NULL;
	this->m_FrameCounter = 0;
}
CLightDirectional::CLightDirectional(const CLightDirectional& light) : CLightBase(light)
{
	this->SetLightType(LightType::LIGHT_TYPE_DIRECTIONAL);
	this->m_CurrentCamera = light.m_CurrentCamera;
	this->m_FrameCounter = light.m_FrameCounter;
	this->m_ViewMatrix[0] = light.m_ViewMatrix[0];
	this->m_ViewMatrix[1] = light.m_ViewMatrix[1];
	this->m_CascadeSettings = light.m_CascadeSettings;
	this->m_CascadeInfo = light.m_CascadeInfo;
	this->m_ProjectionMatrices = light.m_ProjectionMatrices;
}
void CLightDirectional::Init()
{
}
void CLightDirectional::Update()
{
	this->m_FrameCounter = 1 - this->m_FrameCounter;

	if (this->m_CurrentCamera == NULL)
	{
		return;
	}

	CLightDirectional::GenerateCascadeProjectionMatrices(this->m_CurrentCamera, this);

	this->m_CurrentCamera = NULL;
}
void CLightDirectional::GenerateCascadeProjectionMatrices(CCamera* camera, CLightDirectional* light)
{
	INT& frameCounter = light->m_FrameCounter;
	UINT& currentLayerNum = light->m_CascadeInfo.LayerNum;
	std::vector<FLOAT>& currentDistances = light->m_CascadeInfo.Distances;
	std::vector<FLOAT>& currentBorders = light->m_CascadeInfo.Borders;
	std::vector<CustomType::Matrix4x4>& projectionMatrices = light->m_ProjectionMatrices;
	{
		CustomType::Matrix4x4& viewMatrix = light->m_ViewMatrix[frameCounter];
		viewMatrix = CustomType::Matrix4x4(camera->GetPosition(), light->GetRotation());
		viewMatrix = viewMatrix.Inverse();
	}

	{
		currentDistances.clear();
		currentBorders.clear();
		FLOAT totalDist = camera->GetFar() - camera->GetNear();
		UINT cascadeLayerNum = CustomType::CMath::Clamp(light->m_CascadeSettings.LayerNum, 1u, 4u);
		FLOAT* cascadeDistance = light->m_CascadeSettings.Distance;
		FLOAT* cascadeBorder = light->m_CascadeSettings.Border;
		for (UINT i = 0u; i < cascadeLayerNum; i++)
		{
			FLOAT currentDist = cascadeDistance[i];
			if (currentDist > 5.f)
			{
				totalDist = totalDist - currentDist;
				if (totalDist <= 0.f)
				{
					currentDist = totalDist + currentDist;
					currentDistances.push_back(currentDist);
					if (i < (cascadeLayerNum - 1u))
					{
						currentBorders.push_back(currentDist * cascadeBorder[i]);
					}
					break;
				}
				currentDistances.push_back(currentDist);
				if (i < (cascadeLayerNum - 1u))
				{
					currentBorders.push_back(currentDist * cascadeBorder[i]);
				}
			}
		}
		if (currentDistances.size() < 1u)
		{
			currentDistances.resize(1u);
		}
		cascadeLayerNum = currentDistances.size();
		if (totalDist > 0.f)
		{
			currentDistances[currentDistances.size() - 1u] += totalDist;
		}
	}

	if (projectionMatrices.size() != currentLayerNum)
	{
		projectionMatrices.resize(currentLayerNum * 2u);
	}

	{
		std::vector<CustomType::Vector3> frustumPoints(camera->GetCullingPlanePoint());
		FLOAT totalLayerDist = camera->GetFar() - camera->GetNear();
		std::vector<CustomType::Vector3> tempPoints;
		if (currentLayerNum > 1u)
		{
			tempPoints.resize((currentLayerNum - 1u));
		}
		for (size_t i = 0u; i < tempPoints.size(); i++)
		{
			FLOAT currentLayerDist = currentDistances[i];
			for (UINT layerIndex = 0u; layerIndex < i; layerIndex++)
			{
				currentLayerDist += currentDistances[layerIndex];
			}
			FLOAT t = currentLayerDist / totalLayerDist;
			tempPoints[i * 4u + 0u] = CustomType::Vector3::Lerp(frustumPoints[0u], frustumPoints[4u], t);
			tempPoints[i * 4u + 1u] = CustomType::Vector3::Lerp(frustumPoints[1u], frustumPoints[5u], t);
			tempPoints[i * 4u + 2u] = CustomType::Vector3::Lerp(frustumPoints[2u], frustumPoints[6u], t);
			tempPoints[i * 4u + 3u] = CustomType::Vector3::Lerp(frustumPoints[3u], frustumPoints[7u], t);
		}
		for (UINT i = 0u; i < currentLayerNum; i++)
		{

		}
	}
}
void CLightDirectional::PreRenderInitLight(CCamera* camera)
{
	this->m_CurrentCamera = camera;
}
void CLightDirectional::SetCascadeInfo(const ShadowCascadeSettings& settings)
{
	this->m_CascadeSettings = settings;
}
CustomType::Matrix4x4 CLightDirectional::GetCurrentMatrix(const UINT& extraIndex)
{
	return (this->m_ViewMatrix[this->m_FrameCounter]);
}
CustomType::Matrix4x4 CLightDirectional::GetPreviousMatrix(const UINT& extraIndex)
{
	return (this->m_ViewMatrix[1 - this->m_FrameCounter]);
}



CLightPoint::CLightPoint()
{
	this->SetLightType(LightType::LIGHT_TYPE_POINT);
	this->m_AttenuationExponent = 1.5f;
	this->m_Radius = 100.f;
}
CLightPoint::CLightPoint(const CLightPoint& light) : CLightBase(light)
{
	this->SetLightType(LightType::LIGHT_TYPE_POINT);
	this->m_AttenuationExponent = light.m_AttenuationExponent;
	this->m_Radius = light.m_Radius;
}
CustomType::Matrix4x4 CLightPoint::GetCurrentMatrix(const UINT& extraIndex)
{
	return (CustomType::Matrix4x4::Identity());
}
CustomType::Matrix4x4 CLightPoint::GetPreviousMatrix(const UINT& extraIndex)
{
	return (CustomType::Matrix4x4::Identity());
}



CLightSpot::CLightSpot()
{
	this->SetLightType(LightType::LIGHT_TYPE_SPOT);
	this->m_Range = 100.f;
	this->m_HalfRadian = 30.f * CustomType::CMath::GetDegToRad();
	this->m_CosHalfRadian = CustomType::CMath::Cos(this->m_HalfRadian);
	this->m_FrameCounter = 0;
}
CLightSpot::CLightSpot(const CLightSpot& light) : CLightBase(light)
{
	this->SetLightType(LightType::LIGHT_TYPE_SPOT);
	this->m_Range = light.m_Range;
	this->m_HalfRadian = light.m_HalfRadian;
	this->m_CosHalfRadian = light.m_CosHalfRadian;
	this->m_ViewProjectionMatrix[0] = light.m_ViewProjectionMatrix[0];
	this->m_ViewProjectionMatrix[1] = light.m_ViewProjectionMatrix[1];
}
void CLightSpot::Update()
{
	this->m_FrameCounter = 1 - this->m_FrameCounter;

}
CustomType::Matrix4x4 CLightSpot::GetCurrentMatrix(const UINT& extraIndex)
{
	return (this->m_ViewProjectionMatrix[this->m_FrameCounter]);
}
CustomType::Matrix4x4 CLightSpot::GetPreviousMatrix(const UINT& extraIndex)
{
	return (this->m_ViewProjectionMatrix[1 - this->m_FrameCounter]);
}