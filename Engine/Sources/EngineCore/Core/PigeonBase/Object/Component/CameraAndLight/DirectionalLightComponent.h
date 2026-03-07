#pragma once

#include <CoreMinimal.h>
#include <EngineCommon.h>
#include <PigeonBase/Object/Component/SceneComponent.h>

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
		Color3		LightColor;
		FLOAT		LightIntensity;
		BOOL32		CastShadow;
		Vector2Int	ShadowMapSize;
	public:
		ERenderLightParams(const Color3& InLightColor, const FLOAT InLightIntensity
			, const BOOL32 InCastShadow, const Vector2Int& InShadowMapSize)
			: LightColor(InLightColor), LightIntensity(InLightIntensity)
			, CastShadow(InCastShadow), ShadowMapSize(InShadowMapSize)
		{
		}
		ERenderLightParams() = default;
		ERenderLightParams(const ERenderLightParams& Other)
			: LightColor(Other.LightColor), LightIntensity(Other.LightIntensity)
			, CastShadow(Other.CastShadow), ShadowMapSize(Other.ShadowMapSize)
		{
		}
		ERenderLightParams& operator=(const ERenderLightParams& Other)
		{
			LightColor		= Other.LightColor;
			LightIntensity	= Other.LightIntensity;
			CastShadow		= Other.CastShadow;
			ShadowMapSize	= Other.ShadowMapSize;
			return (*this);
		}
	};

	class PDirectionalLightComponent : public PSceneComponent
	{
	public:
		ELightType					GetLightType()const;
		const Color3&				GetLightColor()const;
		FLOAT						GetLightIntensity()const;
		BOOL32						IsLightCastShadow()const;
		const Vector2Int&			GetShadowMapSize()const;
		ECascadeShadowData*			GetCascadeShadowData();
		const ECascadeShadowData*	GetCascadeShadowData()const;
	public:
		void	SetLightColor(const Color3& InColor);
		void	SetLightIntensity(const FLOAT InIntensity);
		void	SetShadowMapSize(const BOOL32 InIsCastShadow,const Vector2Int& InShadowMapSize);
	protected:
		ELightData			LightData;
		ECascadeShadowData*	CascadeShadowData;

		// Render proxy functions START
	public:
		enum PLightUpdateState : UINT8
		{
			LIGHT_UPDATE_STATE_NONE		= 0,
			LIGHT_UPDATE_STATE_MATRIX	= (1 << 0),
			LIGHT_UPDATE_STATE_DATA		= (1 << 1),
			LIGHT_UPDATE_STATE_CASCADE	= (1 << 2)
		};
	public:
		RDirectionalLightSceneProxy*	SceneProxy;	// ONLY render thread could use this proxy pointer.
	public:
		UINT8							GetUpdateRenderState()const;
		RDirectionalLightSceneProxy*	CreateSceneProxy();
		virtual void					CreateRenderState()override;
		virtual void					DestroyRenderState()override;
		virtual void					SendUpdateRenderState()override;
	protected:
		void			MarkAsDirty(PLightUpdateState InState);
		virtual void	MarkRenderTransformAsDirty()override;
		virtual void	MarkRenderStateAsDirty()override;
		virtual void	CleanMarkRenderStateDirty()override;
	protected:
		UINT8							UpdateState;
		// Render proxy functions END

		CLASS_VIRTUAL_NOCOPY_BODY(PDirectionalLightComponent)
	};

};