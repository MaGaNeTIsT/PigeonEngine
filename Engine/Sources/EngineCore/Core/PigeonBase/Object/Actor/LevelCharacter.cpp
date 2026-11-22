#include "LevelCharacter.h"
#include "../Component/Character/MovementComponent.h"
#include "../../../../../../EngineThirdParty/JoltPhysics/Headers/Shapes.h"
#include "../../../../../../EngineThirdParty/JoltPhysics/Headers/Character/CharacterVirtual.h"
#include "../../../../../../EngineThirdParty/JoltPhysics/Headers/PhysicsManager.h"

#include <PhysicsConfig/PhysicsConfig.h>
#include <PigeonBase/Object/World/World.h>
#include <Reflection/Property.h>

namespace PigeonEngine
{
	static void RegisterClassProperties()
	{
		const size_t ClassHash = GetClassHashCode<PCharacter>();
		AddClassPropertyAccessors<PCharacter, FLOAT>(ClassHash, PE_TEXT("CharacterHeightStanding"),
			[](const PCharacter* Character) -> FLOAT { return Character->CharacterHeightStanding; },
			[](PCharacter* Character, const FLOAT& Value) -> void { Character->CharacterHeightStanding = Value; },
			static_cast<UINT32>(EPropertyFlagsType::EDIT_ANYWHERE));
		AddClassPropertyAccessors<PCharacter, FLOAT>(ClassHash, PE_TEXT("CharacterRadiusStanding"),
			[](const PCharacter* Character) -> FLOAT { return Character->CharacterRadiusStanding; },
			[](PCharacter* Character, const FLOAT& Value) -> void { Character->CharacterRadiusStanding = Value; },
			static_cast<UINT32>(EPropertyFlagsType::EDIT_ANYWHERE));
		AddClassPropertyAccessors<PCharacter, FLOAT>(ClassHash, PE_TEXT("CharacterHeightCrouching"),
			[](const PCharacter* Character) -> FLOAT { return Character->CharacterHeightCrouching; },
			[](PCharacter* Character, const FLOAT& Value) -> void { Character->CharacterHeightCrouching = Value; },
			static_cast<UINT32>(EPropertyFlagsType::EDIT_ANYWHERE));
		AddClassPropertyAccessors<PCharacter, FLOAT>(ClassHash, PE_TEXT("CharacterRadiusCrouching"),
			[](const PCharacter* Character) -> FLOAT { return Character->CharacterRadiusCrouching; },
			[](PCharacter* Character, const FLOAT& Value) -> void { Character->CharacterRadiusCrouching = Value; },
			static_cast<UINT32>(EPropertyFlagsType::EDIT_ANYWHERE));
		AddClassBoolPropertyAccessors<PCharacter>(ClassHash, PE_TEXT("ControlMovementDuringJump"),
			[](const PCharacter* Character) -> BOOL32 { return Character->ControlMovementDuringJump; },
			[](PCharacter* Character, const BOOL32& Value) -> void { Character->ControlMovementDuringJump = Value; },
			static_cast<UINT32>(EPropertyFlagsType::EDIT_ANYWHERE));
		AddClassPropertyAccessors<PCharacter, FLOAT>(ClassHash, PE_TEXT("CharacterSpeed"),
			[](const PCharacter* Character) -> FLOAT { return Character->CharacterSpeed; },
			[](PCharacter* Character, const FLOAT& Value) -> void { Character->CharacterSpeed = Value; },
			static_cast<UINT32>(EPropertyFlagsType::EDIT_ANYWHERE));
		AddClassPropertyAccessors<PCharacter, FLOAT>(ClassHash, PE_TEXT("CharacterRunSpeed"),
			[](const PCharacter* Character) -> FLOAT { return Character->CharacterRunSpeed; },
			[](PCharacter* Character, const FLOAT& Value) -> void { Character->CharacterRunSpeed = Value; },
			static_cast<UINT32>(EPropertyFlagsType::EDIT_ANYWHERE));
		AddClassPropertyAccessors<PCharacter, FLOAT>(ClassHash, PE_TEXT("JumpSpeed"),
			[](const PCharacter* Character) -> FLOAT { return Character->JumpSpeed; },
			[](PCharacter* Character, const FLOAT& Value) -> void { Character->JumpSpeed = Value; },
			static_cast<UINT32>(EPropertyFlagsType::EDIT_ANYWHERE));
		// The physics layer a Jolt character is created with. It is serialized because a level
		// character otherwise falls back to the settings default (NON_MOVING) and ignores static
		// floors; the member default is MOVING for levels saved before this property existed.
		AddClassEnumValuePropertyAccessors<PCharacter>(ClassHash, PE_TEXT("Layer"), GetClassHashCode<FPhysicsObjectLayer>(),
			[](const PCharacter* Character) -> INT32 { return static_cast<INT32>(Character->GetCharacterLayer().GetValue()); },
			[](PCharacter* Character, const INT32& Value) -> void { Character->SetCharacterLayer(FPhysicsObjectLayer(static_cast<FPhysicsObjectLayer::Type>(Value))); },
			static_cast<UINT32>(EPropertyFlagsType::EDIT_ANYWHERE));
	}

	static void RegisterClassTypes()
	{
		RegisterClassType<PCharacter, PPawn>();
		RegisterClassProperties();
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
		StandingShape = new FRotatedTranslatedShape(Vector3(0.f, 0.5f * CharacterHeightStanding + CharacterRadiusStanding, 0.f), Quaternion::Identity(), new FCapsuleShape(0.5f * CharacterHeightStanding, CharacterRadiusStanding));
		CrouchingShape = new FRotatedTranslatedShape(Vector3(0.f, 0.5f * CharacterHeightCrouching + CharacterRadiusCrouching, 0.f), Quaternion::Identity(), new FCapsuleShape(0.5f * CharacterHeightCrouching, CharacterRadiusCrouching));
		InCharacterSettings.Shape = StandingShape;
		// The serialized property is authoritative for every creation path, including the
		// level loader's PostDeserialize and the sanity round trip.
		InCharacterSettings.Layer = CharacterLayer;
		Character = new FCharacterVirtual(InCharacterSettings);
	// A loaded character may already carry a serialized movement component. Reuse and link
	// it instead of adding a second one; for a freshly launched character this creates it.
	if (!MovementComponent)
	{
		MovementComponent = FindComponent<PMovementComponent>();
	}
	if (MovementComponent)
	{
		MovementComponent->SetCharacter(this);
	}
	else
	{
		MovementComponent = new PMovementComponent(this);
		this->AddComponent(MovementComponent);
	}
	FPhysicsManager::GetManagerSingleton()->AddCharacter(Character);
}

void PigeonEngine::PCharacter::SetCharacterLayer(const FPhysicsObjectLayer& InLayer)
{
	if (CharacterLayer == InLayer)
	{
		return;
	}
	CharacterLayer = InLayer;
	if (!Character)
	{
		return;
	}

	// Jolt takes the object layer from CharacterVirtualSettings at creation; changing it means
	// rebuilding the body. Preserve the solved transform and re-register it if the actor is
	// already in a world.
	const Vector3 Position = Character->GetPosition();
	const Quaternion Rotation = Character->GetRotation();
	PWorld* World = GetWorld();
	Character->RemoveListener(this);
	UninitCharacter();
	FCharacterVirtualSettings Settings;
	InitCharacter(Settings);
	Character->AddListener(this);
	if (World)
	{
		Character->AddToPhysicsSystem(EActivate::Activate, Position, Rotation, GetUniqueID());
		Character->Activate();
		HandleWorldUpVectorChanged(World->GetUpVector());
	}
}

const PigeonEngine::FPhysicsObjectLayer& PigeonEngine::PCharacter::GetCharacterLayer() const
{
	return CharacterLayer;
}

void PigeonEngine::PCharacter::UninitCharacter()
{
	if (Character)
	{
		FPhysicsManager::GetManagerSingleton()->RemoveCharacter(Character);
		delete Character;
		Character = nullptr;
	}
	if (StandingShape)
	{
		delete StandingShape;
		StandingShape = nullptr;
	}
	if (CrouchingShape)
	{
		delete CrouchingShape;
		CrouchingShape = nullptr;
	}
	// Components are destroyed before this runs from the destructor; do not leave a pointer
	// to a component the actor has already released.
	MovementComponent = nullptr;
}

void PigeonEngine::PCharacter::PostDeserialize()
{
	PActor::PostDeserialize();
	if (!Character)
	{
		// The serialized height/radius/Layer properties are already applied; InitCharacter
		// reads them when it builds the standing and crouching shapes.
		FCharacterVirtualSettings Settings;
		InitCharacter(Settings);
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

#if ENGINE_EDITOR_ONLY
void PigeonEngine::PCharacter::EditorTick(FLOAT deltaTime)
{
	(void)deltaTime;
	if ((Character != nullptr)
		&& (Character->GetShape() != nullptr)
		&& (FPhysicsCommonSettings::GetSingleton()->bDrawCollision != FALSE))
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

PigeonEngine::BOOL8 PigeonEngine::PCharacter::OnContactValidate(const FPhysicsBodyId& inBodyID2, UINT32 inSubShapeID2)
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
