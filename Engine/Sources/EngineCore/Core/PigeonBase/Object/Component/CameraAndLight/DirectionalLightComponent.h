#pragma once

#include <CoreMinimal.h>
#include <EngineCommon.h>
#include <PigeonBase/Object/Component/SceneComponent.h>

namespace PigeonEngine
{

	class RDirectionalLightSceneProxy;

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
	protected:
		RDirectionalLightSceneProxy* SceneProxy;
	public:
		RDirectionalLightSceneProxy*		GetSceneProxy();
		const RDirectionalLightSceneProxy*	GetSceneProxy()const;
		RDirectionalLightSceneProxy*		CreateSceneProxy();
		virtual void	CreateRenderState()override;
		virtual void	DestroyRenderState()override;
		virtual void	SendUpdateRenderState()override;
		// Render proxy functions END

		CLASS_VIRTUAL_NOCOPY_BODY(PDirectionalLightComponent)
	};

};