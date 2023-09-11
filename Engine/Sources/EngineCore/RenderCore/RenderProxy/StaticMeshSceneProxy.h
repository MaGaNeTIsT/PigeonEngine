#pragma once

#include <CoreMinimal.h>
#include "MeshSceneProxy.h"

namespace PigeonEngine
{

	class PStaticMeshComponent;

	class RStaticMeshSceneProxy : public RMeshSceneProxy
	{
	public:
		RStaticMeshSceneProxy(PStaticMeshComponent* InComponent, const BOOL32 InIsMovable, const BOOL32 InIsCastShadow, const BOOL32 InIsReceiveShadow);
	public:
		void	SetupSceneProxy(PStaticMeshComponent* InComponent, const BOOL32 InIsMovable, const BOOL32 InIsCastShadow, const BOOL32 InIsReceiveShadow);
	protected:
		const PStaticMeshComponent*	Component;

		RENDER_PROXY_CLASS_BODY(RStaticMeshSceneProxy)

	};

};