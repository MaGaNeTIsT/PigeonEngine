#pragma once

#include "../../../EngineCore/Core/Headers/Main.h"
#include "./Object.h"

namespace PigeonEngine
{

	class PActor : public PObject
	{

		EClass(PActor, PObject)

	public:
		virtual void	Update() = 0;
		virtual void	FixedUpdate() = 0;
#ifdef _DEVELOPMENT_EDITOR
		virtual void	EditorUpdate() = 0;
#endif

		CLASS_VIRTUAL_NOCOPY_BODY(PActor)

	};

};