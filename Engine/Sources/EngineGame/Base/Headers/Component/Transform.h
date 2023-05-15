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

	enum class EMobilityTypes : UINT8
	{
		EMT_STATIC,
		EMT_DYNAMIC,
		EMT_STATION,
	};

	enum class ECoordinateSpaceTypes : UINT8
	{
		ECST_LOCAL,
		ECST_WORLD,
	};
};