#pragma once

#include <CoreMinimal.h>

namespace PigeonEngine
{

	class RSceneRenderer : public EManagerBase
	{

		EClass(RSceneRenderer, EManagerBase)

	public:
		virtual void Initialize()override;
		virtual void ShutDown()override;
	protected:

		CLASS_MANAGER_SINGLETON_BODY(RSceneRenderer)

	};

};