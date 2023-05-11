#pragma once

#include "../../../EngineCore/Core/Headers/Main.h"
#include "./Actor.h"

namespace PigeonEngine
{

	class PScene : public PActor
	{

		EClass(PScene, PActor)

	public:

		CLASS_VIRTUAL_NOCOPY_BODY(PScene)

	};

};