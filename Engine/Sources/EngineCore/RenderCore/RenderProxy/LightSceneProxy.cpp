#include "LightSceneProxy.h"
#include <RenderConfig/RenderConfig.h>
#include <RenderConfig/ShadowConfig.h>
#include "ViewProxy.h"
#include "ShadowViewProxy.h"

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<RDirectionalLightSceneProxy, RBaseSceneProxy>();
		RegisterClassType<RLocalLightSceneProxy, RBaseSceneProxy>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	void EncodeDirectionalLight(const ELightData& InData, const Quaternion& InWorldRotation
		, const UINT32 InShadowSlot, const UINT32 InCascadeCount, EDirectionalLightsGPU& OutElement) noexcept
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
		// Slot zero is the "casts no shadow" sentinel the slot table starts with, so a light the
		// frame gave no shadow view answers zero here and the receiver skips the shadow term
		// without a branch on the light type. The cascade count tells the receiver how many
		// consecutive slots this light owns.
		OutElement.ShadowAddressing = Vector4Int(
			static_cast<INT32>(InShadowSlot), static_cast<INT32>(InCascadeCount), 0, 0);
	}
	void EncodeLocalLight(const ELightData& InData, const Vector3& InWorldLocation, const Quaternion& InWorldRotation
		, const UINT32 InShadowSlot, ELocalLightsGPU& OutElement) noexcept
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
		// The shadow slot is the first slot this light owns. A point light owns six consecutive
		// ones, so the receiver adds the face it selects; a spot owns one. Zero means the frame
		// allocated no shadow view for the light.
		OutElement.SpotAndTypeParams = Vector4Int(
			static_cast<INT32>(InData.LightType)
			, static_cast<INT32>(EMath::AsUint32(bIsSpot ? EMath::Min(InnerCos, OuterCos) : 0.f))
			, static_cast<INT32>(EMath::AsUint32(bIsSpot ? OuterCos : 0.f))
			, static_cast<INT32>(InShadowSlot));
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
		: RBaseSceneProxy(Other), LightData(Other.LightData), CascadeShadowData(nullptr), IsCascadeShadow(Other.IsCascadeShadow), Component(Other.Component)
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
	const ELightData& RDirectionalLightSceneProxy::GetLightData()const
	{
		return LightData;
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
	FLOAT RLocalLightSceneProxy::ComputeScreenCoverage(const RViewProxy& InView)const
	{
		// The sphere's angular radius times the projection scale is its approximate pixel radius:
		// radius / distance is the tangent of the half angle, and height / (2 tan(fovY/2)) is pixels
		// per unit of tangent. A camera inside the sphere yields the maximum relevance rather than
		// a degenerate one, which is correct - the light fills the screen.
		const ECameraViewInfo& ViewInfo = InView.GetViewInfo();
		const FLOAT Radius = EMath::Max(LightData.AttenuationRadiusMeters, 1.f);
		const FLOAT Distance = Vector3::Length(GetWorldLocation() - InView.GetWorldLocation());
		const FLOAT HalfFovRadians = EMath::DegreesToRadians(0.5f * ViewInfo.FovAngleY);
		const FLOAT ProjectionScale = ((ViewInfo.Viewport.Height > 0.f)
			? (ViewInfo.Viewport.Height / (2.f * EMath::Max(EMath::Tan(HalfFovRadians), 1e-4f))) : 1.f);
		return ((Radius / EMath::Max(Distance, Radius)) * ProjectionScale);
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

	// ---------------------------------------------------------------------------
	// Shadow views
	// ---------------------------------------------------------------------------

	static FLOAT ShadowLayerThickness(const ECascadeShadowData& InData, const UINT32 InIndex, const FLOAT InScale)
	{
		if (InIndex >= InData.Layers.Num<UINT32>())
		{
			return 0.f;
		}
		return EMath::Max(InData.Layers[InIndex] * InScale, 1.f);
	}
	static void ShadowCascadeCorners(const EFrustum& InFrustum, const FLOAT InNearT, const FLOAT InFarT, Vector3 OutCorners[8])
	{
		OutCorners[0] = Vector3::Lerp(InFrustum.NearPlaneTopLeft, InFrustum.FarPlaneTopLeft, InNearT);
		OutCorners[1] = Vector3::Lerp(InFrustum.NearPlaneTopRight, InFrustum.FarPlaneTopRight, InNearT);
		OutCorners[2] = Vector3::Lerp(InFrustum.NearPlaneBottomLeft, InFrustum.FarPlaneBottomLeft, InNearT);
		OutCorners[3] = Vector3::Lerp(InFrustum.NearPlaneBottomRight, InFrustum.FarPlaneBottomRight, InNearT);
		OutCorners[4] = Vector3::Lerp(InFrustum.NearPlaneTopLeft, InFrustum.FarPlaneTopLeft, InFarT);
		OutCorners[5] = Vector3::Lerp(InFrustum.NearPlaneTopRight, InFrustum.FarPlaneTopRight, InFarT);
		OutCorners[6] = Vector3::Lerp(InFrustum.NearPlaneBottomLeft, InFrustum.FarPlaneBottomLeft, InFarT);
		OutCorners[7] = Vector3::Lerp(InFrustum.NearPlaneBottomRight, InFrustum.FarPlaneBottomRight, InFarT);
	}
	// Turns the corner points of a view space frustum into world space and rebuilds its planes,
	// which is what the caster cull and the receiver both test against.
	static void ShadowFrustumToWorld(EViewMatrix& InOutViewMatrix, EFrustum& InOutFrustum)
	{
		for (UINT32 CornerIndex = 0u; CornerIndex < 8u; CornerIndex++)
		{
			Vector3 WorldCorner;
			InOutViewMatrix.TransformViewPointToWorld(InOutFrustum.FarNearPlanePoint[CornerIndex], WorldCorner);
			InOutFrustum.FarNearPlanePoint[CornerIndex] = WorldCorner;
		}
		InOutFrustum.GeneratePlanesFromCornersWorldSpace();
	}

	BOOL32 RDirectionalLightSceneProxy::CollectShadowViews(const RViewProxy& InView, TArray<FShadowViewRequest>& OutRequests)const
	{
		if (IsLightCastShadow() == FALSE)
		{
			return FALSE;
		}
		const RShadowSettings* Settings = RShadowSettings::GetSingleton();
		if (Settings->bEnableDirectionalShadow == FALSE)
		{
			return FALSE;
		}
		UINT32 CascadeNum = 1u;
		if (IsLightUseCascadeShadow() != FALSE)
		{
			CascadeNum = EMath::Min(CascadeShadowData->Layers.Num<UINT32>(), Settings->DirectionalCascadeNumMax);
		}
		const ECameraViewInfo& ViewInfo = InView.GetViewInfo();
		const FLOAT CameraNear = ViewInfo.NearDist;
		const FLOAT CameraFar = EMath::Max(ViewInfo.FarDist, CameraNear + 1.f);
		FLOAT SplitCursor = CameraNear;
		for (UINT32 CascadeIndex = 0u; CascadeIndex < CascadeNum; CascadeIndex++)
		{
			FShadowViewRequest Request;
			Request.Kind			= EShadowViewKindType::SHADOW_VIEW_KIND_DIRECTIONAL_CASCADE;
			Request.Index			= CascadeIndex;
			Request.Count			= CascadeNum;
			// The per cascade ladder against the type cap. The light's own ShadowMapSize is not
			// consulted: the component still carries its placeholder value, and honouring it would
			// shrink every cascade to a handful of texels until the authors actually set one.
			Request.RequestedSize	= EMath::Min(Settings->GetCascadeResolution(CascadeIndex), Settings->DirectionalMaxResolution);
			FLOAT SplitEnd = CameraFar;
			if (IsLightUseCascadeShadow() != FALSE)
			{
				// A ladder that ends before the camera does extends its last cascade to the far
				// plane, which is what keeps the distance beyond the authored thickness shadowed
				// instead of silently falling off the end of the cascade list.
				const BOOL32 bIsLastCascade = (((CascadeIndex + 1u) >= CascadeNum) ? TRUE : FALSE);
				SplitEnd = (bIsLastCascade != FALSE)
					? CameraFar
					: EMath::Min(SplitCursor
						+ ShadowLayerThickness(*CascadeShadowData, CascadeIndex, Settings->DirectionalCascadeDistanceScale), CameraFar);
			}
			Request.SplitDistance = SplitEnd;
			OutRequests.Add(Request);
			SplitCursor = SplitEnd;
		}
		return TRUE;
	}
	BOOL32 RDirectionalLightSceneProxy::BuildShadowViewDomain(const RViewProxy& InView, const FShadowViewRequest& InRequest
		, const UINT32 InResolution, EViewDomainInfo& OutDomain
		, Vector3& OutAnchor, Quaternion& OutRotation)const
	{
		const RShadowSettings* Settings = RShadowSettings::GetSingleton();
		const ECameraViewInfo& ViewInfo = InView.GetViewInfo();
		const FLOAT CameraNear = ViewInfo.NearDist;
		const FLOAT CameraFar = EMath::Max(ViewInfo.FarDist, CameraNear + 1.f);

		// This cascade's slice along the view axis, with the overlap border pulled towards the
		// camera so two neighbours share geometry rather than meeting at a seam.
		FLOAT SplitNear = CameraNear;
		FLOAT SplitFar = CameraFar;
		if (IsLightUseCascadeShadow() != FALSE)
		{
			FLOAT Cursor = CameraNear;
			for (UINT32 LayerIndex = 0u; LayerIndex < InRequest.Index; LayerIndex++)
			{
				Cursor += ShadowLayerThickness(*CascadeShadowData, LayerIndex, Settings->DirectionalCascadeDistanceScale);
			}
			SplitNear = Cursor;
			// The last cascade reaches the far plane for the same reason its request does: the
			// ladder's authored thickness is not the camera's depth budget.
			const BOOL32 bIsLastCascade = (((InRequest.Index + 1u) >= InRequest.Count) ? TRUE : FALSE);
			SplitFar = (bIsLastCascade != FALSE)
				? CameraFar
				: EMath::Min(Cursor
					+ ShadowLayerThickness(*CascadeShadowData, InRequest.Index, Settings->DirectionalCascadeDistanceScale), CameraFar);
			if ((InRequest.Index > 0u) && ((InRequest.Index - 1u) < CascadeShadowData->Borders.Num<UINT32>()))
			{
				SplitNear = EMath::Max(CameraNear, SplitNear - CascadeShadowData->Borders[InRequest.Index - 1u]);
			}
		}
		const FLOAT CameraRange = EMath::Max(CameraFar - CameraNear, 1e-3f);
		const FLOAT NearT = EMath::Clamp((SplitNear - CameraNear) / CameraRange, 0.f, 1.f);
		const FLOAT FarT = EMath::Clamp((SplitFar - CameraNear) / CameraRange, 0.f, 1.f);
		Vector3 Corners[8];
		ShadowCascadeCorners(InView.GetViewFrustum(), NearT, FarT, Corners);

		// The light view is anchored at the camera, so the slice sits inside its depth range
		// whatever the camera does; the orientation is the light's own.
		OutAnchor = InView.GetWorldLocation();
		OutRotation = GetWorldRotation();
		EViewMatrix LightView;
		LightView.GenerateViewPart(OutAnchor, OutRotation);

		Vector3 ViewMin(PE_FLOAT32_MAX, PE_FLOAT32_MAX, PE_FLOAT32_MAX);
		Vector3 ViewMax(-PE_FLOAT32_MAX, -PE_FLOAT32_MAX, -PE_FLOAT32_MAX);
		for (UINT32 CornerIndex = 0u; CornerIndex < 8u; CornerIndex++)
		{
			Vector3 ViewCorner;
			LightView.TransformWorldPointToView(Corners[CornerIndex], ViewCorner);
			ViewMin = MinVector3(ViewMin, ViewCorner);
			ViewMax = MaxVector3(ViewMax, ViewCorner);
		}

		// Texel snapping: quantize the minimum corner to the texel grid so a camera move that
		// changes nothing visible does not shift every shadow edge by a fraction of a texel.
		const FLOAT Resolution = static_cast<FLOAT>(EMath::Max(InResolution, 1u));
		const FLOAT SpanX = EMath::Max(ViewMax.x - ViewMin.x, 1e-3f);
		const FLOAT SpanY = EMath::Max(ViewMax.y - ViewMin.y, 1e-3f);
		const FLOAT TexelX = SpanX / Resolution;
		const FLOAT TexelY = SpanY / Resolution;
		ViewMin.x = EMath::TruncToFloat(ViewMin.x / TexelX) * TexelX;
		ViewMin.y = EMath::TruncToFloat(ViewMin.y / TexelY) * TexelY;
		ViewMax.x = ViewMin.x + SpanX;
		ViewMax.y = ViewMin.y + SpanY;
		// Casters outside the slice still project into it, so the near plane is pulled back.
		ViewMin.z -= Settings->DepthPadding;

		OutDomain.ViewMatrix = EViewMatrix();
		OutDomain.ViewMatrix.ViewPart = LightView.ViewPart;
		OutDomain.ViewMatrix.GenerateOrthographicProjectPart(ViewMin.x, ViewMax.y, ViewMax.x, ViewMin.y, ViewMin.z, ViewMax.z);
		OutDomain.ViewMatrix.GenerateFinalMatrix();

		OutDomain.ViewFrustum = EFrustum();
		OutDomain.ViewFrustum.GenerateOrthographicFrustumInfo(ViewMin.x, ViewMax.y, ViewMax.x, ViewMin.y, ViewMin.z, ViewMax.z);
		ShadowFrustumToWorld(OutDomain.ViewMatrix, OutDomain.ViewFrustum);

		OutDomain.RenderViewport = EViewport(0.f, 0.f, Resolution, Resolution
			, RCommonSettings::RENDER_DEPTH_NEAR, RCommonSettings::RENDER_DEPTH_FAR);
		return TRUE;
	}

	BOOL32 RLocalLightSceneProxy::CollectShadowViews(const RViewProxy& InView, TArray<FShadowViewRequest>& OutRequests)const
	{
		if (LightData.CastShadow == FALSE)
		{
			return FALSE;
		}
		const RShadowSettings* Settings = RShadowSettings::GetSingleton();
		if (Settings->bEnableLocalShadow == FALSE)
		{
			return FALSE;
		}
		const BOOL32 bPoint = ((LightData.LightType == ELightType::LIGHT_TYPE_POINT) ? TRUE : FALSE);
		if ((bPoint == FALSE) && (LightData.LightType != ELightType::LIGHT_TYPE_SPOT))
		{
			return FALSE;
		}

		// Importance is the light's approximate pixel radius on screen: its angular size times the
		// projection scale. The renderer sorts by it and then applies the per category cap.
		const FLOAT Importance = ComputeScreenCoverage(InView);
		const UINT32 ViewNum = ((bPoint != FALSE) ? 6u : 1u);
		for (UINT32 ViewIndex = 0u; ViewIndex < ViewNum; ViewIndex++)
		{
			FShadowViewRequest Request;
			Request.Kind			= ((bPoint != FALSE) ? EShadowViewKindType::SHADOW_VIEW_KIND_POINT_FACE : EShadowViewKindType::SHADOW_VIEW_KIND_SPOT);
			Request.Index			= ViewIndex;
			Request.Count			= ViewNum;
			Request.RequestedSize	= ((bPoint != FALSE) ? Settings->PointFaceMaxResolution : Settings->SpotMaxResolution);
			Request.Importance		= Importance;
			OutRequests.Add(Request);
		}
		return TRUE;
	}
	BOOL32 RLocalLightSceneProxy::BuildShadowViewDomain(const RViewProxy& InView, const FShadowViewRequest& InRequest
		, const UINT32 InResolution, EViewDomainInfo& OutDomain
		, Vector3& OutAnchor, Quaternion& OutRotation)const
	{
		(void)InView;
		const RShadowSettings* Settings = RShadowSettings::GetSingleton();
		const Vector3 LightPosition = GetWorldLocation();
		const FLOAT NearDist = EMath::Max(Settings->LocalNear, 0.05f);
		const FLOAT FarDist = EMath::Max(LightData.AttenuationRadiusMeters, NearDist + 1.f);

		FLOAT FovYDegrees = 90.f;
		if (InRequest.Kind == EShadowViewKindType::SHADOW_VIEW_KIND_SPOT)
		{
			FovYDegrees = EMath::Clamp(2.f * LightData.SpotOuterAngleDegrees, 1.f, 170.f);
			OutRotation = GetWorldRotation();
		}
		else
		{
			// Cube face axes in the order the shader selects them: +X, -X, +Y, -Y, +Z, -Z.
			const Vector3 FaceDirections[6] =
			{
				Vector3(1.f, 0.f, 0.f), Vector3(-1.f, 0.f, 0.f),
				Vector3(0.f, 1.f, 0.f), Vector3(0.f, -1.f, 0.f),
				Vector3(0.f, 0.f, 1.f), Vector3(0.f, 0.f, -1.f)
			};
			const Vector3 FaceDirection = FaceDirections[EMath::Min(InRequest.Index, 5u)];
			const Vector3 UpDirection = ((EMath::Abs(FaceDirection.y) > 0.9f) ? Vector3(1.f, 0.f, 0.f) : Vector3(0.f, 1.f, 0.f));
			OutRotation = LookAtTargetQuaternionRotation(LightPosition + FaceDirection, LightPosition, UpDirection);
		}
		OutAnchor = LightPosition;

		EViewMatrix LightView;
		LightView.GenerateViewPart(OutAnchor, OutRotation);
		const FLOAT Resolution = static_cast<FLOAT>(EMath::Max(InResolution, 1u));
		const EViewport ProjectionViewport(0.f, 0.f, Resolution, Resolution
			, RCommonSettings::RENDER_DEPTH_NEAR, RCommonSettings::RENDER_DEPTH_FAR);

		OutDomain.ViewMatrix = EViewMatrix();
		OutDomain.ViewMatrix.ViewPart = LightView.ViewPart;
		OutDomain.ViewMatrix.GeneratePerspectiveProjectPart(ProjectionViewport, FovYDegrees, NearDist, FarDist);
		OutDomain.ViewMatrix.GenerateFinalMatrix();

		OutDomain.ViewFrustum = EFrustum();
		OutDomain.ViewFrustum.GeneratePerspectiveFrustumInfo(FovYDegrees, 1.f, NearDist, FarDist);
		ShadowFrustumToWorld(OutDomain.ViewMatrix, OutDomain.ViewFrustum);

		OutDomain.RenderViewport = ProjectionViewport;
		return TRUE;
	}

};
