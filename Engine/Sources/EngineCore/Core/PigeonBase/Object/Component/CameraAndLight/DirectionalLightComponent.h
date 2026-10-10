#pragma once

#include <CoreMinimal.h>
#include <EngineCommon.h>
#include "LightComponent.h"

namespace PigeonEngine
{

	class RDirectionalLightSceneProxy;

	struct ERenderDirectionalLightMatrices : public ERenderTransformInfo
	{
	public:

	public:
		ERenderDirectionalLightMatrices(const Vector3& InWorldLocation, const Quaternion& InWorldRotation, const Vector3& InWorldScaling)
			: ERenderTransformInfo(InWorldLocation, InWorldRotation, InWorldScaling)
		{
		}
		ERenderDirectionalLightMatrices() = default;
		ERenderDirectionalLightMatrices(const ERenderDirectionalLightMatrices& Other)
			: ERenderTransformInfo(Other)
		{
		}
		ERenderDirectionalLightMatrices& operator=(const ERenderDirectionalLightMatrices& Other)
		{
			ERenderTransformInfo::CopyFromOther(Other);
			return (*this);
		}
	};

	struct ERenderLightParams
	{
	public:
		Color3		LightColor;				// normalized chromaticity, not a magnitude
		FLOAT		IlluminanceLux;			// perpendicular illuminance [lux]
		FLOAT		SourceAngleDegrees;		// angular diameter of the disc, 0 = a delta light
		BOOL32		CastShadow;
		Vector2Int	ShadowMapSize;
	public:
		ERenderLightParams(const Color3& InLightColor, const FLOAT InIlluminanceLux, const FLOAT InSourceAngleDegrees
			, const BOOL32 InCastShadow, const Vector2Int& InShadowMapSize)
			: LightColor(InLightColor), IlluminanceLux(InIlluminanceLux)
			, SourceAngleDegrees(InSourceAngleDegrees)
			, CastShadow(InCastShadow), ShadowMapSize(InShadowMapSize)
		{
		}
		ERenderLightParams() = default;
		ERenderLightParams(const ERenderLightParams& Other)
			: LightColor(Other.LightColor), IlluminanceLux(Other.IlluminanceLux)
			, SourceAngleDegrees(Other.SourceAngleDegrees)
			, CastShadow(Other.CastShadow), ShadowMapSize(Other.ShadowMapSize)
		{
		}
		ERenderLightParams& operator=(const ERenderLightParams& Other)
		{
			LightColor			= Other.LightColor;
			IlluminanceLux		= Other.IlluminanceLux;
			SourceAngleDegrees	= Other.SourceAngleDegrees;
			CastShadow			= Other.CastShadow;
			ShadowMapSize		= Other.ShadowMapSize;
			return (*this);
		}
	};

	/*
	* A directional light: a light so far away that its rays are parallel and its distance does
	* not change its brightness. Its direction is the component's forward axis, so aiming it is
	* rotating it.
	*
	* What it adds over the base is the two physical values that only a directional light has -
	* the illuminance perpendicular to it, and the angular size of the disc it stands for - plus
	* the cascade shadow settings. Its direction is not a separate field: it comes from the
	* transform like a local light's does.
	*/
	class PDirectionalLightComponent : public PLightComponent
	{
	public:
		// Perpendicular illuminance in lux. Daylight values: 100000 in direct sun, 10000 under
		// overcast, 0.25 in full moonlight.
		FLOAT						GetIlluminanceLux() const noexcept;
		// Angular diameter of the light's disc, which is the input both to the specular lobe's
		// width and to the penumbra a soft shadow will have. The sun is 0.53 degrees across;
		// zero means a delta light, which is what a dome light or a distant moon wants.
		FLOAT						GetSourceAngleDegrees() const noexcept;
		ECascadeShadowData*			GetCascadeShadowData() noexcept;
		const ECascadeShadowData*	GetCascadeShadowData() const noexcept;
	public:
		void	SetIlluminanceLux(const FLOAT InIlluminanceLux);
		void	SetSourceAngleDegrees(const FLOAT InSourceAngleDegrees);
	public:
		// Render proxy functions START
	public:
		RDirectionalLightSceneProxy*	SceneProxy;	// ONLY render thread could use this proxy pointer.
	public:
		RDirectionalLightSceneProxy*	CreateSceneProxy();
	protected:
		virtual void	AddToRenderScene()override;
		virtual void	RemoveFromRenderScene()override;
		virtual void	UpdateInRenderScene()override;
		// Render proxy functions END
	protected:
		ECascadeShadowData*	CascadeShadowData;

		CLASS_VIRTUAL_NOCOPY_BODY(PDirectionalLightComponent)
	};

};
