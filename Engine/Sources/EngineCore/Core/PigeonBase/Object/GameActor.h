#pragma once

#include <CoreMinimal.h>
#include "./Actor.h"

namespace PigeonEngine
{

	class PGameActor : public PActor
	{

		EClass(PGameActor, PActor)

	public:


		CLASS_VIRTUAL_NOCOPY_BODY(PGameActor)

	};

};