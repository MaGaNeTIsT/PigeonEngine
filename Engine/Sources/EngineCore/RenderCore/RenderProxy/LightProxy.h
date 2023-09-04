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
		RDirectionalLightProxy(const RScene* InScene, const PDirectionalLightComponent* InComponent);
	public:
		void	GenerateViewInfo(const RViewProxy* InViewProxy, BOOL InIsCascade = FALSE);
	protected:
		TVisibilityMapType					VisibilityMap;
	protected:
		ELightData							LightData;
		EViewport							RenderViewport;
		EViewMatrix							ViewMatrix;
		EFrustum							ViewFrustum;
	protected:
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