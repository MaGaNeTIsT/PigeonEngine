#pragma once

#include <CoreMinimal.h>
#include <EngineCommon.h>
#include "BaseSceneProxy.h"

namespace PigeonEngine
{

	class PDirectionalLightComponent;
	class RScene;
	class RViewProxy;

	class RDirectionalLightSceneProxy : public RBaseSceneProxy
	{
	public:
		using TVisibilityMapType	= TMap<ObjectIdentityType, BOOL32>;
	public:
		RDirectionalLightSceneProxy(const PDirectionalLightComponent* InComponent, BOOL32 InIsCascade = FALSE);
	public:
		void	GenerateViewInfo(const RViewProxy* InViewProxy);
	public:
		const TVisibilityMapType&	GetVisibilityMap()const;
		TVisibilityMapType&			GetVisibilityMap();
		const ELightData&			GetLightData()const;
		const EViewDomainInfo*		GetViewDomainInfo(UINT32 InIndex)const;
		const ECascadeShadowData*	GetCascadeShadowData()const;
		BOOL32						IsLightUseCascadeShadow()const;
		const Quaternion&			GetLightWorldRotation()const;
	protected:
		TVisibilityMapType					VisibilityMap;
	protected:
		ELightData							LightData;
		TArray<EViewDomainInfo>				ViewDomainInfos;
		ECascadeShadowData*					CascadeShadowData;
	protected:
		BOOL32								IsCascadeShadow;
		Quaternion							WorldRotation;
		const RViewProxy*					View;
		const PDirectionalLightComponent*	Component;

		RENDER_PROXY_CLASS_BODY(RDirectionalLightSceneProxy)

	};

};