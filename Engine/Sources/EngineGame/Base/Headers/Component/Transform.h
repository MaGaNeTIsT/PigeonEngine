#pragma once

#include "../../../../EngineCore/Core/Headers/Main.h"
#include "../../../../EngineCore/Base/Headers/BaseType.h"
#include "../../../../EngineCore/Base/Headers/ObjectBase.h"

namespace PigeonEngine
{

	class ETransform : public EObjectBase
	{

		EClass(ETransform, EObjectBase)

	public:

	protected:
		Vector3 LocalPosition;
		Vector3 LocalRotation;
		Vector3 LocalScale;

		CLASS_VIRTUAL_COPY_BODY(ETransform)

	};

};