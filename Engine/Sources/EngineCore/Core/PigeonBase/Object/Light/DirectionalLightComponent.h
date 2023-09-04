#pragma once

#include <CoreMinimal.h>
#include <EngineCommon.h>
#include <PigeonBase/Object/Light/Light.h>
#include <PigeonBase/Object/Component/SceneComponent.h>

namespace PigeonEngine
{

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

		CLASS_VIRTUAL_NOCOPY_BODY(PDirectionalLightComponent)
	};

};