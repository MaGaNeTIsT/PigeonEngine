#include "LevelCharacter.h"
#include "../Component/Character/MovementComponent.h"
#include "../../../../../../EngineThirdParty/JoltPhysics/Headers/Shapes.h"
#include "../../../../../../EngineThirdParty/JoltPhysics/Headers/Character/Character.h"
#include "../../../../../../EngineThirdParty/JoltPhysics/Headers/PhysicsManager.h"


PigeonEngine::PLevelCharacter::PLevelCharacter()
{
}

PigeonEngine::PLevelCharacter::~PLevelCharacter()
{
}

void PigeonEngine::PLevelCharacter::InitCharacter(const FCharacterSettings* InCharacterSettings)
{
	Character = new FCharacter(InCharacterSettings);
	MoveMentComponent = new PMovementComponent(this);
	this->AddComponent(MoveMentComponent);
	StandingShape = new FCapsuleShape(CharacterHeightStanding,CharacterRadiusStanding);
	CrouchingShape = new FCapsuleShape(CharacterHeightCrouching,CharacterRadiusCrouching);
	FPhysicsManager::GetSingleton()->AddCharacter(Character);
}

void PigeonEngine::PLevelCharacter::UninitCharacter()
{
	FPhysicsManager::GetSingleton()->RemoveCharacter(Character);
	delete Character;
	Character = nullptr;
	delete StandingShape;
	StandingShape = nullptr;
	delete CrouchingShape;
	CrouchingShape = nullptr;
}

PigeonEngine::FShape* PigeonEngine::PLevelCharacter::GetStandingShape()
{
	return StandingShape;
}

PigeonEngine::FShape* PigeonEngine::PLevelCharacter::GetCrouchingShape()
{
	return CrouchingShape;
}

PigeonEngine::FCharacter* PigeonEngine::PLevelCharacter::GetPhysicsCharacter()
{
	return Character;
}

void PigeonEngine::PLevelCharacter::BeginAddedToScene(PWorld* World)
{
	PLevelActor::BeginAddedToScene(World);
	Character->AddToPhysicsSystem(EActivate::Activate, GetActorLocation(), GetActorRotation(), 0);
}

void PigeonEngine::PLevelCharacter::RemovedFromScene()
{
	PLevelActor::RemovedFromScene();
	Character->RemoveFromPhysicsSystem();
}

void PigeonEngine::PLevelCharacter::UserBeginPlay()
{
	PLevelActor::UserBeginPlay();
}

void PigeonEngine::PLevelCharacter::UserTick(FLOAT deltaTime)
{
	PLevelActor::UserTick(deltaTime);
}

void PigeonEngine::PLevelCharacter::UserEndPlay()
{
	PLevelActor::UserEndPlay();
}
