#pragma once

#include <CoreMinimal.h>
#include "PrimitiveSceneProxy.h"

namespace PigeonEngine
{

	class PMeshComponent;

	class RMeshSceneProxy : public RPrimitiveSceneProxy
	{
	protected:
		void	SetupSceneProxy(PMeshComponent* InComponent, const BOOL32 InIsMovable, const BOOL32 InIsCastShadow, const BOOL32 InIsReceiveShadow);

		RENDER_PROXY_CLASS_BODY(RMeshSceneProxy)

	};

};