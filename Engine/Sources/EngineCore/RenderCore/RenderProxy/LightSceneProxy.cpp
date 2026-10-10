#include "LightSceneProxy.h"
#include <RenderConfig/RenderConfig.h>
#include "ViewProxy.h"

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<RDirectionalLightSceneProxy, RBaseSceneProxy>();
		RegisterClassType<RLocalLightSceneProxy, RBaseSceneProxy>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	void EncodeDirectionalLight(const ELightData& InData, const Quaternion& InWorldRotation, EDirectionalLightsGPU& OutElement) noexcept
	{
		// A light aims down its own forward axis, so the direction it travels is the negation of
		// that axis. Storing the direction of travel rather than the direction to the light is
		// what lets the shading side negate once, and it is the same convention the shadow view
		// setup used.
		const Vector3 TravelDirection = -SplitForwardVector(InWorldRotation);

		OutElement.DirectionAndIlluminanceLux = Vector4(
			TravelDirection.x, TravelDirection.y, TravelDirection.z, InData.IlluminanceLux);
		OutElement.ColorAndSourceAngleDegrees = Vector4(
			InData.LightColor.r, InData.LightColor.g, InData.LightColor.b, InData.SourceAngleDegrees);
		// Shadow map index 0 means "casts no shadow", so real indices start at 1. Nothing reads
		// this yet - the shadow terms are stubs - but the addressing is settled here so the
		// shading side does not have to change when the algorithms land.
		OutElement.ShadowAddressing = Vector4Int(
			static_cast<INT32>(InData.CastShadow ? 1u : 0u), 0, 0, 0);
	}
	void EncodeLocalLight(const ELightData& InData, const Vector3& InWorldLocation, const Quaternion& InWorldRotation, ELocalLightsGPU& OutElement) noexcept
	{
		const BOOL32 bIsSpot = ((InData.LightType == ELightType::LIGHT_TYPE_SPOT) ? TRUE : FALSE);
		const Vector3 SpotAxis = SplitForwardVector(InWorldRotation);

		// The cone travels as cosines, because a cosine is what the shader compares a direction
		// against and computing it here costs one transcendental per light instead of one per
		// light per pixel. Inner is clamped to no wider than outer so a misauthored pair cannot
		// invert the penumbra, and a point light stores zeros in both: nothing reads them,
		// because the type field is what selects the cone.
		const FLOAT InnerCos = EMath::Cos(EMath::DegreesToRadians(InData.SpotInnerAngleDegrees));
		const FLOAT OuterCos = EMath::Cos(EMath::DegreesToRadians(InData.SpotOuterAngleDegrees));

		OutElement.WorldPositionAndAttenuationRadiusMeters = Vector4(
			InWorldLocation.x, InWorldLocation.y, InWorldLocation.z, InData.AttenuationRadiusMeters);
		OutElement.ColorAndLuminousIntensityCandela = Vector4(
			InData.LightColor.r, InData.LightColor.g, InData.LightColor.b, InData.LuminousIntensityCandela);
		OutElement.WorldDirectionAndSourceRadiusMeters = Vector4(
			SpotAxis.x, SpotAxis.y, SpotAxis.z, InData.SourceRadiusMeters);
		OutElement.SpotAndTypeParams = Vector4Int(
			static_cast<INT32>(InData.LightType)
			, static_cast<INT32>(EMath::AsUint32(bIsSpot ? EMath::Min(InnerCos, OuterCos) : 0.f))
			, static_cast<INT32>(EMath::AsUint32(bIsSpot ? OuterCos : 0.f))
			, static_cast<INT32>(InData.CastShadow ? 1u : 0u));
	}

	RDirectionalLightSceneProxy::RDirectionalLightSceneProxy(const PDirectionalLightComponent* InComponent)
		: CascadeShadowData(nullptr), IsCascadeShadow(FALSE), Component(InComponent)
	{
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Create directional light scene proxy failed"), (!!Component));
	}
	RDirectionalLightSceneProxy::RDirectionalLightSceneProxy()
		: LightData(ELightData(ELightType::LIGHT_TYPE_DIRECTIONAL, Color3::White(), FALSE, Vector2Int(2, 2))), CascadeShadowData(nullptr), IsCascadeShadow(FALSE), Component(nullptr)
	{
	}
	RDirectionalLightSceneProxy::RDirectionalLightSceneProxy(const RDirectionalLightSceneProxy& Other)
		: RBaseSceneProxy(Other), VisibilityMap(Other.VisibilityMap), LightData(Other.LightData), ViewDomainInfos(Other.ViewDomainInfos), CascadeShadowData(nullptr), IsCascadeShadow(Other.IsCascadeShadow), Views(Other.Views), Component(Other.Component)
	{
		if (Other.IsCascadeShadow)
		{
			if (!!(Other.CascadeShadowData))
			{
				CascadeShadowData = new ECascadeShadowData(*(Other.CascadeShadowData));
			}
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
		UpdateMatrices(InMatrices);
		UpdateLightParams(InParams);
		UpdateCascadeData(InCascadeShadowData);
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
				EViewDomainInfo& DomainInfo = DomainInfos.AddDefaultGetRef();

				DomainInfo.RenderViewport.TopLeftX	= 0.f;
				DomainInfo.RenderViewport.TopLeftY	= 0.f;
				DomainInfo.RenderViewport.Width		= EMath::TruncToFloat(ShadowMapSize.x);
				DomainInfo.RenderViewport.Height	= EMath::TruncToFloat(ShadowMapSize.y);
				DomainInfo.RenderViewport.MinDepth	= RCommonSettings::RENDER_DEPTH_NEAR;
				DomainInfo.RenderViewport.MaxDepth	= RCommonSettings::RENDER_DEPTH_FAR;

				DomainInfo.ViewMatrix.GenerateViewPart(Vector3::Zero(), WorldRotation);
			}
			if (IsUseCascadeShadow)
			{
				for (INT32 i = 0u, n = (CascadeShadowData->Layers.Num()) - 1; i < n; i++)
				{
					EViewDomainInfo& DomainInfo = DomainInfos.AddDefaultGetRef();

					DomainInfo.RenderViewport.TopLeftX	= 0.f;
					DomainInfo.RenderViewport.TopLeftY	= 0.f;
					DomainInfo.RenderViewport.Width		= EMath::TruncToFloat(EMath::Max(1, (ShadowMapSize.x) >> (i + 1)));
					DomainInfo.RenderViewport.Height	= EMath::TruncToFloat(EMath::Max(1, (ShadowMapSize.y) >> (i + 1)));
					DomainInfo.RenderViewport.MinDepth	= RCommonSettings::RENDER_DEPTH_NEAR;
					DomainInfo.RenderViewport.MaxDepth	= RCommonSettings::RENDER_DEPTH_FAR;

					DomainInfo.ViewMatrix.GenerateViewPart(Vector3::Zero(), WorldRotation);
				}
			}
		}

		// Generate light view frustum and projection
		{
			const EFrustum& ViewProxyFrustum = InViewProxy->GetViewFrustum();
#if _EDITOR_ONLY
			const INT32 DomainNum = DomainInfos.Num();
#endif
			for (INT32 CascadeIndex = 0, CascadeNum = IsUseCascadeShadow ? (CascadeShadowData->Layers.Num()) : 1; CascadeIndex < CascadeNum; CascadeIndex++)
			{
#if _EDITOR_ONLY
				PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check cascade num of directional light failed(at least 1)."), ((CascadeNum > 0) && (CascadeIndex < DomainNum)));
				if ((CascadeNum <= 0) || (CascadeIndex >= DomainNum))
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
						Vector2(RCommonSettings::RENDER_DEPTH_NEAR, RCommonSettings::RENDER_DEPTH_FAR)),
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
		if (IsCascadeShadow)
		{
			if (!!CascadeShadowData)
			{
				return ((CascadeShadowData->Layers.Num() > 0) && (CascadeShadowData->Layers.Num() == (CascadeShadowData->Borders.Num() + 1)));
			}
		}
		return FALSE;
	}
	void RDirectionalLightSceneProxy::MarkNeedUpdateParams()const
	{
		NeedUpdateParams = TRUE;
	}
	BOOL32 RDirectionalLightSceneProxy::IsNeedUpdateParams()const
	{
		BOOL32 Result = NeedUpdateParams;
		NeedUpdateParams = FALSE;
		return Result;
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
		SetProxyWorldTransform(Vector3::Zero(), InMatrices.WorldRotation, Vector3::One());
	}
	void RDirectionalLightSceneProxy::UpdateLightParams(const ERenderLightParams& InParams)
	{
		LightData.LightType				= ELightType::LIGHT_TYPE_DIRECTIONAL;
		LightData.LightColor			= InParams.LightColor;
		LightData.IlluminanceLux		= InParams.IlluminanceLux;
		LightData.SourceAngleDegrees	= InParams.SourceAngleDegrees;
		LightData.CastShadow			= InParams.CastShadow;
		LightData.ShadowMapSize			= InParams.ShadowMapSize;
	}
	void RDirectionalLightSceneProxy::UpdateCascadeData(const ECascadeShadowData* InCascadeShadowData)
	{
		IsCascadeShadow = (InCascadeShadowData) ? (InCascadeShadowData->IsUseShadow) : FALSE;
		if (IsCascadeShadow)
		{
			if (!CascadeShadowData)
			{
				CascadeShadowData = new ECascadeShadowData();
			}
#if _EDITOR_ONLY
			if (InCascadeShadowData)
#endif
			{
#if _EDITOR_ONLY
				const INT32 CascadeLayerNum = InCascadeShadowData->Layers.Num();
				PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check directional light is cascade shadow but setup data is null."), (CascadeLayerNum > 0));
				if (CascadeLayerNum > 0)
#endif
				{
					CascadeShadowData->Setup(InCascadeShadowData->IsUseShadow, InCascadeShadowData->Layers, InCascadeShadowData->Borders);
				}
			}
		}
		else
		{
			if (CascadeShadowData)
			{
				CascadeShadowData->IsUseShadow = FALSE;
			}
		}
	}

	RLocalLightSceneProxy::RLocalLightSceneProxy(const PLocalLightComponent* InComponent)
		: bNeedUpdateParams(TRUE), Component(InComponent)
	{
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), (PE_TEXT("Create local light scene proxy failed")), (!!Component));
	}
	RLocalLightSceneProxy::RLocalLightSceneProxy()
		: bNeedUpdateParams(TRUE), Component(nullptr)
	{
	}
	RLocalLightSceneProxy::RLocalLightSceneProxy(const RLocalLightSceneProxy& Other)
		: RBaseSceneProxy(Other), bNeedUpdateParams(TRUE), LightData(Other.LightData), Component(Other.Component)
	{
	}
	RLocalLightSceneProxy::~RLocalLightSceneProxy()
	{
	}
	void RLocalLightSceneProxy::SetupProxy(const ERenderTransformInfo& InTransform, const ELightData& InData)
	{
		UpdateTransform(InTransform);
		UpdateLightParams(InData);
	}
	const ELightData& RLocalLightSceneProxy::GetLightData() const
	{
		return LightData;
	}
	void RLocalLightSceneProxy::MarkNeedUpdateParams() const
	{
		bNeedUpdateParams = TRUE;
	}
	BOOL32 RLocalLightSceneProxy::IsNeedUpdateParams() const
	{
		BOOL32 Result = bNeedUpdateParams;
		bNeedUpdateParams = FALSE;
		return Result;
	}
	void RLocalLightSceneProxy::UpdateTransform(const ERenderTransformInfo& InTransform)
	{
		// The scaling is deliberately dropped for the direction and the position alike: a light
		// has a size and an aim, both of which are authored, and a scaled parent transform would
		// otherwise stretch a cone or move a light away from where its position says it is.
		SetProxyWorldTransform(InTransform.WorldLocation, InTransform.WorldRotation, Vector3::One());
	}
	void RLocalLightSceneProxy::UpdateLightParams(const ELightData& InData)
	{
		// A whole struct copy rather than field by field: the light data is the unit of an
		// update, and every field a light type does not use is already at its default in the
		// component's copy.
		LightData = InData;
	}

};
