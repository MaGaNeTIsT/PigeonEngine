#pragma once

#include "../../../EngineCore/Core/Headers/Main.h"
#include "../../../EngineCore/Base/Headers/ObjectBase.h"

namespace PigeonEngine
{

	class PObject : public EObjectBase
	{

		EClass(PObject, EObjectBase)

	public:
		virtual void	Init() = 0;
		virtual void	Uninit() = 0;
#ifdef _DEVELOPMENT_EDITOR
	public:
		void	SetName(const std::string& name);
#endif

		CLASS_VIRTUAL_NOCOPY_BODY(PObject)

	};

};