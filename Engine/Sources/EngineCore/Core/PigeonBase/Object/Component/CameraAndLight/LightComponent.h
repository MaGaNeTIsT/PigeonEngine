#pragma once

#include <CoreMinimal.h>
#include <EngineCommon.h>
#include <PigeonBase/Object/Component/SceneComponent.h>

namespace PigeonEngine
{

	class RLocalLightSceneProxy;

	/*
	* The base of every light component.
	*
	* A light type brings two things: the setters that mean something for it, and the three
	* calls that put it in and take it out of the render scene's light list. Everything else -
	* the data, the dirty tracking, the render state lifecycle - is the same for all of them
	* and is written once here, which is what keeps a new light type from being a copy of the
	* one before it.
	*
	* The data is the engine's ELightData: one struct for every light type, with the fields a
	* type does not use left at their defaults. A struct per type would be three types to keep
	* in step with one gpu layout; this way there is one, and the field a type reads is the
	* unit-explicit one that belongs to it.
	*/
	class PLightComponent : public PSceneComponent
	{
	public:
		enum PLightUpdateState : UINT8
		{
			LIGHT_UPDATE_STATE_NONE		= 0,
			LIGHT_UPDATE_STATE_MATRIX	= (1 << 0),
			LIGHT_UPDATE_STATE_DATA		= (1 << 1),
			LIGHT_UPDATE_STATE_CASCADE	= (1 << 2)
		};
	public:
		ELightType			GetLightType() const noexcept;
		const Color3&		GetLightColor() const noexcept;
		BOOL32				IsLightCastShadow() const noexcept;
		const Vector2Int&	GetShadowMapSize() const noexcept;
		const ELightData&	GetLightData() const noexcept;
	public:
		// Shared by every type. Each marks the data dirty, which is what makes the render proxy
		// copy the new values on the next update rather than reading the component from the
		// render thread.
		void	SetLightColor(const Color3& InColor);
		void	SetShadowMapSize(const BOOL32 InIsCastShadow, const Vector2Int& InShadowMapSize);
	public:
		UINT8			GetUpdateRenderState() const noexcept;
		virtual void	CreateRenderState() override;
		virtual void	DestroyRenderState() override;
		virtual void	SendUpdateRenderState() override;
	protected:
		void			MarkAsDirty(PLightUpdateState InState);
		virtual void	MarkRenderTransformAsDirty() override;
		virtual void	MarkRenderStateAsDirty() override;
		virtual void	CleanMarkRenderStateDirty() override;
	protected:
		// Where this type belongs. The lifecycle above is the same for every light, and the only
		// thing that differs is which list of the render scene it lives in, so that is all a
		// derived class has to answer.
		virtual void	AddToRenderScene() = 0;
		virtual void	RemoveFromRenderScene() = 0;
		virtual void	UpdateInRenderScene() = 0;
	protected:
		ELightData	LightData;
		UINT8		UpdateState;

		CLASS_VIRTUAL_NOCOPY_BODY(PLightComponent)
	};

	/*
	* The base of the two local light types.
	*
	* Point and spot differ in one thing that anyone downstream can see: whether the stored
	* cone is consulted. Everything else - the falloff, the radius, the way the light enters and
	* leaves the scene's list - is identical, so it is written once here and the two concrete
	* types are almost entirely a constructor and a name.
	*
	* Both live in one render list, which is why there is one proxy type and one set of scene
	* calls rather than one per type. The element carries its own type, so the shading side
	* branches on the data rather than the renderer having to.
	*/
	class PLocalLightComponent : public PLightComponent
	{
	public:
		// Luminous intensity in candela.
		FLOAT	GetLuminousIntensityCandela() const noexcept;
		// The distance past which the light is not shaded at all. Physical falloff never reaches
		// zero, so this is a cutoff: it bounds the cost of a light whose contribution has become
		// invisible, and the shading wraps the falloff so the contribution arrives at exactly
		// zero there rather than stepping.
		FLOAT	GetAttenuationRadiusMeters() const noexcept;
		// The emitter's radius, in metres. What it buys is a specular highlight with a size:
		// a mirror reflecting a sphere light returns a disc. Zero is an ideal point.
		FLOAT	GetSourceRadiusMeters() const noexcept;
	public:
		void	SetLuminousIntensityCandela(const FLOAT InLuminousIntensityCandela);
		// The unit a lamp is sold in. A local light that radiates into the whole sphere relates
		// the two as I = Phi / (4 pi); a spot overrides this, because its flux is spread over a
		// cone instead.
		virtual void	SetLuminousFluxLumen(const FLOAT InLuminousFluxLumen);
		void	SetAttenuationRadiusMeters(const FLOAT InAttenuationRadiusMeters);
		void	SetSourceRadiusMeters(const FLOAT InSourceRadiusMeters);
	public:
		RLocalLightSceneProxy*	SceneProxy;
	public:
		RLocalLightSceneProxy*	CreateSceneProxy();
	protected:
		virtual void	AddToRenderScene() override;
		virtual void	RemoveFromRenderScene() override;
		virtual void	UpdateInRenderScene() override;

		CLASS_VIRTUAL_NOCOPY_BODY(PLocalLightComponent)
	};

}   // namespace PigeonEngine
