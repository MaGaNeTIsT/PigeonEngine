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
		using RVisibilityMapType = TMap<ObjectIdentityType, BOOL32>;
	public:
		RViewProxy(const PCameraComponent* InComponent);
	public:
		RVisibilityMapType&		GetVisibilityMap();
		const RVisibilityMapType&	GetVisibilityMap()const;
		PCameraViewInfo&			GetViewInfo();
		const PCameraViewInfo&		GetViewInfo()const;
		EViewport&					GetRenderViewport();
		const EViewport&			GetRenderViewport()const;
		EViewMatrix&				GetViewMatrix();
		const EViewMatrix&			GetViewMatrix()const;
		EFrustum&					GetViewFrustum();
		const EFrustum&				GetViewFrustum()const;
	protected:
		RVisibilityMapType			VisibilityMap;
	protected:
		PCameraViewInfo				CameraViewInfo;
		EViewDomainInfo				ViewDomainInfo;
	protected:
		const PCameraComponent*		Component;

		RENDER_PROXY_CLASS_BODY(RViewProxy)

	};

};