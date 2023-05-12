#pragma once

#include "../../../EngineCore/Core/Headers/Main.h"
#include "./Object.h"

namespace PigeonEngine
{

	class PComponent : public PObject
	{

		EClass(PComponent, PObject)

	public:
		virtual void	Init()override;
		virtual void	Uninit()override;
		virtual void	Tick(FLOAT deltaTime)override;
#ifdef _DEVELOPMENT_EDITOR
		virtual void	EditorTick(FLOAT deltaTime)override;
#endif

		CLASS_VIRTUAL_COPY_BODY(PComponent)

	};

};