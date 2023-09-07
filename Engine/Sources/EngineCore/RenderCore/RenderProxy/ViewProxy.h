#pragma once

#include <CoreMinimal.h>
#include <EngineCommon.h>
#include <RenderConfig/RenderConfig.h>
#include "BaseSceneProxy.h"
#include <PigeonBase/Object/Component/CameraAndLight/CameraComponent.h>

namespace PigeonEngine
{

	class PCameraComponent;
	class RScene;

	class RViewProxy : public RBaseSceneProxy
	{
	public:
		RViewProxy(const PCameraComponent* InComponent);
	public:
		const PCameraViewInfo&	GetViewInfo()const;
		const EViewport&		GetRenderViewport()const;
		const EViewMatrix&		GetViewMatrix()const;
		const EFrustum&			GetViewFrustum()const;
	public:
		TMap<ObjectIdentityType, BOOL32>	VisibilityMap;
	protected:
		PCameraViewInfo						CameraViewInfo;
		EViewDomainInfo						ViewDomainInfo;
	protected:
		const PCameraComponent*				Component;

		RENDER_PROXY_CLASS_BODY(RViewProxy)

	};

};