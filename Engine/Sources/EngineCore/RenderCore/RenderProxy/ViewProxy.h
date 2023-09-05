#pragma once

#include <CoreMinimal.h>
#include <EngineCommon.h>
#include <RenderConfig/RenderConfig.h>
#include <PigeonBase/Object/Component/CameraComponent.h>

namespace PigeonEngine
{

	class PCameraComponent;
	class RScene;

	class RViewProxy : public EObjectBase
	{
	public:
		RViewProxy(const RScene* InScene, const PCameraComponent* InComponent);
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
		const RScene*						Scene;
		const PCameraComponent*				Component;
	public:
		RViewProxy();
		RViewProxy(const RViewProxy& Other);
		virtual ~RViewProxy();
		RViewProxy& operator=(const RViewProxy&) = delete;
	};

};