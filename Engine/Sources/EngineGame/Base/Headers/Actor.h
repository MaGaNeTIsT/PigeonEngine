#pragma once

#include "../../../EngineCore/Core/Headers/Main.h"
#include "./Object.h"

namespace PigeonEngine
{

	class PActor : public PObject
	{

		EClass(PActor, PObject)

	public:
		virtual void	FixedTick(FLOAT deltaTime) = 0;

		CLASS_VIRTUAL_NOCOPY_BODY(PActor)

	};

};