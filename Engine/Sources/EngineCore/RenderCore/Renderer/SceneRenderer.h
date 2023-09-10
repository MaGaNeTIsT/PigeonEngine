#pragma once

#include <CoreMinimal.h>
#include "RenderScene.h"

namespace PigeonEngine
{

	class RViewProxy;

	class RSceneTextures
	{
	public:

	};

	class RSceneRenderer : public EManagerBase
	{
	public:
		virtual void	Initialize()override;
		virtual void	ShutDown()override;
	public:
		void			InitNewFrame();
		void			Render();
	protected:
		void			InitViews();
		void			OctreeCull();
		void			PrimitiveCull();
	protected:
		RScene*					Scene;
		TArray<RViewProxy*>		ViewProxies;

		CLASS_MANAGER_SINGLETON_BODY(RSceneRenderer)

	};

};