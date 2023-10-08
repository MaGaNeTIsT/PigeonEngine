#include "LevelCharacter.h"
#include "../Component/Character/MovementComponent.h"
#include "../../../../../../EngineThirdParty/JoltPhysics/Headers/Shapes.h"
#include "../../../../../../EngineThirdParty/JoltPhysics/Headers/Character/Character.h"
#include "../../../../../../EngineThirdParty/JoltPhysics/Headers/PhysicsManager.h"


PigeonEngine::PCharacter::PCharacter()
{
}

PigeonEngine::PCharacter::~PCharacter()
{
}

void PigeonEngine::PCharacter::InitCharacter(const FCharacterSettings* InCharacterSettings)
{
	Character = new FCharacter(InCharacterSettings);
	MoveMentComponent = new PMovementComponent(this);
	this->AddComponent(MoveMentComponent);
	StandingShape = new FCapsuleShape(CharacterHeightStanding,CharacterRadiusStanding);
	CrouchingShape = new FCapsuleShape(CharacterHeightCrouching,CharacterRadiusCrouching);
	FPhysicsManager::GetSingleton()->AddCharacter(Character);
}

void PigeonEngine::PCharacter::UninitCharacter()
{
	FPhysicsManager::GetSingleton()->RemoveCharacter(Character);
	delete Character;
	Character = nullptr;
	delete StandingShape;
	StandingShape = nullptr;
	delete CrouchingShape;
	CrouchingShape = nullptr;
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

void PigeonEngine::PCharacter::BeginAddedToScene(PWorld* World)
{
	PPawn::BeginAddedToScene(World);
	Character->AddToPhysicsSystem(EActivate::Activate, GetActorLocation(), GetActorRotation(), 0);
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
}

void PigeonEngine::PCharacter::UserEndPlay()
{
	PPawn::UserEndPlay();
}
