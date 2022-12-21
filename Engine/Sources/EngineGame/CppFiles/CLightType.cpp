#include "../Headers/CLightType.h"
#include "../Headers/CCamera.h"

CLightBase::CLightBase(const BOOL& active, const class CScene* scene) : CGameObject(active, scene)
{
	this->m_LightType = LightType::LIGHT_TYPE_NONE;
	this->m_Color = CustomStruct::CColor(1.f, 1.f, 1.f, 1.f);
	this->m_Intensity = 2.5f;
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



CLightDirectional::CLightDirectional(const BOOL& active, const class CScene* scene) : CLightBase(active, scene)
{
	this->SetLightType(LightType::LIGHT_TYPE_DIRECTIONAL);
	this->m_FrameCounter = 0;
	this->m_IsTransmitShadow = FALSE;
	this->m_CascadeInfo[0] = nullptr;
	this->m_CascadeInfo[1] = nullptr;
}
void CLightDirectional::Init()
{
	this->AddNewTransform();
	this->m_FrameCounter = 0;
	CGameObject::Init();
}
void CLightDirectional::Update()
{
	this->m_FrameCounter = 1 - this->m_FrameCounter;
	CGameObject::Update();
}
#ifdef _DEVELOPMENT_EDITOR
void CLightDirectional::SelectedEditorUpdate()
{
	FLOAT lightClr[3]		= { this->m_Color.r, this->m_Color.g, this->m_Color.b };
	FLOAT lightIntensity	= this->m_Intensity;

	ImGui::Begin("DirectionalLight", NULL, ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar);
	CGameObject::SelectedEditorUpdate();
	ImGui::SetNextItemOpen(true, ImGuiCond_::ImGuiCond_Once);
	if (ImGui::TreeNode("LightEditorMenu"))
	{
		ImGui::Text("LightColor");
		ImGui::ColorEdit3("LightColor", lightClr, ImGuiColorEditFlags_::ImGuiColorEditFlags_NoLabel);
		ImGui::SliderFloat("LightIntensity", &lightIntensity, 0.f, 10.f);

		ImGui::TreePop();
	}
	ImGui::End();

	this->m_Color.r		= lightClr[0];
	this->m_Color.g		= lightClr[1];
	this->m_Color.b		= lightClr[2];
	this->m_Intensity	= lightIntensity;
}
#endif
BOOL CLightDirectional::GenerateClosestShadowMap(CLightDirectional* light)
{
	BOOL result = TRUE;
	for (UINT i = 0u; i < 2u; i++)
	{
		std::shared_ptr<ShadowCascadeInfo> currentInfo = (light->m_CascadeInfo[i]);
		std::vector<LightShadowInfo>& currentShadowMap = (currentInfo->ShadowMap);
		LightShadowInfo& shadowMap0 = currentShadowMap[0];

		CustomType::Vector2Int usedShadowSize(CustomType::CMath::Log2Ceil(shadowMap0.Size.X()), CustomType::CMath::Log2Ceil(shadowMap0.Size.X()));
		usedShadowSize = CustomType::Vector2Int(CustomType::CMath::Max(CustomType::CMath::Exp2(usedShadowSize.X()), 2), CustomType::CMath::Max(CustomType::CMath::Exp2(usedShadowSize.Y()), 2));
		UINT usedShadowDepth = (shadowMap0.Depth == 16u || shadowMap0.Depth == 24u || shadowMap0.Depth == 32u) ? shadowMap0.Depth : 24u;

		currentInfo->Viewport = CustomStruct::CRenderViewport(CustomType::Vector4(0, 0, usedShadowSize.X(), usedShadowSize.Y()), CustomType::Vector2(0.f, 1.f));
		shadowMap0.Size = usedShadowSize;
		shadowMap0.Depth = usedShadowDepth;
		if (!CLightBase::CreateShadowTexture(shadowMap0.Texture, usedShadowSize, usedShadowDepth))
		{
			result = FALSE;
		}
	}
	return result;
}
BOOL CLightDirectional::GenerateCascadeShadowMap(CLightDirectional* light, const INT& frameCounter)
{
	BOOL result = TRUE;
	std::shared_ptr<ShadowCascadeInfo> currentInfo = (light->m_CascadeInfo[frameCounter]);
	std::vector<LightShadowInfo>& currentShadowMap = (currentInfo->ShadowMap);
	const UINT& currentLayerNum = currentInfo->LayerInfo.LayerNum;
	if (currentShadowMap.size() != currentLayerNum)
	{
		currentShadowMap.resize(currentLayerNum);
	}
	LightShadowInfo& shadowMap0 = currentShadowMap[0];
	CustomType::Vector2Int usedShadowSize(shadowMap0.Size);
	UINT usedShadowDepth = shadowMap0.Depth;
	for (size_t i = 1u; i < currentShadowMap.size(); i++)
	{
		usedShadowSize = CustomType::Vector2Int(CustomType::CMath::Max(usedShadowSize.X() / 2, 2), CustomType::CMath::Max(usedShadowSize.Y() / 2, 2));
		if (currentShadowMap[i].Size == usedShadowSize && currentShadowMap[i].Depth == usedShadowDepth)
		{
			continue;
		}
		currentShadowMap[i].Size = usedShadowSize;
		currentShadowMap[i].Depth = usedShadowDepth;
		if (!CLightBase::CreateShadowTexture(currentShadowMap[i].Texture, usedShadowSize, usedShadowDepth))
		{
			result = FALSE;
		}
	}
	return result;
}
void CLightDirectional::GenerateCascadeMatrices(CCamera* camera, CLightDirectional* light)
{
	const INT& frameCounter = light->m_FrameCounter;
	std::shared_ptr<ShadowCascadeInfo> currentShadowCascadeInfo = light->m_CascadeInfo[frameCounter];

	UINT& currentLayerNum = currentShadowCascadeInfo->LayerInfo.LayerNum;
	std::vector<FLOAT>& currentDistances = currentShadowCascadeInfo->LayerInfo.Distances;
	std::vector<FLOAT>& currentBorders = currentShadowCascadeInfo->LayerInfo.Borders;
	std::vector<CustomType::Matrix4x4>& projectionMatrices = currentShadowCascadeInfo->ProjectionMatrices;
	std::vector<CustomType::Vector4>& projectionSphereBounds = currentShadowCascadeInfo->ProjectionSphereBounds;
	CustomType::Vector3 cameraWorldPosition(camera->GetWorldPosition());
	{
		CustomType::Matrix4x4& viewMatrix = currentShadowCascadeInfo->ViewMatrix;
		viewMatrix = CustomType::Matrix4x4(cameraWorldPosition, light->GetWorldRotation());
		viewMatrix = viewMatrix.Inverse();
	}

	{
		std::vector<FLOAT> tempDistances;
		std::vector<FLOAT> tempBorders;
		currentDistances.clear();
		currentBorders.clear();
		FLOAT totalDist = camera->GetFar() - camera->GetNear();
		UINT cascadeLayerNum = CustomType::CMath::Clamp(currentShadowCascadeInfo->CascadeSettings.LayerNum, 1u, 4u);
		FLOAT* cascadeDistance = currentShadowCascadeInfo->CascadeSettings.Distance;
		FLOAT* cascadeBorder = currentShadowCascadeInfo->CascadeSettings.Border;
		for (UINT i = 0u; i < cascadeLayerNum; i++)
		{
			FLOAT currentDist = cascadeDistance[i];
			if (currentDist > 5.f)
			{
				totalDist = totalDist - currentDist;
				if (totalDist <= 0.f)
				{
					currentDist = totalDist + currentDist;
					tempDistances.push_back(currentDist);
					if (i < (cascadeLayerNum - 1u))
					{
						tempBorders.push_back((currentDist * cascadeBorder[i]));
					}
					break;
				}
				tempDistances.push_back(currentDist);
				if (i < (cascadeLayerNum - 1u))
				{
					tempBorders.push_back((currentDist * cascadeBorder[i]));
				}
			}
		}
		if (tempDistances.size() < 1u)
		{
			tempDistances.resize(1u);
		}
		currentLayerNum = static_cast<UINT>(tempDistances.size());
		if (totalDist > 0.f)
		{
			tempDistances[tempDistances.size() - 1u] += totalDist;
		}
		currentDistances = tempDistances;
		currentBorders = tempBorders;
	}

	if (projectionMatrices.size() != currentLayerNum)
	{
		projectionMatrices.resize(currentLayerNum);
	}
	if (projectionSphereBounds.size() != currentLayerNum)
	{
		projectionSphereBounds.resize(currentLayerNum);
	}

	{
		std::vector<CustomType::Vector3> tempPoints;
		std::vector<FLOAT> tempLayerDistances;
		std::vector<FLOAT> tempCenterT;
		{
			tempPoints.resize(currentLayerNum * 8u);
			tempCenterT.resize(currentLayerNum);
			std::vector<CustomType::Vector3> frustumPoints(camera->GetCullingPlanePoint());
			FLOAT totalLayerDist = camera->GetFar() - camera->GetNear();
			for (UINT i = 0u; i < currentLayerNum; i++)
			{
				if (i > 0u)
				{
					FLOAT tNear, tFar;
					{
						FLOAT tempNearDist = 0.f, tempFarDist = 0.f;
						for (UINT layerIndex = 0u; layerIndex <= i; layerIndex++)
						{
							if (layerIndex != i)
							{
								tempNearDist += currentDistances[layerIndex];
							}
							tempFarDist += currentDistances[layerIndex];
						}
						tempNearDist -= currentBorders[i - 1u];
						tNear = tempNearDist / totalLayerDist;
						tFar = tempFarDist / totalLayerDist;
						tempCenterT[i] = 0.5f * (tNear + tFar);
					}
					tempPoints[i * 8u + 0u] = CustomType::Vector3::Lerp(frustumPoints[0u], frustumPoints[4u], tNear);
					tempPoints[i * 8u + 1u] = CustomType::Vector3::Lerp(frustumPoints[1u], frustumPoints[5u], tNear);
					tempPoints[i * 8u + 2u] = CustomType::Vector3::Lerp(frustumPoints[2u], frustumPoints[6u], tNear);
					tempPoints[i * 8u + 3u] = CustomType::Vector3::Lerp(frustumPoints[3u], frustumPoints[7u], tNear);
					tempPoints[i * 8u + 4u] = CustomType::Vector3::Lerp(frustumPoints[0u], frustumPoints[4u], tFar);
					tempPoints[i * 8u + 5u] = CustomType::Vector3::Lerp(frustumPoints[1u], frustumPoints[5u], tFar);
					tempPoints[i * 8u + 6u] = CustomType::Vector3::Lerp(frustumPoints[2u], frustumPoints[6u], tFar);
					tempPoints[i * 8u + 7u] = CustomType::Vector3::Lerp(frustumPoints[3u], frustumPoints[7u], tFar);
				}
				else
				{
					FLOAT t = currentDistances[0u] / totalLayerDist;
					tempCenterT[0u] = 0.5f * t;
					tempPoints[0u] = frustumPoints[0u];
					tempPoints[1u] = frustumPoints[1u];
					tempPoints[2u] = frustumPoints[2u];
					tempPoints[3u] = frustumPoints[3u];
					tempPoints[4u] = CustomType::Vector3::Lerp(frustumPoints[0u], frustumPoints[4u], t);
					tempPoints[5u] = CustomType::Vector3::Lerp(frustumPoints[1u], frustumPoints[5u], t);
					tempPoints[6u] = CustomType::Vector3::Lerp(frustumPoints[2u], frustumPoints[6u], t);
					tempPoints[7u] = CustomType::Vector3::Lerp(frustumPoints[3u], frustumPoints[7u], t);
				}
			}
			tempLayerDistances.resize(currentLayerNum);
			for (UINT i = 0u; i < currentLayerNum; i++)
			{
				tempLayerDistances[i] = currentDistances[i];
				if (i > 0u)
				{
					tempLayerDistances[i] += currentBorders[i - 1u];
				}
			}
		}

		{
			struct AABB
			{
				FLOAT MinX; FLOAT MaxX;
				FLOAT MinY; FLOAT MaxY;
				FLOAT MinZ; FLOAT MaxZ;
			};
			auto calculateAABB = [](CustomType::Vector3* pos)->AABB {
				AABB result;
				result.MinX = result.MaxX = pos[0].X();
				result.MinY = result.MaxY = pos[0].Y();
				result.MinZ = result.MaxZ = pos[0].Z();
				for (UINT i = 1u; i < 8u; i++)
				{
					result.MinX = CustomType::CMath::Min(pos[i].X(), result.MinX);
					result.MaxX = CustomType::CMath::Max(pos[i].X(), result.MaxX);
					result.MinY = CustomType::CMath::Min(pos[i].Y(), result.MinY);
					result.MaxY = CustomType::CMath::Max(pos[i].Y(), result.MaxY);
					result.MinZ = CustomType::CMath::Min(pos[i].Z(), result.MinZ);
					result.MaxZ = CustomType::CMath::Max(pos[i].Z(), result.MaxZ);
				}
				return result;
			};
#if 0
			ImGui::Begin("Cascade Shadow");
#endif
			CustomType::Vector3 tempCameraDirection(camera->GetForwardVector());
			CustomType::Vector3 tempCameraCenterNear(tempCameraDirection * (camera->GetNear()) + cameraWorldPosition);
			CustomType::Vector3 tempCameraCenterFar(tempCameraDirection * (camera->GetFar()) + cameraWorldPosition);
			for (UINT i = 0u; i < currentLayerNum; i++)
			{
				CustomType::Vector3 layerPoints[] = {
					tempPoints[i * 8u + 0u], tempPoints[i * 8u + 1u],
					tempPoints[i * 8u + 2u], tempPoints[i * 8u + 3u],
					tempPoints[i * 8u + 4u], tempPoints[i * 8u + 5u],
					tempPoints[i * 8u + 6u], tempPoints[i * 8u + 7u] };
				
				{
					FLOAT tempBoundExtentSq = CustomType::Vector3::DistanceSquare(layerPoints[4], layerPoints[5]);
					tempBoundExtentSq = CustomType::CMath::Min(CustomType::Vector3::DistanceSquare(layerPoints[4], layerPoints[6]), tempBoundExtentSq);
					FLOAT tempBoundExtent = CustomType::CMath::Sqrt(tempBoundExtentSq);
					tempBoundExtent = CustomType::CMath::Min(tempLayerDistances[i], tempBoundExtent);
					tempBoundExtent *= 0.5f;
					CustomType::Vector3 tempBoundCenter(CustomType::Vector3::Lerp(tempCameraCenterNear, tempCameraCenterFar, tempCenterT[i]));
					projectionSphereBounds[i] = CustomType::Vector4(tempBoundCenter.X(), tempBoundCenter.Y(), tempBoundCenter.Z(), tempBoundExtent);
				}
#if 0
				{
					ImGui::Text("Shadow Layer %d", i);
					for (UINT debugIndex = 0u; debugIndex < 8u; debugIndex++)
					{
						ImGui::Text("Layer Points %d : x = %f, y = %f, z = %f", debugIndex, layerPoints[debugIndex].X(), layerPoints[debugIndex].Y(), layerPoints[debugIndex].Z());
					}
					ImGui::Text("Sphere Vector : x = %f, y = %f, z = %f, w = %f", projectionSphereBounds[i].X(), projectionSphereBounds[i].Y(), projectionSphereBounds[i].Z(), projectionSphereBounds[i].W());
				}
#endif
				for (UINT pointIndex = 0u; pointIndex < 8u; pointIndex++)
				{
					layerPoints[pointIndex] = currentShadowCascadeInfo->ViewMatrix.MultiplyPosition(layerPoints[pointIndex]);
				}

				{
					AABB aabb = calculateAABB(layerPoints);
					projectionMatrices[i] = CustomType::Matrix4x4::OrthographicMatrix(aabb.MinX, aabb.MaxY, aabb.MaxX, aabb.MinY, aabb.MinZ, aabb.MaxZ);
				}
			}
#if 0
			ImGui::End();
#endif
		}
	}
}
BOOL CLightDirectional::IsTransmitShadow()
{
	return ((this->m_IsTransmitShadow) && (this->m_CascadeInfo[0] != nullptr) && (this->m_CascadeInfo[1] != nullptr));
}
BOOL CLightDirectional::SetShadowInfo(const CustomType::Vector2Int& shadowSize, const UINT& shadowDepth)
{
	for (UINT i = 0u; i < 2u; i++)
	{
		if (this->m_CascadeInfo[i] == nullptr)
		{
			this->m_CascadeInfo[i] = std::shared_ptr<ShadowCascadeInfo>(new ShadowCascadeInfo());
			this->m_CascadeInfo[i]->ShadowMap.resize(1u);
		}
		std::shared_ptr<ShadowCascadeInfo> currentInfo = (this->m_CascadeInfo[i]);
		std::vector<LightShadowInfo>& currentShadowMap = (currentInfo->ShadowMap);
		if (currentShadowMap.size() < 1u)
		{
			return FALSE;
		}
		LightShadowInfo& shadowMap0 = currentShadowMap[0];
		shadowMap0.Size = shadowSize;
		shadowMap0.Depth = shadowDepth;
	}
	if (!CLightDirectional::GenerateClosestShadowMap(this))
	{
		return FALSE;
	}
	this->m_IsTransmitShadow = TRUE;
	return TRUE;
}
void CLightDirectional::PrepareCascadeShadowInfo(CCamera* camera, const ShadowCascadeSettings* settings)
{
	INT nextFrameIndex = 1 - this->m_FrameCounter;
	std::shared_ptr<ShadowCascadeInfo> nextCascadeInfo = this->m_CascadeInfo[nextFrameIndex];
	nextCascadeInfo->CurrentCamera = camera;
	if (settings)
	{
		nextCascadeInfo->CascadeSettings.LayerNum = settings->LayerNum;
		for (UINT i = 0u; i < 4u; i++)
		{
			nextCascadeInfo->CascadeSettings.Distance[i] = settings->Distance[i];
		}
		for (UINT i = 0u; i < 3u; i++)
		{
			nextCascadeInfo->CascadeSettings.Border[i] = settings->Border[i];
		}
	}
}
void CLightDirectional::UpdateCascadeShadowInfo()
{
	std::shared_ptr<ShadowCascadeInfo> currentCascadeInfo = this->m_CascadeInfo[this->m_FrameCounter];
	if (currentCascadeInfo->CurrentCamera == NULL)
	{
		return;
	}

	CLightDirectional::GenerateCascadeMatrices(currentCascadeInfo->CurrentCamera, this);
	if (!CLightDirectional::GenerateCascadeShadowMap(this, this->m_FrameCounter))
	{
		//TODO
	}

	currentCascadeInfo->CurrentCamera = NULL;
}
const UINT& CLightDirectional::GetCurrentShadowMapLayerNum()
{
	const INT& currentFrame = this->m_FrameCounter;
	return (this->m_CascadeInfo[currentFrame]->LayerInfo.LayerNum);
}
CustomStruct::CRenderViewport CLightDirectional::GetCurrentShadowMapViewport(const UINT& extraIndex)
{
	const INT& currentFrame = this->m_FrameCounter;
	std::shared_ptr<ShadowCascadeInfo> currentCascadeInfo = this->m_CascadeInfo[currentFrame];
	CustomStruct::CRenderViewport result = currentCascadeInfo->Viewport;
	result.Width = static_cast<FLOAT>(currentCascadeInfo->ShadowMap[extraIndex].Size.X());
	result.Height = static_cast<FLOAT>(currentCascadeInfo->ShadowMap[extraIndex].Size.Y());
	return result;
}
const CLightBase::LightShadowInfo& CLightDirectional::GetCurrentShadowMap(const UINT& extraIndex)
{
	const INT& currentFrame = this->m_FrameCounter;
	return (this->m_CascadeInfo[currentFrame]->ShadowMap[extraIndex]);
}
CustomType::Vector4 CLightDirectional::GetCurrentProjectionSphereBounding(const UINT& extraIndex)
{
	const INT& currentFrame = this->m_FrameCounter;
	const std::shared_ptr<ShadowCascadeInfo> currentCascadeInfo = this->m_CascadeInfo[currentFrame];
	if (extraIndex >= currentCascadeInfo->ProjectionSphereBounds.size())
	{
		return CustomType::Vector4::Zero();
	}
	return (currentCascadeInfo->ProjectionSphereBounds[extraIndex]);
}
CustomType::Matrix4x4 CLightDirectional::GetCurrentViewMatrix(const UINT& extraIndex)
{
	const INT& currentFrame = this->m_FrameCounter;
	const std::shared_ptr<ShadowCascadeInfo> currentCascadeInfo = this->m_CascadeInfo[currentFrame];
	return (currentCascadeInfo->ViewMatrix);
}
CustomType::Matrix4x4 CLightDirectional::GetCurrentProjectionMatrix(const UINT& extraIndex)
{
	const INT& currentFrame = this->m_FrameCounter;
	const std::shared_ptr<ShadowCascadeInfo> currentCascadeInfo = this->m_CascadeInfo[currentFrame];
	if (extraIndex >= currentCascadeInfo->ProjectionMatrices.size())
	{
		return CustomType::Matrix4x4::Identity();
	}
	return (currentCascadeInfo->ProjectionMatrices[extraIndex]);
}
CustomType::Matrix4x4 CLightDirectional::GetPreviousViewMatrix(const UINT& extraIndex)
{
	INT previousFrame = 1 - this->m_FrameCounter;
	const std::shared_ptr<ShadowCascadeInfo> previousCascadeInfo = this->m_CascadeInfo[previousFrame];
	return (previousCascadeInfo->ViewMatrix);
}
CustomType::Matrix4x4 CLightDirectional::GetPreviousProjectionMatrix(const UINT& extraIndex)
{
	INT previousFrame = 1 - this->m_FrameCounter;
	const std::shared_ptr<ShadowCascadeInfo> previousCascadeInfo = this->m_CascadeInfo[previousFrame];
	if (extraIndex >= previousCascadeInfo->ProjectionMatrices.size())
	{
		return CustomType::Matrix4x4::Identity();
	}
	return (previousCascadeInfo->ProjectionMatrices[extraIndex]);
}



CLightPoint::CLightPoint(const BOOL& active, const class CScene* scene) : CLightBase(active, scene)
{
	this->SetLightType(LightType::LIGHT_TYPE_POINT);
	this->m_AttenuationExponent = 1.5f;
	this->m_Radius = 100.f;
}
CustomType::Matrix4x4 CLightPoint::GetCurrentViewMatrix(const UINT& extraIndex)
{
	return (CustomType::Matrix4x4::Identity());
}
CustomType::Matrix4x4 CLightPoint::GetCurrentProjectionMatrix(const UINT& extraIndex)
{
	return (CustomType::Matrix4x4::Identity());
}
CustomType::Matrix4x4 CLightPoint::GetPreviousViewMatrix(const UINT& extraIndex)
{
	return (CustomType::Matrix4x4::Identity());
}
CustomType::Matrix4x4 CLightPoint::GetPreviousProjectionMatrix(const UINT& extraIndex)
{
	return (CustomType::Matrix4x4::Identity());
}
void CLightPoint::Init()
{
	this->AddNewTransform();
	CGameObject::Init();
}



CLightSpot::CLightSpot(const BOOL& active, const class CScene* scene) : CLightBase(active, scene)
{
	this->SetLightType(LightType::LIGHT_TYPE_SPOT);
	this->m_Range = 100.f;
	this->m_HalfRadian = 30.f * CustomType::CMath::GetDegToRad();
	this->m_CosHalfRadian = CustomType::CMath::Cos(this->m_HalfRadian);
	this->m_FrameCounter = 0;
	this->m_ViewMatrix[0] = CustomType::Matrix4x4::Identity();
	this->m_ViewMatrix[1] = CustomType::Matrix4x4::Identity();
	this->m_ProjectionMatrix[0] = CustomType::Matrix4x4::Identity();
	this->m_ProjectionMatrix[1] = CustomType::Matrix4x4::Identity();
}
CustomType::Matrix4x4 CLightSpot::GetCurrentViewMatrix(const UINT& extraIndex)
{
	return (this->m_ViewMatrix[this->m_FrameCounter]);
}
CustomType::Matrix4x4 CLightSpot::GetCurrentProjectionMatrix(const UINT& extraIndex)
{
	return (this->m_ProjectionMatrix[this->m_FrameCounter]);
}
CustomType::Matrix4x4 CLightSpot::GetPreviousViewMatrix(const UINT& extraIndex)
{
	return (this->m_ViewMatrix[1 - this->m_FrameCounter]);
}
CustomType::Matrix4x4 CLightSpot::GetPreviousProjectionMatrix(const UINT& extraIndex)
{
	return (this->m_ProjectionMatrix[1 - this->m_FrameCounter]);
}
void CLightSpot::Init()
{
	this->AddNewTransform();
	CGameObject::Init();
}
void CLightSpot::Update()
{
	this->m_FrameCounter = 1 - this->m_FrameCounter;
	CGameObject::Update();
}