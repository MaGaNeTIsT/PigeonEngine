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
		RViewProxy(const PCameraComponent* InComponent);
	public:
		const PCameraViewInfo&	GetViewInfo()const;
		const EViewport&		GetRenderViewport()const;
		const EViewMatrix&		GetViewMatrix()const;
		const EFrustum&			GetViewFrustum()const;
	public:
		const RScene*			GetRenderScene()const;
		PE_INLINE void			SetRenderScene(const RScene* InScene);
	public:
		TMap<ObjectIdentityType, BOOL32>	VisibilityMap;
	protected:
		PCameraViewInfo						CameraViewInfo;
		EViewDomainInfo						ViewDomainInfo;
	protected:
		const RScene*						Scene;
		const PCameraComponent*				Component;

		RENDER_PROXY_CLASS_BODY(RViewProxy)

	};

};