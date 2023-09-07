#pragma once

#include <CoreMinimal.h>
#include "MeshSceneProxy.h"

namespace PigeonEngine
{

	class PStaticMeshComponent;

	class RStaticMeshSceneProxy : public RMeshSceneProxy
	{
	public:
		RStaticMeshSceneProxy(PStaticMeshComponent* InComponent);
	protected:
		const PStaticMeshComponent* Component;

		RENDER_PROXY_CLASS_BODY(RStaticMeshSceneProxy)

	};

};