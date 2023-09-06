#pragma once

#include <CoreMinimal.h>
#include "PrimitiveSceneProxy.h"

namespace PigeonEngine
{

	class PMeshComponent;

	class RMeshSceneProxy : public RPrimitiveSceneProxy
	{
	public:
		RMeshSceneProxy(PMeshComponent* InComponent);
	protected:
		const PMeshComponent*	Component;

		RENDER_PROXY_CLASS_BODY(RMeshSceneProxy)

	};

};