#pragma once

#include "../../../EngineCore/Core/Headers/Main.h"
#include "./Component.h"

namespace PigeonEngine
{

	class PActorComponent : public PComponent
	{

		EClass(PActorComponent, PComponent)

	public:


		CLASS_VIRTUAL_COPY_BODY(PActorComponent)

	};

};