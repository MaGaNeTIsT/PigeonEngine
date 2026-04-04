#include "LevelCharacter.h"
#include "../Component/Character/MovementComponent.h"
#include "../../../../../../EngineThirdParty/JoltPhysics/Headers/Shapes.h"
#include "../../../../../../EngineThirdParty/JoltPhysics/Headers/Character/Character.h"
#include "../../../../../../EngineThirdParty/JoltPhysics/Headers/PhysicsManager.h"

#include <PhysicsConfig/PhysicsConfig.h>



PigeonEngine::PCharacter::PCharacter()
{
}

PigeonEngine::PCharacter::~PCharacter()
{
	UninitCharacter();
}

void PigeonEngine::PCharacter::InitCharacter(FCharacterSettings& InCharacterSettings)
{
	StandingShape = New<FRotatedTranslatedShape>(Vector3(0.f, 0.5f * CharacterHeightStanding + CharacterRadiusStanding, 0.f), Quaternion::Identity(), New<FCapsuleShape>(0.5f * CharacterHeightStanding,CharacterRadiusStanding));
	CrouchingShape = New<FRotatedTranslatedShape>(Vector3(0.f, 0.5f * CharacterHeightCrouching + CharacterRadiusCrouching, 0.f), Quaternion::Identity(), New<FCapsuleShape>(0.5f * CharacterHeightCrouching, CharacterRadiusCrouching));
	InCharacterSettings.Shape = StandingShape;
	Character = New<FCharacter>(InCharacterSettings);
	MovementComponent = New<PMovementComponent>(this);
	this->AddComponent(MovementComponent);
	FPhysicsManager::GetSingleton()->AddCharacter(Character);
}

void PigeonEngine::PCharacter::UninitCharacter()
{
	if (Character)
	{
		FPhysicsManager::GetSingleton()->RemoveCharacter(Character);
		Delete(Character);
		Character = nullptr;
	}
	if (StandingShape)
	{
		Delete(StandingShape);
		StandingShape = nullptr;
	}
	if (CrouchingShape)
	{
		Delete(CrouchingShape);
		CrouchingShape = nullptr;
	}
	if (MovementComponent)
	{
		Delete(MovementComponent);
		MovementComponent = nullptr;
	}
}

PigeonEngine::FShape* PigeonEngine::PCharacter::GetStandingShape()
{
	return StandingShape;
}

PigeonEngine::FShape* PigeonEngine::PCharacter::GetCrouchingShape()
{
	return CrouchingShape;
}

PigeonEngine::FCharacter* PigeonEngine::PCharacter::GetPhysicsCharacter()
{
	return Character;
}
PigeonEngine::PMovementComponent* PigeonEngine::PCharacter::GetMovementComponent()
{
	return MovementComponent;
}
#if _EDITOR_ONLY
void PigeonEngine::PCharacter::EditorTick(FLOAT deltaTime)
{
	if (Character->GetShape() && FPhysicsCommonSettings::GetSingleton()->PHYSICS_DRAW_DEBUG_PRIMITIVE)
	{
		RDebugWireframePrimitiveManager* Manager = RDebugWireframePrimitiveManager::GetManagerSingleton();
		const Vector3 Pos = GetActorLocation();
		const Quaternion Rot = GetActorRotation();
		Character->GetShape()->DrawPrimitive(Manager, Pos, Rot);
	}
}
#endif

void PigeonEngine::PCharacter::BeginAddedToScene(PWorld* World)
{
	PPawn::BeginAddedToScene(World);
	Character->AddToPhysicsSystem(EActivate::Activate, GetActorLocation(), GetActorRotation(), 0);
	Character->Activate();
}

void PigeonEngine::PCharacter::RemovedFromScene()
{
	PPawn::RemovedFromScene();
	Character->RemoveFromPhysicsSystem();
}

void PigeonEngine::PCharacter::UserBeginPlay()
{
	PPawn::UserBeginPlay();
}

void PigeonEngine::PCharacter::UserTick(FLOAT deltaTime)
{
	PPawn::UserTick(deltaTime);
	//MoveMentComponent->HandleInput(Vector3(0.f, 0.f, 0.f), FALSE , FALSE);
}

void PigeonEngine::PCharacter::UserEndPlay()
{
	PPawn::UserEndPlay();
}
