#pragma once

#include <CoreMinimal.h>
#include <EngineCommon.h>
#include <RenderConfig/RenderConfig.h>
#include <PigeonBase/Object/Component/CameraComponent.h>

namespace PigeonEngine
{
	class RScene;
	class PCameraComponent;

	class RViewProxy : public EObjectBase
	{
	public:
		using TPrimitiveObjectID = ULONGLONG;
	public:
		RViewProxy(const RScene* InScene, const PCameraComponent* InComponent);
	protected:
		TMap<TPrimitiveObjectID, BOOL32>	VisibilityMap;
	protected:
		PCameraViewInfo					CameraViewInfo;
		EViewport						RenderViewport;
		ECameraMatrix					ViewMatrix;
		EFrustum						ViewFrustum;
	protected:
		const RScene*					Scene;
		const PCameraComponent*			Component;
	public:
		RViewProxy();
		RViewProxy(const RViewProxy& Other);
		virtual ~RViewProxy();
		RViewProxy& operator=(const RViewProxy&) = delete;
	};

};