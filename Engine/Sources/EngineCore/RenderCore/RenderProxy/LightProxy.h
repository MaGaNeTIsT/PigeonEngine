#pragma once

#include <CoreMinimal.h>
#include <EngineCommon.h>
#include <PigeonBase/Object/Light/Light.h>

namespace PigeonEngine
{
	class PDirectionalLightComponent;
	class RScene;
	class RViewProxy;

	class RDirectionalLightProxy : public EObjectBase
	{
	public:
		using TVisibilityMapType	= TMap<ObjectIdentityType, BOOL32>;
	public:
		RDirectionalLightProxy(const RScene* InScene, const PDirectionalLightComponent* InComponent, BOOL32 InIsCascade = FALSE);
	public:
		void	GenerateViewInfo(const RViewProxy* InViewProxy);
	protected:
		TVisibilityMapType					VisibilityMap;
	protected:
		ELightData							LightData;
		TArray<EViewDomainInfo>				ViewDomainInfos;
	protected:
		BOOL32								IsCascadeShadow;
		Quaternion							WorldRotation;
		const RScene*						Scene;
		const RViewProxy*					View;
		const PDirectionalLightComponent*	Component;
	public:
		RDirectionalLightProxy();
		RDirectionalLightProxy(const RDirectionalLightProxy& Other);
		virtual ~RDirectionalLightProxy();
		RDirectionalLightProxy& operator=(const RDirectionalLightProxy&) = delete;
	};

};