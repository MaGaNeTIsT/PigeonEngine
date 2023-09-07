#pragma once

#include <CoreMinimal.h>
#include "MeshSceneProxy.h"

namespace PigeonEngine
{

	class PSkeletalMeshComponent;

	class RSkeletalMeshSceneProxy : public RMeshSceneProxy
	{
	public:
		RSkeletalMeshSceneProxy(PSkeletalMeshComponent* InComponent);
	protected:
		const PSkeletalMeshComponent* Component;

		RENDER_PROXY_CLASS_BODY(RSkeletalMeshSceneProxy)

	};

};