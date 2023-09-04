#pragma once

#include <CoreMinimal.h>

namespace PigeonEngine
{

	class RScene;
	class RViewProxy;

	class RSceneTextures
	{
	public:

	};

	class RSceneRenderer : public EManagerBase
	{
	public:
		virtual void Initialize()override;
		virtual void ShutDown()override;
	protected:
		RScene*				Scene;
		TArray<RViewProxy*>	ViewProxies;

		CLASS_MANAGER_SINGLETON_BODY(RSceneRenderer)

	};

};