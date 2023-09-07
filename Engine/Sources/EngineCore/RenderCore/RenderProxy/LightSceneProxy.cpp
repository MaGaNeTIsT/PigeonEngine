#include "LightSceneProxy.h"
#include <PigeonBase/Object/Component/CameraAndLight/DirectionalLightComponent.h>
#include "ViewProxy.h"

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<RDirectionalLightSceneProxy, RBaseSceneProxy>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	RDirectionalLightSceneProxy::RDirectionalLightSceneProxy(const PDirectionalLightComponent* InComponent, BOOL32 InIsCascade)
		: CascadeShadowData(nullptr), IsCascadeShadow(InIsCascade), WorldRotation(InComponent->GetComponentWorldRotation()), View(nullptr), Component(InComponent)
	{
		const Color3&		LightColor		= InComponent->GetLightColor();
		const Vector2Int&	ShadowMapSize	= InComponent->GetShadowMapSize();
		LightData = ELightData(InComponent->GetLightType(), LightColor.r, LightColor.g, LightColor.b, InComponent->GetLightIntensity(), InComponent->IsLightCastShadow(), ShadowMapSize.x, ShadowMapSize.y);
		//We must add first view domain for light.
		{
			EViewDomainInfo& ViewDomainInfo = ViewDomainInfos.Add_Default_GetRef();
			ViewDomainInfo.RenderViewport.TopLeftX	= 0.f;
			ViewDomainInfo.RenderViewport.TopLeftY	= 0.f;
			ViewDomainInfo.RenderViewport.Width		= EMath::TruncToFloat(ShadowMapSize.x);
			ViewDomainInfo.RenderViewport.Height	= EMath::TruncToFloat(ShadowMapSize.y);
			ViewDomainInfo.RenderViewport.MinDepth	= 0.f;
			ViewDomainInfo.RenderViewport.MaxDepth	= 1.f;
			ViewDomainInfo.ViewMatrix.GenerateViewPart(Vector3::Zero(), WorldRotation);
		}
		if (InIsCascade)
		{
			CascadeShadowData = new ECascadeShadowData();
		}
	}
	RDirectionalLightSceneProxy::RDirectionalLightSceneProxy()
		: LightData(ELightData(ELightType::LIGHT_TYPE_DIRECTIONAL, 1.f, 1.f, 1.f, 1.f, FALSE, 2, 2)), CascadeShadowData(nullptr), IsCascadeShadow(FALSE), WorldRotation(Quaternion::Identity()), View(nullptr), Component(nullptr)
	{
	}
	RDirectionalLightSceneProxy::RDirectionalLightSceneProxy(const RDirectionalLightSceneProxy& Other)
		: RBaseSceneProxy(Other), VisibilityMap(Other.VisibilityMap), LightData(Other.LightData), ViewDomainInfos(Other.ViewDomainInfos), CascadeShadowData(nullptr), IsCascadeShadow(Other.IsCascadeShadow), WorldRotation(Other.WorldRotation), View(Other.View), Component(Other.Component)
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
		if (!InViewProxy)
		{
			return;
		}
		View = InViewProxy;
		const EFrustum& ViewProxyFrustum = InViewProxy->GetViewFrustum();
		Vector3 TempViewFrustum[8] =
		{
			ViewProxyFrustum.FarNearPlanePoint[0],
			ViewProxyFrustum.FarNearPlanePoint[1],
			ViewProxyFrustum.FarNearPlanePoint[2],
			ViewProxyFrustum.FarNearPlanePoint[3],
			ViewProxyFrustum.FarNearPlanePoint[4],
			ViewProxyFrustum.FarNearPlanePoint[5],
			ViewProxyFrustum.FarNearPlanePoint[6],
			ViewProxyFrustum.FarNearPlanePoint[7]
		};
		for (UINT32 CascadeIndex = 0u, ViewDomainNum = ViewDomainInfos.Length(), CascadeNum = (IsCascadeShadow && (!!(CascadeShadowData))) ? (CascadeShadowData->CascadeLayerNum) : 1u; CascadeIndex < CascadeNum; CascadeIndex++)
		{
			if (CascadeIndex >= ViewDomainNum)
			{
				continue;
			}
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
		}
	}
	const RDirectionalLightSceneProxy::TVisibilityMapType& RDirectionalLightSceneProxy::GetVisibilityMap()const
	{
		return VisibilityMap;
	}
	RDirectionalLightSceneProxy::TVisibilityMapType& RDirectionalLightSceneProxy::GetVisibilityMap()
	{
		return VisibilityMap;
	}
	const ELightData& RDirectionalLightSceneProxy::GetLightData()const
	{
		return LightData;
	}
	const EViewDomainInfo* RDirectionalLightSceneProxy::GetViewDomainInfo(UINT32 InIndex)const
	{
		if (InIndex < ViewDomainInfos.Length())
		{
			return (&(ViewDomainInfos[InIndex]));
		}
		return nullptr;
	}
	const ECascadeShadowData* RDirectionalLightSceneProxy::GetCascadeShadowData()const
	{
		return CascadeShadowData;
	}
	BOOL32 RDirectionalLightSceneProxy::IsLightUseCascadeShadow()const
	{
		return IsCascadeShadow;
	}
	const Quaternion& RDirectionalLightSceneProxy::GetLightWorldRotation()const
	{
		return WorldRotation;
	}

};