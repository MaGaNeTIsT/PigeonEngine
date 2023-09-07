#pragma once

#include <CoreMinimal.h>
#include "PrimitiveSceneProxy.h"

namespace PigeonEngine
{

	class RMeshSceneProxy : public RPrimitiveSceneProxy
	{
	public:
		RMeshSceneProxy();
	protected:

		RENDER_PROXY_CLASS_BODY(RMeshSceneProxy)

	};

};