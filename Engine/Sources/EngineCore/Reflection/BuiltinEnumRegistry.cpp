#include <CoreMinimal.h>
#include "../../../EngineThirdParty/JoltPhysics/Headers/JoltPhysicsLayer.h"
#include "../../../EngineThirdParty/JoltPhysics/Headers/PhysicsUtility.h"
#include <PigeonBase/Object/Component/Physics/TriggerComponent.h>
#include <PigeonBase/Object/Component/Navigation/NavObstacleComponent.h>
#include <PigeonBase/Object/Component/Water/WaterSimulationTypes.h>
#include <Animation/AnimationTypes.h>
#include "EnumRegistry.h"

namespace PigeonEngine
{
	/*
	* On-disk names for the built-in enums that component properties expose. The names are
	* the enum identifiers, so a file stays readable and inserting a value never shifts an
	* existing entry.
	*/
	static void RegisterBuiltInEnums()
	{
		RegisterEnumType<EMobilityType>(PE_TEXT("EMobilityType"), {
			{ static_cast<INT32>(EMobilityType::EMT_STATIC),  EString(PE_TEXT("EMT_STATIC")) },
			{ static_cast<INT32>(EMobilityType::EMT_DYNAMIC), EString(PE_TEXT("EMT_DYNAMIC")) },
			{ static_cast<INT32>(EMobilityType::EMT_STATION), EString(PE_TEXT("EMT_STATION")) },
		});

		RegisterEnumType<PhysicsUtility::EMotionType>(PE_TEXT("PhysicsUtility::EMotionType"), {
			{ static_cast<INT32>(PhysicsUtility::EMotionType::Static),    EString(PE_TEXT("Static")) },
			{ static_cast<INT32>(PhysicsUtility::EMotionType::Kinematic), EString(PE_TEXT("Kinematic")) },
			{ static_cast<INT32>(PhysicsUtility::EMotionType::Dynamic),   EString(PE_TEXT("Dynamic")) },
		});

		// FPhysicsObjectLayer is a wrapper class, so its values are registered under the
		// class hash and looked up through GetValue()/the constructor.
		RegisterEnumType<FPhysicsObjectLayer>(PE_TEXT("FPhysicsObjectLayer"), {
			{ 0, EString(PE_TEXT("NON_MOVING")) },
			{ 1, EString(PE_TEXT("MOVING")) },
			{ 2, EString(PE_TEXT("SENSOR")) },
			{ 3, EString(PE_TEXT("QUERY_ONLY")) },
		});

		RegisterEnumType<ETriggerBackendType>(PE_TEXT("ETriggerBackendType"), {
			{ static_cast<INT32>(ETriggerBackendType::QUERY),  EString(PE_TEXT("QUERY")) },
			{ static_cast<INT32>(ETriggerBackendType::SENSOR), EString(PE_TEXT("SENSOR")) },
		});

		RegisterEnumType<ENavigationObstacleShapeType>(PE_TEXT("ENavigationObstacleShapeType"), {
			{ static_cast<INT32>(NAVIGATION_OBSTACLE_CYLINDER), EString(PE_TEXT("CYLINDER")) },
			{ static_cast<INT32>(NAVIGATION_OBSTACLE_BOX),      EString(PE_TEXT("BOX")) },
		});

		RegisterEnumType<EWaterGroundType>(PE_TEXT("EWaterGroundType"), {
			{ static_cast<INT32>(EWaterGroundType::WATER_GROUND_FLAT),       EString(PE_TEXT("FLAT")) },
			{ static_cast<INT32>(EWaterGroundType::WATER_GROUND_HEIGHT_MAP), EString(PE_TEXT("HEIGHT_MAP")) },
		});

		RegisterEnumType<EAnimationLoopMode>(PE_TEXT("EAnimationLoopMode"), {
			{ static_cast<INT32>(ANIMATION_LOOP_ONCE),     EString(PE_TEXT("ONCE")) },
			{ static_cast<INT32>(ANIMATION_LOOP_REPEAT),   EString(PE_TEXT("REPEAT")) },
			{ static_cast<INT32>(ANIMATION_LOOP_PINGPONG), EString(PE_TEXT("PINGPONG")) },
			{ static_cast<INT32>(ANIMATION_LOOP_CLAMP),    EString(PE_TEXT("CLAMP")) },
		});
	}

	PE_REGISTER_CLASS_TYPE(&RegisterBuiltInEnums);
}
