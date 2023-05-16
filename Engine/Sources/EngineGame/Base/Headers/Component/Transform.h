#pragma once

#include <Main.h>
#include "../../../../EngineCore/Base/Headers/BaseType.h"
#include "../../../../EngineCore/Base/Headers/ObjectBase.h"
#include "./ActorComponent.h"

namespace PigeonEngine
{

	enum class ECoordinateSpaceType : UINT8
	{
		ECST_LOCAL,
		ECST_WORLD,
	};

	class ETransform : public EObjectBase
	{

		EClass(ETransform, EObjectBase)

	public:
		template<ECoordinateSpaceType _CoordinateSpaceType>
		Matrix4x4	ToMatrix4x4(const PActorComponent* InParentComponent = nullptr)const;

		template<ECoordinateSpaceType _CoordinateSpaceType>
		Vector3		GetLocation(const PActorComponent* InParentComponent = nullptr)const;

		template<ECoordinateSpaceType _CoordinateSpaceType>
		Quaternion	GetRotation(const PActorComponent* InParentComponent = nullptr)const;

		template<ECoordinateSpaceType _CoordinateSpaceType>
		Vector3		GetScaling(const PActorComponent* InParentComponent = nullptr)const;
	protected:
		Vector3		LocalPosition;
		Quaternion	LocalRotation;
		Vector3		LocalScaling;

		CLASS_VIRTUAL_COPY_BODY(ETransform)

	};

	enum class EMobilityType : UINT8
	{
		EMT_STATIC,
		EMT_DYNAMIC,
		EMT_STATION,
	};

};