#include "LightSceneProxy.h"
#include <RenderConfig/RenderConfig.h>
#include <PigeonBase/Object/Component/CameraAndLight/DirectionalLightComponent.h>
#include "ViewProxy.h"

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<RDirectionalLightSceneProxy, RBaseSceneProxy>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	RDirectionalLightSceneProxy::RDirectionalLightSceneProxy(const PDirectionalLightComponent* InComponent, const BOOL32 InIsCascade, const FLOAT* InCascadeLayers, const FLOAT* InCascadeBorders, const UINT32* InCascadeLayerNum)
		: CascadeShadowData(nullptr), IsCascadeShadow(InIsCascade), Component(InComponent)
	{
		if (InComponent)
		{
			SetupProxyWorldTransform(Vector3::Zero(), InComponent->GetComponentWorldRotation(), Vector3::One());
			const Color3& LightColor = InComponent->GetLightColor();
			const Vector2Int& ShadowMapSize = InComponent->GetShadowMapSize();
			LightData = ELightData(InComponent->GetLightType(), LightColor.r, LightColor.g, LightColor.b, InComponent->GetLightIntensity(), InComponent->IsLightCastShadow(), ShadowMapSize.x, ShadowMapSize.y);
			//We must add first view domain for light.
			{
				EViewDomainInfo& ViewDomainInfo = ViewDomainInfos.Add_Default_GetRef();
				ViewDomainInfo.RenderViewport.TopLeftX	= 0.f;
				ViewDomainInfo.RenderViewport.TopLeftY	= 0.f;
				ViewDomainInfo.RenderViewport.Width		= EMath::TruncToFloat(ShadowMapSize.x);
				ViewDomainInfo.RenderViewport.Height	= EMath::TruncToFloat(ShadowMapSize.y);
				ViewDomainInfo.RenderViewport.MinDepth	= RCommonSettings::RENDER_DEPTH_MIN;
				ViewDomainInfo.RenderViewport.MaxDepth	= RCommonSettings::RENDER_DEPTH_MAX;
				ViewDomainInfo.ViewMatrix.GenerateViewPart(Vector3::Zero(), WorldRotation);
			}
			if (InIsCascade)
			{
#if _EDITOR_ONLY
				Check((ENGINE_RENDER_CORE_ERROR), ("Check directional light is cascade shadow but setup data is null."), ((!!InCascadeLayers) && (!!InCascadeBorders) && (!!InCascadeLayerNum) && ((*InCascadeLayerNum) > 0u)));
				if ((!!InCascadeLayers) && (!!InCascadeBorders) && (!!InCascadeLayerNum))
#endif
				{
					CascadeShadowData = new ECascadeShadowData(InCascadeLayers, InCascadeBorders, *InCascadeLayerNum);
					for (INT32 i = 0u, n = ((INT32)(*InCascadeLayerNum)) - 1; i < n; i++)
					{
						EViewDomainInfo& ViewDomainInfo = ViewDomainInfos.Add_Default_GetRef();
						ViewDomainInfo.RenderViewport.TopLeftX	= 0.f;
						ViewDomainInfo.RenderViewport.TopLeftY	= 0.f;
						ViewDomainInfo.RenderViewport.Width		= EMath::TruncToFloat(EMath::Max(1, (ShadowMapSize.x) >> (i + 1)));
						ViewDomainInfo.RenderViewport.Height	= EMath::TruncToFloat(EMath::Max(1, (ShadowMapSize.y) >> (i + 1)));
						ViewDomainInfo.RenderViewport.MinDepth	= RCommonSettings::RENDER_DEPTH_MIN;
						ViewDomainInfo.RenderViewport.MaxDepth	= RCommonSettings::RENDER_DEPTH_MAX;
						ViewDomainInfo.ViewMatrix.GenerateViewPart(Vector3::Zero(), WorldRotation);
					}
				}
			}
		}
	}
	RDirectionalLightSceneProxy::RDirectionalLightSceneProxy()
		: LightData(ELightData(ELightType::LIGHT_TYPE_DIRECTIONAL, 1.f, 1.f, 1.f, 1.f, FALSE, 2, 2)), CascadeShadowData(nullptr), IsCascadeShadow(FALSE), Component(nullptr)
	{
	}
	RDirectionalLightSceneProxy::RDirectionalLightSceneProxy(const RDirectionalLightSceneProxy& Other)
		: RBaseSceneProxy(Other), VisibilityMap(Other.VisibilityMap), LightData(Other.LightData), ViewDomainInfos(Other.ViewDomainInfos), CascadeShadowData(nullptr), IsCascadeShadow(Other.IsCascadeShadow), Views(Other.Views), Component(Other.Component)
	{
		if (Other.IsCascadeShadow && (!!(Other.CascadeShadowData)))
		{
			CascadeShadowData = new ECascadeShadowData(*(Other.CascadeShadowData));
		}
	}
	RDirectionalLightSceneProxy::~RDirectionalLightSceneProxy()
	{
		if (CascadeShadowData)
		{
			delete CascadeShadowData;
			CascadeShadowData = nullptr;
		}
	}
	void RDirectionalLightSceneProxy::GenerateViewInfo(const RViewProxy* InViewProxy)
	{
		if (Views.Contains(InViewProxy))
		{
			return;
		}

		View = InViewProxy;
		const EFrustum& ViewProxyFrustum = InViewProxy->GetViewFrustum();
		for (UINT32 CascadeIndex = 0u, ViewDomainNum = ViewDomainInfos.Length(), CascadeNum = (IsCascadeShadow && (!!(CascadeShadowData))) ? (CascadeShadowData->Layers.Length()) : 1u; CascadeIndex < CascadeNum; CascadeIndex++)
		{
#if _EDITOR_ONLY
			Check((ENGINE_RENDER_CORE_ERROR), ("Check cascade num of directional light failed(at least 1)."), ((CascadeNum > 0u) && (CascadeIndex < ViewDomainNum)));
			if ((CascadeNum == 0u) || (CascadeIndex >= ViewDomainNum))
			{
				continue;
			}
#endif
			const FLOAT FrustumNearT = EMath::TruncToFloat(CascadeIndex) / EMath::TruncToFloat(CascadeNum);
			const FLOAT FrustumFarT = EMath::TruncToFloat(CascadeIndex + 1u) / EMath::TruncToFloat(CascadeNum);
			Vector3 TempViewFrustum[8] =
			{
				Vector3::Lerp(ViewProxyFrustum.NearPlaneTopLeft, ViewProxyFrustum.FarPlaneTopLeft, FrustumNearT),
				Vector3::Lerp(ViewProxyFrustum.NearPlaneTopRight, ViewProxyFrustum.FarPlaneTopRight, FrustumNearT),
				Vector3::Lerp(ViewProxyFrustum.NearPlaneBottomLeft, ViewProxyFrustum.FarPlaneBottomLeft, FrustumNearT),
				Vector3::Lerp(ViewProxyFrustum.NearPlaneBottomRight, ViewProxyFrustum.FarPlaneBottomRight, FrustumNearT),
				Vector3::Lerp(ViewProxyFrustum.NearPlaneTopLeft, ViewProxyFrustum.FarPlaneTopLeft, FrustumFarT),
				Vector3::Lerp(ViewProxyFrustum.NearPlaneTopRight, ViewProxyFrustum.FarPlaneTopRight, FrustumFarT),
				Vector3::Lerp(ViewProxyFrustum.NearPlaneBottomLeft, ViewProxyFrustum.FarPlaneBottomLeft, FrustumFarT),
				Vector3::Lerp(ViewProxyFrustum.NearPlaneBottomRight, ViewProxyFrustum.FarPlaneBottomRight, FrustumFarT)
			};
			EViewDomainInfo& ViewDomainInfo = ViewDomainInfos[CascadeIndex];
			Vector3 TempViewMin(PE_FLOAT32_MAX, PE_FLOAT32_MAX, PE_FLOAT32_MAX), TempViewMax(-PE_FLOAT32_MAX, -PE_FLOAT32_MAX, -PE_FLOAT32_MAX);
			for (UINT32 i = 0u; i < 8u; i++)
			{
				ViewDomainInfo.ViewMatrix.TransformWorldPointToView(TempViewFrustum[i], TempViewFrustum[i]);
				TempViewMin = MinVector3(TempViewMin, TempViewFrustum[i]);
				TempViewMax = MaxVector3(TempViewMax, TempViewFrustum[i]);
			}
			ViewDomainInfo.ViewMatrix.GenerateOrthographicProjectPart(
				EViewport(
					Vector2(TempViewMin.x, TempViewMax.y),
					Vector2(TempViewMax.x - TempViewMin.x, TempViewMin.y - TempViewMax.y),
					Vector2(RCommonSettings::RENDER_DEPTH_MIN, RCommonSettings::RENDER_DEPTH_MAX)),
				TempViewMin.z,
				TempViewMax.z);
			ViewDomainInfo.ViewMatrix.GenerateFinalMatrix();
			Vector3 TempViewMinWorld, TempViewMaxWorld;
			ViewDomainInfo.ViewMatrix.TransformViewPointToWorld(TempViewMin, TempViewMinWorld);
			ViewDomainInfo.ViewMatrix.TransformViewPointToWorld(TempViewMax, TempViewMaxWorld);
			TempViewMin = MinVector3(TempViewMinWorld, TempViewMaxWorld);
			TempViewMax = MaxVector3(TempViewMinWorld, TempViewMaxWorld);
			ViewDomainInfo.ViewFrustum.GenerateOrthographicFrustumInfo(TempViewMin.x, TempViewMax.y, TempViewMax.x, TempViewMin.y, TempViewMin.z, TempViewMax.z);
			ViewDomainInfo.ViewFrustum.GenerateSeparatingProjectionWorldSpace();
		}
	}
	const RDirectionalLightSceneProxy::RPerViewVisibilityMapType& RDirectionalLightSceneProxy::GetVisibilityMap()const
	{
		return VisibilityMap;
	}
	RDirectionalLightSceneProxy::RPerViewVisibilityMapType& RDirectionalLightSceneProxy::GetVisibilityMap()
	{
		return VisibilityMap;
	}
	const ELightData& RDirectionalLightSceneProxy::GetLightData()const
	{
		return LightData;
	}
	RDirectionalLightSceneProxy::RPerViewDomainInfoType& RDirectionalLightSceneProxy::GetViewDomainInfos()
	{
		return ViewDomainInfos;
	}
	const RDirectionalLightSceneProxy::RPerViewDomainInfoType& RDirectionalLightSceneProxy::GetViewDomainInfos()const
	{
		return ViewDomainInfos;
	}
	const ECascadeShadowData* RDirectionalLightSceneProxy::GetCascadeShadowData()const
	{
		return CascadeShadowData;
	}
	BOOL32 RDirectionalLightSceneProxy::IsLightUseCascadeShadow()const
	{
		return IsCascadeShadow;
	}
	RDirectionalLightSceneProxy::RViewSetType& RDirectionalLightSceneProxy::GetViews()
	{
		return Views;
	}
	const RDirectionalLightSceneProxy::RViewSetType& RDirectionalLightSceneProxy::GetViews()const
	{
		return Views;
	}

};