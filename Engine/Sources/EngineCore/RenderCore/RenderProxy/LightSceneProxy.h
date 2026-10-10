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
	extern void	EncodeDirectionalLight(const ELightData& InData, const Quaternion& InWorldRotation, EDirectionalLightsGPU& OutElement) noexcept;
	// A local light needs the world position as well, because its falloff is measured from it.
	// A point light stores a direction of zero and a cosine pair of -1; nothing reads either,
	// because the type field is what the shading code branches on.
	extern void	EncodeLocalLight(const ELightData& InData, const Vector3& InWorldLocation, const Quaternion& InWorldRotation, ELocalLightsGPU& OutElement) noexcept;

	class RDirectionalLightSceneProxy : public RBaseSceneProxy
	{
	public:
		using RViewSetType				= TSet<const RViewProxy*>;
		using RPerViewDomainInfoType	= TMap<ObjectIdentityType, TArray<EViewDomainInfo>>;
		using RVisibilityMapType		= TMap<ObjectIdentityType, BOOL32>;
		using RPerViewVisibilityMapType	= TMap<ObjectIdentityType, TArray<RVisibilityMapType>>;
	public:
		RDirectionalLightSceneProxy(const PDirectionalLightComponent* InComponent);
		void	SetupProxy(const ERenderDirectionalLightMatrices& InMatrices, const ERenderLightParams& InParams, const ECascadeShadowData* InCascadeShadowData);
	public:
		void	GenerateViewInfo(const RViewProxy* InViewProxy);
	public:
		const RPerViewVisibilityMapType&	GetVisibilityMap()const;
		RPerViewVisibilityMapType&			GetVisibilityMap();
		const ELightData&					GetLightData()const;
		RPerViewDomainInfoType&				GetViewDomainInfos();
		const RPerViewDomainInfoType&		GetViewDomainInfos()const;
		const ECascadeShadowData*			GetCascadeShadowData()const;
		BOOL32								IsLightCastShadow()const;
		BOOL32								IsLightUseCascadeShadow()const;
		void								MarkNeedUpdateParams()const;
		BOOL32								IsNeedUpdateParams()const;
		RViewSetType&						GetViews();
		const RViewSetType&					GetViews()const;
	public:
		void	UpdateMatrices(const ERenderDirectionalLightMatrices& InMatrices);
		void	UpdateLightParams(const ERenderLightParams& InParams);
		void	UpdateCascadeData(const ECascadeShadowData* InCascadeShadowData);
	protected:
		PE_MUTABLE BOOL32					NeedUpdateParams;
		RPerViewVisibilityMapType			VisibilityMap;
	protected:
		ELightData							LightData;
		RPerViewDomainInfoType				ViewDomainInfos;
		ECascadeShadowData*					CascadeShadowData;
	protected:
		BOOL32								IsCascadeShadow;
		RViewSetType						Views;
		const PDirectionalLightComponent*	Component;

		RENDER_PROXY_CLASS_BODY(RDirectionalLightSceneProxy)

	};

	/*
	* A local light on the render side: a point or a spot, which is what the element's own type
	* field says. One proxy type for both, because they share one gpu list and one set of scene
	* calls - the shading side reads the type out of the element, so the renderer never has to
	* know which it is holding.
	*
	* There is no view info and no culling here yet. A directional light generates shadow domains
	* because it has cascades; a local light has neither a shadow view nor a per view split until
	* shadows land, and inventing either now would be a layout to keep in step with no reader.
	*/
	class RLocalLightSceneProxy : public RBaseSceneProxy
	{
	public:
		RLocalLightSceneProxy(const PLocalLightComponent* InComponent);
		void	SetupProxy(const ERenderTransformInfo& InTransform, const ELightData& InData);
	public:
		const ELightData&	GetLightData() const;
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
