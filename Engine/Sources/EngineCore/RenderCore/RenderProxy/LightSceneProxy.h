#pragma once

#include <CoreMinimal.h>
#include <EngineCommon.h>
#include "BaseSceneProxy.h"
#include <Shader/ShaderParameter.h>
#include <PigeonBase/Object/Component/CameraAndLight/LightComponent.h>
#include <PigeonBase/Object/Component/CameraAndLight/DirectionalLightComponent.h>

namespace PigeonEngine
{

	class RScene;
	class RViewProxy;
	class RDirectionalLightSceneProxy;

	// ---------------------------------------------------------------------------
	// One element of a GPU light list.
	//
	// The layout is declared in Assets/ShaderSources/Components/C_Lighting.json and the two
	// sides have to be changed together. Three properties are what keep them in step:
	//
	//   1. EVERY MEMBER IS ONE 16 BYTE SLOT. The element size is therefore the field count
	//      times sixteen, which lets the debug check in
	//      RSceneRenderer::ValidateLightBufferLayout compare this struct against the shader's
	//      reflected field list without knowing any field type. A float3 followed by a float
	//      would fill the same register, but only under one compiler's packing rule.
	//   2. THE ORDER IS THE LAYOUT. There is no padding and no alignment gap to reason about.
	//   3. A FIELD THAT IS NOT A FLOAT IS STORED AS ITS BITS, so the shader can asfloat it
	//      back. That is why the two integer slots are written through EMath::AsUint32 and not
	//      through a numeric conversion, which would round every cosine in the cone to zero.
	//
	// The units are photometric and stated per field: illuminance in lux, luminous intensity
	// in candela. See Public/Lighting/ShaderLightData.hlsl for why the pipeline needs no
	// luminous efficacy constant.
	// ---------------------------------------------------------------------------

	struct EDirectionalLightsGPU
	{
		Vector4		DirectionAndIlluminanceLux;		// .xyz the direction the light travels, .w [lux]
		Vector4		ColorAndSourceAngleDegrees;		// .xyz normalized chromaticity, .w disc diameter [deg]
		Vector4Int	ShadowAddressing;				// .x shadow map index, .y cascade count, .z/.w spare
	};
	static_assert(sizeof(EDirectionalLightsGPU) == 48u, "EDirectionalLightsGPU must be three 16 byte slots");

	struct ELocalLightsGPU
	{
		Vector4		WorldPositionAndAttenuationRadiusMeters;	// .xyz world position, .w [m]
		Vector4		ColorAndLuminousIntensityCandela;			// .xyz normalized chromaticity, .w [candela]
		Vector4		WorldDirectionAndSourceRadiusMeters;		// .xyz spot axis, .w emitter radius [m]
		Vector4Int	SpotAndTypeParams;							// .x type, .y bits(cos inner), .z bits(cos outer), .w shadow map index
	};
	static_assert(sizeof(ELocalLightsGPU) == 64u, "ELocalLightsGPU must be four 16 byte slots");

	// Fills one GPU element from a light's authored values.
	//
	// The direction is the component's forward axis, so rotating a light in the world is how
	// its aim is authored - the same convention the shadow view setup already used. A
	// directional light stores the direction the light travels, which is the negation of the
	// axis it points along.
	//
	// InShadowSlot is the index into the per view shadow slot table, or zero for a light that
	// casts no shadow in this frame - zero is the table's "no shadow" sentinel, which is why real
	// slots start at one. The renderer assigns it; the light only carries the address.
	extern void	EncodeDirectionalLight(const ELightData& InData, const Quaternion& InWorldRotation
		, const UINT32 InShadowSlot, const UINT32 InCascadeCount, EDirectionalLightsGPU& OutElement) noexcept;
	// A local light needs the world position as well, because its falloff is measured from it.
	// A point light stores a direction of zero and a cosine pair of -1; nothing reads either,
	// because the type field is what the shading code branches on.
	extern void	EncodeLocalLight(const ELightData& InData, const Vector3& InWorldLocation, const Quaternion& InWorldRotation
		, const UINT32 InShadowSlot, ELocalLightsGPU& OutElement) noexcept;

	class RDirectionalLightSceneProxy : public RBaseSceneProxy
	{
	public:
		RDirectionalLightSceneProxy(const PDirectionalLightComponent* InComponent);
		void	SetupProxy(const ERenderDirectionalLightMatrices& InMatrices, const ERenderLightParams& InParams, const ECascadeShadowData* InCascadeShadowData);
	public:
		virtual BOOL32	CollectShadowViews(const RViewProxy& InView, TArray<FShadowViewRequest>& OutRequests) const override;
		virtual BOOL32	BuildShadowViewDomain(const RViewProxy& InView, const FShadowViewRequest& InRequest
			, const UINT32 InResolution, const EBoundAABB& InCasterBounds, const BOOL32 InAreCasterBoundsComplete
			, EViewDomainInfo& OutDomain, Vector3& OutAnchor, Quaternion& OutRotation) const override;
		// A cascade is orthographic, so the drawn casters' light space x and y bound every
		// shadow they can cast. This is the one proxy kind whose projection the renderer re-fits
		// after the cull has said what it actually draws.
		virtual BOOL32	RefitShadowViewDomain(const RViewProxy& InView, const FShadowViewRequest& InRequest
			, const UINT32 InResolution, const EBoundAABB& InDrawnBounds
			, EViewDomainInfo& InOutDomain) const override;
	public:
		const ELightData&					GetLightData() const;
		const ECascadeShadowData*			GetCascadeShadowData() const;
		BOOL32								IsLightCastShadow() const;
		BOOL32								IsLightUseCascadeShadow() const;
		void								MarkNeedUpdateParams() const;
		BOOL32								IsNeedUpdateParams() const;
	public:
		void	UpdateMatrices(const ERenderDirectionalLightMatrices& InMatrices);
		void	UpdateLightParams(const ERenderLightParams& InParams);
		void	UpdateCascadeData(const ECascadeShadowData* InCascadeShadowData);
	protected:
		PE_MUTABLE BOOL32					NeedUpdateParams	= FALSE;
	protected:
		ELightData							LightData;
		ECascadeShadowData*					CascadeShadowData;
	protected:
		BOOL32								IsCascadeShadow;
		const PDirectionalLightComponent*	Component;

		RENDER_PROXY_CLASS_BODY(RDirectionalLightSceneProxy)

	};

	/*
	* A local light on the render side: a point or a spot, which is what the element's own type
	* field says. One proxy type for both, because they share one gpu list and one set of scene
	* calls - the shading side reads the type out of the element, so the renderer never has to
	* know which it is holding.
	*/
	class RLocalLightSceneProxy : public RBaseSceneProxy
	{
	public:
		RLocalLightSceneProxy(const PLocalLightComponent* InComponent);
		void	SetupProxy(const ERenderTransformInfo& InTransform, const ELightData& InData);
	public:
		virtual BOOL32	CollectShadowViews(const RViewProxy& InView, TArray<FShadowViewRequest>& OutRequests) const override;
		virtual BOOL32	BuildShadowViewDomain(const RViewProxy& InView, const FShadowViewRequest& InRequest
			, const UINT32 InResolution, const EBoundAABB& InCasterBounds, const BOOL32 InAreCasterBoundsComplete
			, EViewDomainInfo& OutDomain, Vector3& OutAnchor, Quaternion& OutRotation) const override;
	public:
		const ELightData&	GetLightData() const;
		// How much of the given view this light covers, as the approximate pixel radius of its
		// attenuation sphere on screen. The renderer ranks shadow candidates by it, so a light
		// whose shadow the frame cannot afford to keep is always the smallest one on screen.
		PE_NODISCARD FLOAT	ComputeScreenCoverage(const RViewProxy& InView) const;
		void				MarkNeedUpdateParams() const;
		BOOL32				IsNeedUpdateParams() const;
	public:
		void	UpdateTransform(const ERenderTransformInfo& InTransform);
		void	UpdateLightParams(const ELightData& InData);
	protected:
		PE_MUTABLE BOOL32					bNeedUpdateParams;
	protected:
		ELightData							LightData;
	protected:
		const PLocalLightComponent*			Component;

		RENDER_PROXY_CLASS_BODY(RLocalLightSceneProxy)

	};

};
