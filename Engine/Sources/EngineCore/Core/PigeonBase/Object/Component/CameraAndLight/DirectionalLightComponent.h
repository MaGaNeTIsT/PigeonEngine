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
		ELightType			GetLightType()const;
		const Color3&		GetLightColor()const;
		FLOAT				GetLightIntensity()const;
		BOOL32				IsLightCastShadow()const;
		const Vector2Int&	GetShadowMapSize()const;
	public:
		void	SetLightColor(const Color3& InColor);
		void	SetLightIntensity(FLOAT InIntensity);
		void	SetLightCastShadow(BOOL32 InIsCastShadow);
		void	SetShadowMapSize(const Vector2Int& InShadowMapSize);
	protected:
		ELightData		LightData;

		// Render proxy functions START
	public:
		enum PLightUpdateState : UINT8
		{
			LIGHT_UPDATE_STATE_NONE		= 0,
			LIGHT_UPDATE_STATE_MATRIX	= (1 << 0),
			LIGHT_UPDATE_STATE_DATA		= (1 << 1)
		};
	public:
		UINT8	GetUpdateRenderState()const;
		void	MarkAsDirty(PLightUpdateState InState);
	protected:
		RDirectionalLightSceneProxy*	SceneProxy;
		UINT8							UpdateState;
	public:
		RDirectionalLightSceneProxy*		GetSceneProxy();
		const RDirectionalLightSceneProxy*	GetSceneProxy()const;
		RDirectionalLightSceneProxy*		CreateSceneProxy();
		virtual void	MarkRenderStateAsDirty()override;
		virtual void	CreateRenderState()override;
		virtual void	DestroyRenderState()override;
		virtual void	SendUpdateRenderState()override;
	protected:
		virtual void	CleanMarkRenderStateDirty()override;
		// Render proxy functions END

		CLASS_VIRTUAL_NOCOPY_BODY(PDirectionalLightComponent)
	};

};