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
	const ELightData& RDirectionalLightSceneProxy::GetLightData() const
	{
		return LightData;
	}
	const ECascadeShadowData* RDirectionalLightSceneProxy::GetCascadeShadowData() const
	{
		return CascadeShadowData;
	}
	BOOL32 RDirectionalLightSceneProxy::IsLightCastShadow() const
	{
		return (LightData.CastShadow);
	}
	BOOL32 RDirectionalLightSceneProxy::IsLightUseCascadeShadow() const
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
	void RDirectionalLightSceneProxy::MarkNeedUpdateParams() const
	{
		NeedUpdateParams = TRUE;
	}
	BOOL32 RDirectionalLightSceneProxy::IsNeedUpdateParams() const
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
#if ENGINE_EDITOR_ONLY
			if (InCascadeShadowData)
#endif
			{
#if ENGINE_EDITOR_ONLY
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
	FLOAT RLocalLightSceneProxy::ComputeScreenCoverage(const RViewProxy& InView) const
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

	BOOL32 RDirectionalLightSceneProxy::CollectShadowViews(const RViewProxy& InView, TArray<FShadowViewRequest>& OutRequests) const
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
				// The ladder is authoritative, including its last layer. A cascade list that
				// extended its last cascade to the camera's far plane would spread that tile over
				// the whole depth range: the far plane is 200000 units, so the last cascade would
				// carry metres per texel and the near cascades would be the only usable ones.
				// The distance past the ladder is left unshadowed rather than covered by a tile
				// that cannot resolve anything; a static shadow solution owns that range.
				SplitEnd = EMath::Min(SplitCursor
					+ ShadowLayerThickness(*CascadeShadowData, CascadeIndex, Settings->DirectionalCascadeDistanceScale), CameraFar);
			}
			Request.SplitDistance = SplitEnd;
			OutRequests.Add(Request);
			SplitCursor = SplitEnd;
		}
		return TRUE;
	}
	/*
	* The extent of an AABB in one view's space.
	*
	* It exists for the two places a directional cascade needs a bound it did not derive from
	* the camera slice: the conservative frustum reaches the casters the frame can draw, and the
	* refit fits the tile to the casters the cull actually kept.
	*/
	struct FShadowViewSpaceBounds
	{
		Vector3	Min;
		Vector3	Max;
		FShadowViewSpaceBounds() noexcept
			: Min(PE_FLOAT32_MAX, PE_FLOAT32_MAX, PE_FLOAT32_MAX)
			, Max(-PE_FLOAT32_MAX, -PE_FLOAT32_MAX, -PE_FLOAT32_MAX)
		{
		}
	};
	static void AccumulateShadowBoundInView(const EBoundAABB& InBounds, const EViewMatrix& InView
		, FShadowViewSpaceBounds& InOutBounds)
	{
		if (InBounds.IsValid == FALSE)
		{
			return;
		}
		for (UINT32 CornerIndex = 0u; CornerIndex < 8u; CornerIndex++)
		{
			const Vector3 Corner(
				(((CornerIndex & 1u) != 0u) ? InBounds.AABBMax.x : InBounds.AABBMin.x),
				(((CornerIndex & 2u) != 0u) ? InBounds.AABBMax.y : InBounds.AABBMin.y),
				(((CornerIndex & 4u) != 0u) ? InBounds.AABBMax.z : InBounds.AABBMin.z));
			Vector3 ViewCorner;
			InView.TransformWorldPointToView(Corner, ViewCorner);
			InOutBounds.Min = MinVector3(InOutBounds.Min, ViewCorner);
			InOutBounds.Max = MaxVector3(InOutBounds.Max, ViewCorner);
		}
	}

	BOOL32 RDirectionalLightSceneProxy::BuildShadowViewDomain(const RViewProxy& InView, const FShadowViewRequest& InRequest
		, const UINT32 InResolution, const EBoundAABB& InCasterBounds, const BOOL32 InAreCasterBoundsComplete
		, EViewDomainInfo& OutDomain, Vector3& OutAnchor, Quaternion& OutRotation) const
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
			// The last layer's thickness bounds the last cascade here for the same reason it
			// bounds its request: the ladder states where CSM ends, and the range past it is
			// another system's. Reaching the far plane would make this fit meaningless.
			SplitFar = EMath::Min(Cursor
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

		// The light view's orientation is the light's travel direction, not the component's
		// forward axis: a directional light keeps that axis pointed back at its emitter (that is
		// what EncodeDirectionalLight negates), and a shadow view built from it looks up towards
		// the sun. The depths it would write are the far side of every caster measured from the
		// wrong end of the axis, so the receiver's comparison would come out fully lit for the
		// whole frame. The up axis is kept so the projection stays upright.
		const Vector3 TravelDirection = -SplitForwardVector(GetWorldRotation());
		const Vector3 UpDirection = SplitUpVector(GetWorldRotation());
		OutRotation = LookAtTargetQuaternionRotation(TravelDirection, Vector3::Zero(), UpDirection);

		// A directional cascade has to be fitted in a frame that is fixed in the world, or the
		// texel grid follows the camera and the shadows of static casters crawl as it moves. The
		// fit is a sphere around the slice: its radius is invariant under camera rotation and
		// translation, and snapping the centre in the light's world basis puts the texels on a
		// grid that does not move. Snapping the min corner of a camera-anchored view cannot do
		// that - the grid it aligns to moves with the camera.
		Vector3 SliceCenter = Vector3::Zero();
		for (UINT32 CornerIndex = 0u; CornerIndex < 8u; CornerIndex++)
		{
			SliceCenter += Corners[CornerIndex];
		}
		SliceCenter = SliceCenter / 8.f;

		EViewMatrix WorldLightView;
		WorldLightView.GenerateViewPart(Vector3::Zero(), OutRotation);
		Vector3 LightSpaceCenter;
		WorldLightView.TransformWorldPointToView(SliceCenter, LightSpaceCenter);

		const FLOAT Resolution = static_cast<FLOAT>(EMath::Max(InResolution, 1u));
		FLOAT SphereRadius = 0.f;
		for (UINT32 CornerIndex = 0u; CornerIndex < 8u; CornerIndex++)
		{
			SphereRadius = EMath::Max(SphereRadius, Vector3::Length(Corners[CornerIndex] - SliceCenter));
		}
		SphereRadius = EMath::Max(SphereRadius, 1e-3f);
		// One texel of slack: the snapped centre can move by up to half a texel on each axis,
		// and the slice has to stay inside the box after that move.
		FLOAT TexelSize = ((2.f * SphereRadius) / Resolution);
		SphereRadius += TexelSize;
		TexelSize = ((2.f * SphereRadius) / Resolution);

		LightSpaceCenter.x = EMath::RoundToFloat(LightSpaceCenter.x / TexelSize) * TexelSize;
		LightSpaceCenter.y = EMath::RoundToFloat(LightSpaceCenter.y / TexelSize) * TexelSize;
		Vector3 SnappedCenter;
		WorldLightView.TransformViewPointToWorld(LightSpaceCenter, SnappedCenter);

		OutAnchor = SnappedCenter;
		EViewMatrix LightView;
		LightView.GenerateViewPart(OutAnchor, OutRotation);

		// The depth range still comes from the slice, because the light direction is not the
		// view direction of anything else. The x and y extents are the sphere by construction.
		Vector3 ViewMin(PE_FLOAT32_MAX, PE_FLOAT32_MAX, PE_FLOAT32_MAX);
		Vector3 ViewMax(-PE_FLOAT32_MAX, -PE_FLOAT32_MAX, -PE_FLOAT32_MAX);
		for (UINT32 CornerIndex = 0u; CornerIndex < 8u; CornerIndex++)
		{
			Vector3 ViewCorner;
			LightView.TransformWorldPointToView(Corners[CornerIndex], ViewCorner);
			ViewMin = MinVector3(ViewMin, ViewCorner);
			ViewMax = MaxVector3(ViewMax, ViewCorner);
		}
		// The slice bounds the receivers, not the casters. An occluder anywhere between the light
		// and a receiver shadows it, so the near side of this conservative frustum has to reach
		// the frame's caster bounds; the far side is the slice's far corner, because nothing
		// behind it can shadow what is in front of it. The fixed padding stays as a safety
		// margin for the bound itself.
		ViewMin.z -= Settings->DepthPadding;
		FShadowViewSpaceBounds CasterViewBounds;
		AccumulateShadowBoundInView(InCasterBounds, LightView, CasterViewBounds);
		if (CasterViewBounds.Min.z < PE_FLOAT32_MAX)
		{
			ViewMin.z = EMath::Min(ViewMin.z, CasterViewBounds.Min.z);
		}
		if (InAreCasterBoundsComplete == FALSE)
		{
			// A caster that cannot state a bound - a skinned mesh today - can sit anywhere within
			// the camera's reach. The far plane is the only finite bound the renderer has for it,
			// so the near side is extended by it rather than left to the authored padding.
			ViewMin.z = EMath::Min(ViewMin.z, (ViewMax.z - CameraFar));
		}

		OutDomain.ViewMatrix = EViewMatrix();
		OutDomain.ViewMatrix.ViewPart = LightView.ViewPart;
		OutDomain.ViewMatrix.GenerateOrthographicProjectPart(-SphereRadius, SphereRadius, SphereRadius, -SphereRadius, ViewMin.z, ViewMax.z);
		OutDomain.ViewMatrix.GenerateFinalMatrix();

		OutDomain.ViewFrustum = EFrustum();
		OutDomain.ViewFrustum.GenerateOrthographicFrustumInfo(-SphereRadius, SphereRadius, SphereRadius, -SphereRadius, ViewMin.z, ViewMax.z);
		ShadowFrustumToWorld(OutDomain.ViewMatrix, OutDomain.ViewFrustum);

		OutDomain.RenderViewport = EViewport(0.f, 0.f, Resolution, Resolution
			, RCommonSettings::RENDER_DEPTH_NEAR, RCommonSettings::RENDER_DEPTH_FAR);
		return TRUE;
	}

	BOOL32 RDirectionalLightSceneProxy::RefitShadowViewDomain(const RViewProxy& InView, const FShadowViewRequest& InRequest
		, const UINT32 InResolution, const EBoundAABB& InDrawnBounds
		, EViewDomainInfo& InOutDomain) const
	{
		(void)InView;
		(void)InRequest;
		if (InDrawnBounds.IsValid == FALSE)
		{
			return FALSE;
		}

		// The view is unchanged; only the projection is fitted to the drawn casters. The depth
		// range comes from the conservative frustum this projection replaces - its corners are
		// world space already - so every receiver the conservative pass could reach keeps a
		// depth inside [0, 1] after the lateral refit.
		const EViewMatrix& LightView = InOutDomain.ViewMatrix;
		FShadowViewSpaceBounds ConservativeBounds;
		for (UINT32 CornerIndex = 0u; CornerIndex < 8u; CornerIndex++)
		{
			Vector3 ViewCorner;
			LightView.TransformWorldPointToView(InOutDomain.ViewFrustum.FarNearPlanePoint[CornerIndex], ViewCorner);
			ConservativeBounds.Min = MinVector3(ConservativeBounds.Min, ViewCorner);
			ConservativeBounds.Max = MaxVector3(ConservativeBounds.Max, ViewCorner);
		}
		if ((ConservativeBounds.Max.z - ConservativeBounds.Min.z) < 1e-3f)
		{
			return FALSE;
		}

		FShadowViewSpaceBounds DrawnBounds;
		AccumulateShadowBoundInView(InDrawnBounds, LightView, DrawnBounds);
		if (DrawnBounds.Min.x > DrawnBounds.Max.x)
		{
			return FALSE;
		}

		// The tile is square and the sampling code maps the projection's normalized coordinates
		// onto it with one texel size, so the fit is square too: the larger of the two extents
		// decides it. The margin covers the receiver's normal offset and the half texel the
		// centre snap below can move.
		const FLOAT Resolution = static_cast<FLOAT>(EMath::Max(InResolution, 1u));
		const FLOAT CenterX = 0.5f * (DrawnBounds.Min.x + DrawnBounds.Max.x);
		const FLOAT CenterY = 0.5f * (DrawnBounds.Min.y + DrawnBounds.Max.y);
		FLOAT HalfExtent = 0.5f * EMath::Max(DrawnBounds.Max.x - DrawnBounds.Min.x
			, DrawnBounds.Max.y - DrawnBounds.Min.y);
		HalfExtent = EMath::Max(HalfExtent, 1e-3f);
		FLOAT TexelSize = ((2.f * HalfExtent) / Resolution);
		HalfExtent += (2.f * TexelSize);
		TexelSize = ((2.f * HalfExtent) / Resolution);

		// Snap the centre to the light's texel grid so a moving camera does not make a static
		// shadow crawl; the margin above keeps the drawn bounds inside the snapped box.
		const FLOAT SnappedCenterX = EMath::RoundToFloat(CenterX / TexelSize) * TexelSize;
		const FLOAT SnappedCenterY = EMath::RoundToFloat(CenterY / TexelSize) * TexelSize;
		const FLOAT Left	= SnappedCenterX - HalfExtent;
		const FLOAT Right	= SnappedCenterX + HalfExtent;
		const FLOAT Top		= SnappedCenterY + HalfExtent;
		const FLOAT Bottom	= SnappedCenterY - HalfExtent;

		InOutDomain.ViewMatrix.GenerateOrthographicProjectPart(Left, Top, Right, Bottom
			, ConservativeBounds.Min.z, ConservativeBounds.Max.z);
		InOutDomain.ViewMatrix.GenerateFinalMatrix();

		InOutDomain.ViewFrustum = EFrustum();
		InOutDomain.ViewFrustum.GenerateOrthographicFrustumInfo(Left, Top, Right, Bottom
			, ConservativeBounds.Min.z, ConservativeBounds.Max.z);
		ShadowFrustumToWorld(InOutDomain.ViewMatrix, InOutDomain.ViewFrustum);
		return TRUE;
	}

	BOOL32 RLocalLightSceneProxy::CollectShadowViews(const RViewProxy& InView, TArray<FShadowViewRequest>& OutRequests) const
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
		, const UINT32 InResolution, const EBoundAABB& InCasterBounds, const BOOL32 InAreCasterBoundsComplete
		, EViewDomainInfo& OutDomain, Vector3& OutAnchor, Quaternion& OutRotation) const
	{
		(void)InView;
		// A local light's shadow view is its own shape - one perspective cone for a spot, one
		// 90 degree face for a point - so the frame's caster bounds say nothing about it. The
		// conservative domain here is already the exact one the receiver samples.
		(void)InCasterBounds;
		(void)InAreCasterBoundsComplete;
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
