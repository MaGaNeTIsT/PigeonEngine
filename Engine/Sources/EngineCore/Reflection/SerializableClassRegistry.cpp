#include <PigeonBase/Object/Object.h>
#include <PigeonBase/Object/Actor.h>
#include <PigeonBase/Object/Actor/LevelActor.h>
#include <PigeonBase/Object/Actor/Pawn.h>
#include <PigeonBase/Object/Actor/LevelCharacter.h>
#include <PigeonBase/Object/Actor/NavMeshActor.h>
#include <PigeonBase/Object/Controller/Controller.h>
#include <PigeonBase/Object/Controller/CharacterController.h>
#include <PigeonBase/Object/Component/Component.h>
#include <PigeonBase/Object/Component/ActorComponent.h>
#include <PigeonBase/Object/Component/SceneComponent.h>
#include <PigeonBase/Object/Component/Primitive/PrimitiveComponent.h>
#include <PigeonBase/Object/Component/Primitive/MeshComponent.h>
#include <PigeonBase/Object/Component/Primitive/StaticMeshComponent.h>
#include <PigeonBase/Object/Component/Primitive/SkeletalMeshComponent.h>
#include <PigeonBase/Object/Component/Primitive/LineComponent.h>
#include <PigeonBase/Object/Component/Primitive/BezierGrassComponent.h>
#include <PigeonBase/Object/Component/Primitive/SkyLightComponent.h>
#include <PigeonBase/Object/Component/CameraAndLight/CameraComponent.h>
#include <PigeonBase/Object/Component/CameraAndLight/LightComponent.h>
#include <PigeonBase/Object/Component/CameraAndLight/PointLightComponent.h>
#include <PigeonBase/Object/Component/CameraAndLight/SpotLightComponent.h>
#include <PigeonBase/Object/Component/CameraAndLight/DirectionalLightComponent.h>
#include <PigeonBase/Object/Component/Physics/PhysicsComponent.h>
#include <PigeonBase/Object/Component/Physics/TriggerComponent.h>
#include <PigeonBase/Object/Component/Character/MovementComponent.h>
#include <PigeonBase/Object/Component/Character/CharacterInputComponent.h>
#include <PigeonBase/Object/Component/Navigation/NavMeshBoundsComponent.h>
#include <PigeonBase/Object/Component/Navigation/NavObstacleComponent.h>
#include <PigeonBase/Object/Component/Navigation/NavAgentComponent.h>
#include <PigeonBase/Object/Component/Water/WaterSimulationComponent.h>
#include <PigeonBase/Object/Component/Water/WaterSurfaceComponent.h>
#include <Demo/DemoObstacleMotionComponent.h>

namespace PigeonEngine
{

	/*
	* Stable class names and factories for the object/component/actor hierarchy, in one
	* auditable table: these strings are the on-disk class identity of every level/asset
	* file. Renaming one is a save-format change; adding a class is one line here plus
	* its RegisterClassType call.
	*
	* Abstract bases are named but get no factory. A level file only instantiates the
	* concrete classes listed in the second half.
	*/
	static void RegisterSerializableClasses()
	{
		// Hierarchy: named so files and error messages use readable names. No factories.
		RegisterSerializableClassType<PObject, EObjectBase>(PE_TEXT("PObject"));
		RegisterSerializableClassType<PComponent, PObject>(PE_TEXT("PComponent"));
		RegisterSerializableClassType<PActor, PObject>(PE_TEXT("PActor"));
		RegisterSerializableClassType<PActorComponent, PComponent>(PE_TEXT("PActorComponent"));
		RegisterSerializableClassType<PSceneComponent, PActorComponent>(PE_TEXT("PSceneComponent"));
		RegisterSerializableClassType<PPrimitiveComponent, PSceneComponent>(PE_TEXT("PPrimitiveComponent"));
		RegisterSerializableClassType<PMeshComponent, PPrimitiveComponent>(PE_TEXT("PMeshComponent"));
		RegisterSerializableClassType<PLightComponent, PSceneComponent>(PE_TEXT("PLightComponent"));
		RegisterSerializableClassType<PLocalLightComponent, PLightComponent>(PE_TEXT("PLocalLightComponent"));

		// Actors.
		SaveDefaultClassFactory<PLevelActor>();
		RegisterSerializableClassType<PLevelActor, PActor>(PE_TEXT("PLevelActor"));
		SaveDefaultClassFactory<PPawn>();
		RegisterSerializableClassType<PPawn, PActor>(PE_TEXT("PPawn"));
		SaveDefaultClassFactory<PCharacter>();
		RegisterSerializableClassType<PCharacter, PPawn>(PE_TEXT("PCharacter"));
		SaveDefaultClassFactory<PNavMeshActor>();
		RegisterSerializableClassType<PNavMeshActor, PActor>(PE_TEXT("PNavMeshActor"));
		SaveDefaultClassFactory<PController>();
		RegisterSerializableClassType<PController, PActor>(PE_TEXT("PController"));
		SaveDefaultClassFactory<PCharacterController>();
		RegisterSerializableClassType<PCharacterController, PController>(PE_TEXT("PCharacterController"));

		// Components.
		SaveDefaultClassFactory<PSceneComponent>();
		SaveDefaultClassFactory<PPrimitiveComponent>();
		SaveDefaultClassFactory<PMeshComponent>();
		SaveDefaultClassFactory<PStaticMeshComponent>();
		RegisterSerializableClassType<PStaticMeshComponent, PMeshComponent>(PE_TEXT("PStaticMeshComponent"));
		SaveDefaultClassFactory<PSkeletalMeshComponent>();
		RegisterSerializableClassType<PSkeletalMeshComponent, PMeshComponent>(PE_TEXT("PSkeletalMeshComponent"));
		SaveDefaultClassFactory<PLineComponent>();
		RegisterSerializableClassType<PLineComponent, PPrimitiveComponent>(PE_TEXT("PLineComponent"));
		SaveDefaultClassFactory<PBezierGrassComponent>();
		RegisterSerializableClassType<PBezierGrassComponent, PPrimitiveComponent>(PE_TEXT("PBezierGrassComponent"));
		SaveDefaultClassFactory<PCameraComponent>();
		RegisterSerializableClassType<PCameraComponent, PSceneComponent>(PE_TEXT("PCameraComponent"));
		SaveDefaultClassFactory<PPointLightComponent>();
		RegisterSerializableClassType<PPointLightComponent, PLocalLightComponent>(PE_TEXT("PPointLightComponent"));
		SaveDefaultClassFactory<PSpotLightComponent>();
		RegisterSerializableClassType<PSpotLightComponent, PLocalLightComponent>(PE_TEXT("PSpotLightComponent"));
		SaveDefaultClassFactory<PDirectionalLightComponent>();
		RegisterSerializableClassType<PDirectionalLightComponent, PLightComponent>(PE_TEXT("PDirectionalLightComponent"));
		SaveDefaultClassFactory<PSkyLightComponent>();
		RegisterSerializableClassType<PSkyLightComponent, PSceneComponent>(PE_TEXT("PSkyLightComponent"));
		SaveDefaultClassFactory<PTriggerComponent>();
		RegisterSerializableClassType<PTriggerComponent, PSceneComponent>(PE_TEXT("PTriggerComponent"));
		SaveDefaultClassFactory<PPhysicsComponent>();
		RegisterSerializableClassType<PPhysicsComponent, PActorComponent>(PE_TEXT("PPhysicsComponent"));
		SaveDefaultClassFactory<PMovementComponent>();
		RegisterSerializableClassType<PMovementComponent, PActorComponent>(PE_TEXT("PMovementComponent"));
		SaveDefaultClassFactory<PCharacterInputComponent>();
		RegisterSerializableClassType<PCharacterInputComponent, PActorComponent>(PE_TEXT("PCharacterInputComponent"));
		SaveDefaultClassFactory<PNavMeshBoundsComponent>();
		RegisterSerializableClassType<PNavMeshBoundsComponent, PSceneComponent>(PE_TEXT("PNavMeshBoundsComponent"));
		SaveDefaultClassFactory<PNavObstacleComponent>();
		RegisterSerializableClassType<PNavObstacleComponent, PSceneComponent>(PE_TEXT("PNavObstacleComponent"));
		SaveDefaultClassFactory<PNavAgentComponent>();
		RegisterSerializableClassType<PNavAgentComponent, PActorComponent>(PE_TEXT("PNavAgentComponent"));
		SaveDefaultClassFactory<PWaterSimulationComponent>();
		RegisterSerializableClassType<PWaterSimulationComponent, PPrimitiveComponent>(PE_TEXT("PWaterSimulationComponent"));
		SaveDefaultClassFactory<PWaterSurfaceComponent>();
		RegisterSerializableClassType<PWaterSurfaceComponent, PStaticMeshComponent>(PE_TEXT("PWaterSurfaceComponent"));
		// Demo-only behaviour component, stored in the default demo level so the obstacle
		// motion survives a data-driven load.
		SaveDefaultClassFactory<PDemoObstacleMotionComponent>();
		RegisterSerializableClassType<PDemoObstacleMotionComponent, PActorComponent>(PE_TEXT("PDemoObstacleMotionComponent"));
	}

	PE_REGISTER_CLASS_TYPE(&RegisterSerializableClasses);

}
