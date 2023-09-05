#include "LightProxy.h"
#include <PigeonBase/Object/Light/DirectionalLightComponent.h>
#include "ViewProxy.h"

namespace PigeonEngine
{

	RDirectionalLightProxy::RDirectionalLightProxy(const RScene* InScene, const PDirectionalLightComponent* InComponent, BOOL32 InIsCascade)
		: IsCascadeShadow(InIsCascade), WorldRotation(InComponent->GetComponentWorldRotation()), Scene(InScene), View(nullptr), Component(InComponent)
	{
		const Color3&		LightColor		= InComponent->GetLightColor();
		const Vector2Int&	ShadowMapSize	= InComponent->GetShadowMapSize();
		LightData = ELightData(InComponent->GetLightType(), LightColor.r, LightColor.g, LightColor.b, InComponent->GetLightIntensity(), InComponent->IsLightCastShadow(), ShadowMapSize.x, ShadowMapSize.y);
		if (!InIsCascade)
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
	}
	RDirectionalLightProxy::RDirectionalLightProxy()
		: LightData(ELightData(ELightType::LIGHT_TYPE_DIRECTIONAL, 1.f, 1.f, 1.f, 1.f, FALSE, 2, 2)), IsCascadeShadow(FALSE), WorldRotation(Quaternion::Identity()), Scene(nullptr), View(nullptr), Component(nullptr)
	{
	}
	RDirectionalLightProxy::RDirectionalLightProxy(const RDirectionalLightProxy& Other)
		: VisibilityMap(Other.VisibilityMap), LightData(Other.LightData), ViewDomainInfos(Other.ViewDomainInfos), IsCascadeShadow(Other.IsCascadeShadow), WorldRotation(Other.WorldRotation), Scene(Other.Scene), View(Other.View), Component(Other.Component)
	{
	}
	RDirectionalLightProxy::~RDirectionalLightProxy()
	{
	}
	void RDirectionalLightProxy::GenerateViewInfo(const RViewProxy* InViewProxy)
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
		for(UINT CascadeIndex = 0u, CascadeNum= )
		Vector3 TempViewMin(PE_FLOAT32_MAX, PE_FLOAT32_MAX, PE_FLOAT32_MAX), TempViewMax(-PE_FLOAT32_MAX, -PE_FLOAT32_MAX, -PE_FLOAT32_MAX);
		for (UINT i = 0u; i < 8u; i++)
		{
			ViewMatrix.TransformWorldPointToView(TempViewFrustum[i], TempViewFrustum[i]);
			TempViewMin = MinVector3(TempViewMin, TempViewFrustum[i]);
			TempViewMax = MaxVector3(TempViewMax, TempViewFrustum[i]);
		}
		ViewMatrix.GenerateOrthographicProjectPart(
			EViewport(
				Vector2(TempViewMin.x, TempViewMax.y),
				Vector2(TempViewMax.x - TempViewMin.x, TempViewMin.y - TempViewMax.y),
				Vector2(RCommonSettings::RENDER_DEPTH_MIN, RCommonSettings::RENDER_DEPTH_MAX)),
			TempViewMin.z,
			TempViewMax.z);
		ViewMatrix.GenerateFinalMatrix();
		Vector3 TempViewMinWorld, TempViewMaxWorld;
		ViewMatrix.TransformViewPointToWorld(TempViewMin, TempViewMinWorld);
		ViewMatrix.TransformViewPointToWorld(TempViewMax, TempViewMaxWorld);
		TempViewMin = MinVector3(TempViewMinWorld, TempViewMaxWorld);
		TempViewMax = MaxVector3(TempViewMinWorld, TempViewMaxWorld);
		ViewFrustum.GenerateOrthographicFrustumInfo(TempViewMin.x, TempViewMax.y, TempViewMax.x, TempViewMin.y, TempViewMin.z, TempViewMax.z);
	}

};