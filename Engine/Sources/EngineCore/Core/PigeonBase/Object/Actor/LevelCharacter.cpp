#include "LevelCharacter.h"
#include "../Component/Character/MovementComponent.h"
#include "../../../../../../EngineThirdParty/JoltPhysics/Headers/Shapes.h"
#include "../../../../../../EngineThirdParty/JoltPhysics/Headers/Character/CharacterVirtual.h"
#include "../../../../../../EngineThirdParty/JoltPhysics/Headers/PhysicsManager.h"

#include <PhysicsConfig/PhysicsConfig.h>
#include <PigeonBase/Object/World/World.h>

namespace PigeonEngine
{
	static void RegisterClassTypes()
	{
		RegisterClassType<PCharacter, PPawn>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);
}

PigeonEngine::PCharacter::PCharacter()
{
}

PigeonEngine::PCharacter::~PCharacter()
{
	UninitCharacter();
}

void PigeonEngine::PCharacter::InitCharacter(FCharacterVirtualSettings& InCharacterSettings)
{
	StandingShape = New<FRotatedTranslatedShape>(Vector3(0.f, 0.5f * CharacterHeightStanding + CharacterRadiusStanding, 0.f), Quaternion::Identity(), New<FCapsuleShape>(0.5f * CharacterHeightStanding, CharacterRadiusStanding));
	CrouchingShape = New<FRotatedTranslatedShape>(Vector3(0.f, 0.5f * CharacterHeightCrouching + CharacterRadiusCrouching, 0.f), Quaternion::Identity(), New<FCapsuleShape>(0.5f * CharacterHeightCrouching, CharacterRadiusCrouching));
	InCharacterSettings.Shape = StandingShape;
	Character = New<FCharacterVirtual>(InCharacterSettings);
	MovementComponent = New<PMovementComponent>(this);
	this->AddComponent(MovementComponent);
	FPhysicsManager::GetManagerSingleton()->AddCharacter(Character);
}

void PigeonEngine::PCharacter::UninitCharacter()
{
	if (Character)
	{
        FPhysicsManager::GetManagerSingleton()->RemoveCharacter(Character);
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
}

void PigeonEngine::PCharacter::SetUp(Vector3 UpVector)
{
	FCharacterVirtual* PhysicsCharacter = GetPhysicsCharacter();
	if (PhysicsCharacter)
	{
		PhysicsCharacter->SetUp(UpVector);
	}
}

void PigeonEngine::PCharacter::HandleWorldUpVectorChanged(const Vector3& InUpVector)
{
	SetUp(InUpVector);
}

PigeonEngine::FShape* PigeonEngine::PCharacter::GetStandingShape()
{
	return StandingShape;
}

PigeonEngine::FShape* PigeonEngine::PCharacter::GetCrouchingShape()
{
	return CrouchingShape;
}

PigeonEngine::FCharacterVirtual* PigeonEngine::PCharacter::GetPhysicsCharacter()
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
    OnWorldUpVectorChangedHandler = [this](const Vector3& InUpVector)
	{
		HandleWorldUpVectorChanged(InUpVector);
	};
	if (World)
	{
		World->OnUpVectorChanged.Add(OnWorldUpVectorChangedHandler);
	}
	Character->AddToPhysicsSystem(EActivate::Activate, GetActorLocation(), GetActorRotation(), GetUniqueID());
	Character->AddListener(this);
	Character->Activate();
	if (World && Character)
	{
        HandleWorldUpVectorChanged(World->GetUpVector());
	}
}

void PigeonEngine::PCharacter::RemovedFromScene()
{
    if (PWorld* World = GetWorld())
	{
		World->OnUpVectorChanged.Remove(OnWorldUpVectorChangedHandler);
	}
	PPawn::RemovedFromScene();
	Character->RemoveListener(this);
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

void PigeonEngine::PCharacter::OnAdjustBodyVelocity(const FPhysicsBodyId& inBodyID2, Vector3& ioLinearVelocity, Vector3& ioAngularVelocity)
{
	OnPhysicsAdjustBodyVelocity(inBodyID2, ioLinearVelocity, ioAngularVelocity);
}

bool PigeonEngine::PCharacter::OnContactValidate(const FPhysicsBodyId& inBodyID2, UINT32 inSubShapeID2)
{
	return OnPhysicsContactValidate(inBodyID2, inSubShapeID2);
}

void PigeonEngine::PCharacter::OnContactAdded(const FPhysicsBodyId& inBodyID2, UINT32 inSubShapeID2, const Vector3& inContactPosition, const Vector3& inContactNormal, FPhysicsCharacterContactSettings& ioSettings)
{
	OnPhysicsContactAdded(inBodyID2, inSubShapeID2, inContactPosition, inContactNormal, ioSettings);
}

void PigeonEngine::PCharacter::OnContactSolve(const FPhysicsBodyId& inBodyID2, UINT32 inSubShapeID2, const Vector3& inContactPosition, const Vector3& inContactNormal, const Vector3& inContactVelocity, const Vector3& inCharacterVelocity, Vector3& ioNewCharacterVelocity)
{
	OnPhysicsContactSolve(inBodyID2, inSubShapeID2, inContactPosition, inContactNormal, inContactVelocity, inCharacterVelocity, ioNewCharacterVelocity);
}
