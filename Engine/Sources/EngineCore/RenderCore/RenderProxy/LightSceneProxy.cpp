#include "LightSceneProxy.h"
#include <RenderConfig/RenderConfig.h>
#include "ViewProxy.h"

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<RDirectionalLightSceneProxy, RBaseSceneProxy>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	RDirectionalLightSceneProxy::RDirectionalLightSceneProxy(const PDirectionalLightComponent* InComponent)
		: CascadeShadowData(nullptr), IsCascadeShadow(FALSE), Component(InComponent)
	{
		Check((ENGINE_RENDER_CORE_ERROR), ("Create directional light scene proxy failed"), (!!Component));
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
	void RDirectionalLightSceneProxy::SetupProxy(const ERenderDirectionalLightMatrices& InMatrices, const ERenderLightParams& InParams, const ECascadeShadowData* InCascadeShadowData)
	{

	}
	void RDirectionalLightSceneProxy::GenerateViewInfo(const RViewProxy* InViewProxy)
	{
		if (Views.Contains(InViewProxy))
		{
			return;
		}

		// Fill the set and view domains.
		{
			Views.Add(InViewProxy);
			ViewDomainInfos.Add(InViewProxy->GetUniqueID(), TArray<EViewDomainInfo>());
		}

		const BOOL32				IsUseCascadeShadow	= IsLightUseCascadeShadow();
		TArray<EViewDomainInfo>&	DomainInfos			= ViewDomainInfos[InViewProxy->GetUniqueID()];
		const Vector2Int&			ShadowMapSize		= LightData.ShadowMapSize;

		// We add view domains at first.
		{
			{
				EViewDomainInfo& DomainInfo = DomainInfos.Add_Default_GetRef();

				DomainInfo.RenderViewport.TopLeftX	= 0.f;
				DomainInfo.RenderViewport.TopLeftY	= 0.f;
				DomainInfo.RenderViewport.Width		= EMath::TruncToFloat(ShadowMapSize.x);
				DomainInfo.RenderViewport.Height	= EMath::TruncToFloat(ShadowMapSize.y);
				DomainInfo.RenderViewport.MinDepth	= RCommonSettings::RENDER_DEPTH_MIN;
				DomainInfo.RenderViewport.MaxDepth	= RCommonSettings::RENDER_DEPTH_MAX;

				DomainInfo.ViewMatrix.GenerateViewPart(Vector3::Zero(), WorldRotation);
			}
			if (IsUseCascadeShadow)
			{
				for (INT32 i = 0u, n = ((INT32)(CascadeShadowData->Layers.Length())) - 1; i < n; i++)
				{
					EViewDomainInfo& DomainInfo = DomainInfos.Add_Default_GetRef();

					DomainInfo.RenderViewport.TopLeftX	= 0.f;
					DomainInfo.RenderViewport.TopLeftY	= 0.f;
					DomainInfo.RenderViewport.Width		= EMath::TruncToFloat(EMath::Max(1, (ShadowMapSize.x) >> (i + 1)));
					DomainInfo.RenderViewport.Height	= EMath::TruncToFloat(EMath::Max(1, (ShadowMapSize.y) >> (i + 1)));
					DomainInfo.RenderViewport.MinDepth	= RCommonSettings::RENDER_DEPTH_MIN;
					DomainInfo.RenderViewport.MaxDepth	= RCommonSettings::RENDER_DEPTH_MAX;

					DomainInfo.ViewMatrix.GenerateViewPart(Vector3::Zero(), WorldRotation);
				}
			}
		}

		// Generate light view frustum and projection
		{
			const EFrustum& ViewProxyFrustum = InViewProxy->GetViewFrustum();
			for (UINT32 CascadeIndex = 0u, DomainNum = DomainInfos.Length(), CascadeNum = IsUseCascadeShadow ? (CascadeShadowData->Layers.Length()) : 1u; CascadeIndex < CascadeNum; CascadeIndex++)
			{
#if _EDITOR_ONLY
				Check((ENGINE_RENDER_CORE_ERROR), ("Check cascade num of directional light failed(at least 1)."), ((CascadeNum > 0u) && (CascadeIndex < DomainNum)));
				if ((CascadeNum == 0u) || (CascadeIndex >= DomainNum))
				{
					continue;
				}
#endif
				const FLOAT FrustumNearT	= EMath::TruncToFloat(CascadeIndex) / EMath::TruncToFloat(CascadeNum);
				const FLOAT FrustumFarT		= EMath::TruncToFloat(CascadeIndex + 1u) / EMath::TruncToFloat(CascadeNum);
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
				EViewDomainInfo& DomainInfo = DomainInfos[CascadeIndex];
				Vector3 TempViewMin(PE_FLOAT32_MAX, PE_FLOAT32_MAX, PE_FLOAT32_MAX), TempViewMax(-PE_FLOAT32_MAX, -PE_FLOAT32_MAX, -PE_FLOAT32_MAX);
				for (UINT32 i = 0u; i < 8u; i++)
				{
					DomainInfo.ViewMatrix.TransformWorldPointToView(TempViewFrustum[i], TempViewFrustum[i]);
					TempViewMin = MinVector3(TempViewMin, TempViewFrustum[i]);
					TempViewMax = MaxVector3(TempViewMax, TempViewFrustum[i]);
				}
				DomainInfo.ViewMatrix.GenerateOrthographicProjectPart(
					EViewport(
						Vector2(TempViewMin.x, TempViewMax.y),
						Vector2(TempViewMax.x - TempViewMin.x, TempViewMin.y - TempViewMax.y),
						Vector2(RCommonSettings::RENDER_DEPTH_MIN, RCommonSettings::RENDER_DEPTH_MAX)),
					TempViewMin.z,
					TempViewMax.z);
				DomainInfo.ViewMatrix.GenerateFinalMatrix();
				Vector3 TempViewMinWorld, TempViewMaxWorld;
				DomainInfo.ViewMatrix.TransformViewPointToWorld(TempViewMin, TempViewMinWorld);
				DomainInfo.ViewMatrix.TransformViewPointToWorld(TempViewMax, TempViewMaxWorld);
				TempViewMin = MinVector3(TempViewMinWorld, TempViewMaxWorld);
				TempViewMax = MaxVector3(TempViewMinWorld, TempViewMaxWorld);
				DomainInfo.ViewFrustum.GenerateOrthographicFrustumInfo(TempViewMin.x, TempViewMax.y, TempViewMax.x, TempViewMin.y, TempViewMin.z, TempViewMax.z);
				DomainInfo.ViewFrustum.GenerateSeparatingProjectionWorldSpace();
			}
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
	BOOL32 RDirectionalLightSceneProxy::IsLightCastShadow()const
	{
		return (LightData.CastShadow);
	}
	BOOL32 RDirectionalLightSceneProxy::IsLightUseCascadeShadow()const
	{
		return (IsCascadeShadow && (!!CascadeShadowData) && (CascadeShadowData->Layers.Length() > 0u) && (CascadeShadowData->Layers.Length() == (CascadeShadowData->Borders.Length() + 1u)));
	}
	RDirectionalLightSceneProxy::RViewSetType& RDirectionalLightSceneProxy::GetViews()
	{
		return Views;
	}
	const RDirectionalLightSceneProxy::RViewSetType& RDirectionalLightSceneProxy::GetViews()const
	{
		return Views;
	}
	void RDirectionalLightSceneProxy::UpdateMatrices(const ERenderDirectionalLightMatrices& InMatrices)
	{
		SetupProxyWorldTransform(Vector3::Zero(), InMatrices.WorldRotation, Vector3::One());
	}
	void RDirectionalLightSceneProxy::UpdateLightParams(const ERenderLightParams& InParams)
	{
		const Color3&		LightColor		= InParams.LightColor;
		const Vector2Int&	ShadowMapSize	= InParams.ShadowMapSize;
		LightData = ELightData(ELightType::LIGHT_TYPE_DIRECTIONAL
			, LightColor.r, LightColor.g, LightColor.b
			, InParams.LightIntensity, InParams.CastShadow
			, ShadowMapSize.x, ShadowMapSize.y);
	}
	void RDirectionalLightSceneProxy::UpdateCascadeData(const ECascadeShadowData* InCascadeShadowData)
	{
		IsCascadeShadow = !!InCascadeShadowData;
		if (IsCascadeShadow)
		{
			if (!CascadeShadowData)
			{

			}
			const UINT32 CascadeLayerNum = InCascadeShadowData->Layers.Length();
#if _EDITOR_ONLY
			Check((ENGINE_RENDER_CORE_ERROR), ("Check directional light is cascade shadow but setup data is null."), (CascadeLayerNum > 0u));
			if (CascadeLayerNum > 0u)
#endif
			{
				CascadeShadowData = new ECascadeShadowData(InCascadeShadowData->Layers.RawData(), InCascadeShadowData->Borders.RawData(), CascadeLayerNum);
			}
		}
	}

};