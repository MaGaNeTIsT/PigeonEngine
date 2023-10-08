#pragma once

#include <CoreMinimal.h>
#include <TextureAsset/TextureAsset.h>
#include <PigeonBase/Object/Component/SceneComponent.h>

namespace PigeonEngine
{

	class RSkyLightSceneProxy;

	struct ERenderSkyLightMatrices : public ERenderTransformInfo
	{
	public:

	public:
		ERenderSkyLightMatrices(const Vector3& InWorldLocation, const Quaternion& InWorldRotation, const Vector3& InWorldScaling)
			: ERenderTransformInfo(InWorldLocation, InWorldRotation, InWorldScaling)
		{
		}
		ERenderSkyLightMatrices() = default;
		ERenderSkyLightMatrices(const ERenderSkyLightMatrices& Other)
			: ERenderTransformInfo(Other)
		{
		}
		ERenderSkyLightMatrices& operator=(const ERenderSkyLightMatrices& Other)
		{
			ERenderTransformInfo::CopyFromOther(Other);
			return (*this);
		}
	};

	struct ERenderSkyLightParams
	{
	public:
		Color3		LightAdjustColor;
		FLOAT		LightAdjustIntensity;
	public:
		ERenderSkyLightParams(const Color3& InLightAdjustColor, const FLOAT InLightAdjustIntensity)
			: LightAdjustColor(InLightAdjustColor), LightAdjustIntensity(InLightAdjustIntensity)
		{
		}
		ERenderSkyLightParams() = default;
		ERenderSkyLightParams(const ERenderSkyLightParams& Other)
			: LightAdjustColor(Other.LightAdjustColor), LightAdjustIntensity(Other.LightAdjustIntensity)
		{
		}
		ERenderSkyLightParams& operator=(const ERenderSkyLightParams& Other)
		{
			LightAdjustColor		= Other.LightAdjustColor;
			LightAdjustIntensity	= Other.LightAdjustIntensity;
			return (*this);
		}
	};

	class PSkyLightComponent : public PSceneComponent
	{
	public:
		const ETextureCubeAsset*	GetCubeMapAsset()const;
		const Color3&				GetLightAdjust()const;
		FLOAT						GetIntensity()const;
	public:
		void	SetCubeMapAsset(const ETextureCubeAsset* InCubeMapAsset);
		void	SetLightAdjust(const Color3& InColor);
		void	SetIntensity(const FLOAT InIntensity);
	protected:
		const ETextureCubeAsset*	CubeMapAsset;
		Color3						LightAdjust;
		FLOAT						Intensity;

		// Render proxy functions START
	public:
		enum PSkyLightUpdateState : UINT8
		{
			SKY_LIGHT_UPDATE_STATE_NONE		= 0,
			SKY_LIGHT_UPDATE_STATE_TEXTURE	= (1 << 0),
			SKY_LIGHT_UPDATE_STATE_PARAMS	= (1 << 1),
			SKY_LIGHT_UPDATE_STATE_MATRIX	= (1 << 2)
		};
	public:
		RSkyLightSceneProxy*			SceneProxy;	// ONLY render thread could use this proxy pointer.
	public:
		UINT8							GetUpdateRenderState()const;
		RSkyLightSceneProxy*			CreateSceneProxy();
		virtual void					CreateRenderState()override;
		virtual void					DestroyRenderState()override;
		virtual void					SendUpdateRenderState()override;
	protected:
		void			MarkAsDirty(PSkyLightUpdateState InState);
		virtual void	MarkRenderTransformAsDirty()override;
		virtual void	MarkRenderStateAsDirty()override;
		virtual void	CleanMarkRenderStateDirty()override;
	protected:
		UINT8							UpdateState;
		// Render proxy functions END

		CLASS_VIRTUAL_NOCOPY_BODY(PSkyLightComponent)
	};

};