#pragma once

#include <CoreMinimal.h>
#include "MeshSceneProxy.h"

namespace PigeonEngine
{

	class PSkeletalMeshComponent;

	class RSkeletonMeshSceneProxy : public RMeshSceneProxy
	{
	public:
		RSkeletonMeshSceneProxy(PSkeletalMeshComponent* InComponent);
	};

};