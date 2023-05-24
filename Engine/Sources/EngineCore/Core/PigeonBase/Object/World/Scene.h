#pragma once

#include <CoreMinimal.h>
#include "PigeonBase/Object/Object.h"


namespace PigeonEngine
{
	class PWorld;
	class PSceneNode : public PObject
	{
		EClass(PSceneNode, PObject)
	public:
		CLASS_VIRTUAL_NOCOPY_BODY(PSceneNode)
		
	};
	
	class PScene : public PObject
	{
		EClass(PScene, PObject)
	public:
		CLASS_VIRTUAL_NOCOPY_BODY(PScene)

		
	private:
		PWorld* World = nullptr;
		
	};

};