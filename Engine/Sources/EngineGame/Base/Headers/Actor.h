#pragma once

#include "../../../EngineCore/Core/Headers/Main.h"
#include "../../../EngineCore/Core/Headers/ErrorCaption.h"
#include "../../../EngineCore/Base/Headers/ObjectBase.h"
#include "../../../EngineCore/Base/Headers/RTTI.h"

namespace PigeonEngine
{

	class PActor :public EObjectBase
	{
		EClass(PActor, EObjectBase)

	public:
		PActor();
		PActor(const PActor&) = delete;
		~PActor();
	public:
		PActor& operator=(const PActor&) = delete;
	};

};