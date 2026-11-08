#include "MovementComponent.h"
#include "../../../../../../../EngineThirdParty/JoltPhysics/Headers/Shapes.h"
#include "../../../../../../../EngineThirdParty/JoltPhysics/Headers/PhysicsManager.h"
#include "../../../../../../../EngineThirdParty/JoltPhysics/Headers/Character/CharacterVirtual.h"
#include "../../../../../../../EngineThirdParty/JoltPhysics/Headers/Character/CharacterBase.h"
#include "PigeonBase/Object/World/World.h"
#include "PigeonBase/Object/World/WorldTickManager.h"
#include "Base/Math/Math.h"

namespace PigeonEngine
{
	namespace
	{
		Vector3 GetCharacterWorldUp(const PCharacter* InCharacter)
		{
			if (InCharacter && InCharacter->GetWorld())
			{
				return InCharacter->GetWorld()->GetUpVector();
			}

			return Vector3::YVector();
		}
	}

	PMovementComponent::~PMovementComponent()
	{
	}

	void PMovementComponent::Init()
	{
		PActorComponent::Init();
	}

	void PMovementComponent::BeginAddedToScene(PWorld* World)
	{
		PActorComponent::BeginAddedToScene(World);
		if (CharacterActor)
		{
			LastSyncedPosition = CharacterActor->GetActorLocation();
			LastSyncedRotation = CharacterActor->GetActorRotation();
			bHasSyncedTransform = TRUE;
		}
		TryRegisterPostPhysicsTick();
	}

	void PMovementComponent::RemovedFromScene()
	{
		TryUnregisterPostPhysicsTick();
		bHasSyncedTransform = FALSE;
		LastSyncedPosition = Vector3::Zero();
		LastSyncedRotation = Quaternion::Identity();
		PActorComponent::RemovedFromScene();
	}

	void PMovementComponent::TryRegisterPostPhysicsTick()
	{
		if (CharacterActor && CharacterActor->GetWorld() && !bPostPhysicsTickRegistered)
		{
			PWorld* World = CharacterActor->GetWorld();
			const ObjectIdentityType ComponentID = GetUniqueID();
			PostPhysicsTickHandler = TFunction<void(FLOAT)>([World, ComponentID](FLOAT deltaTime)
			{
				if (!World)
				{
					return;
				}

				const PActorComponent* FoundComponent = World->GetComponentByUniqueID(ComponentID, TRUE);
				PMovementComponent* MovementComponent = dynamic_cast<PMovementComponent*>(const_cast<PActorComponent*>(FoundComponent));
				if (MovementComponent)
				{
					MovementComponent->HandlePostPhysicsTick(deltaTime);
				}
			});
			EWorldTickManager::GetManagerSingleton()->RegisterPostPhysicsTick(PostPhysicsTickHandler);
			bPostPhysicsTickRegistered = TRUE;
		}
	}

	void PMovementComponent::TryUnregisterPostPhysicsTick()
	{
		if (bPostPhysicsTickRegistered)
		{
			EWorldTickManager::GetManagerSingleton()->UnregisterPostPhysicsTick(PostPhysicsTickHandler);
			bPostPhysicsTickRegistered = FALSE;
		}
	}

	void PMovementComponent::Uninit()
	{
		PActorComponent::Uninit();
	}

	void PMovementComponent::HandlePostPhysicsTick(FLOAT deltaTime)
	{
		(void)deltaTime;
		if (!CharacterActor)
		{
			return;
		}

		if (FCharacterVirtual* Character = CharacterActor->GetPhysicsCharacter())
		{
			const Vector3 NewPosition = Character->GetPosition();
			const Quaternion NewRotation = Character->GetRotation();
			if (!bHasSyncedTransform || PhysicsUtility::HasTransformChanged(NewPosition, NewRotation, LastSyncedPosition, LastSyncedRotation))
			{
				CharacterActor->SetActorLocation(NewPosition);
				CharacterActor->SetActorRotation(NewRotation);
				LastSyncedPosition = NewPosition;
				LastSyncedRotation = NewRotation;
				bHasSyncedTransform = TRUE;
			}
		}
	}

	void PMovementComponent::HandleInput(const ECharacterMoveInput& Input)
	{
		Vector3 MovementDirection = CharacterActor->GetActorForwardVector() * EMath::Clamp(Input.YInput, -1.f, 1.f) + CharacterActor->GetActorRightVector() * EMath::Clamp(Input.XInput, -1.f, 1.f);
		MovementDirection.Normalize();
		HandleInputInternal(MovementDirection, Input.bJump, Input.bCrouch, Input.bRun);
	}

	void PMovementComponent::HandleInputInternal(Vector3 InMovementDirection, BOOL32 InJump, BOOL32 InSwitchStance, BOOL32 InRun)
	{
		FCharacterVirtual* Character = CharacterActor->GetPhysicsCharacter();
		const Vector3 WorldUp = GetCharacterWorldUp(CharacterActor);
		//character->SetPositionAndRotation(CharacterActor->GetActorLocation(), CharacterActor->GetActorRotation());
		FCharacterBase::EGroundState GroundState = Character->GetGroundState();
		if (GroundState == FCharacterBase::EGroundState::OnSteepGround
			|| GroundState == FCharacterBase::EGroundState::NotSupported)
		{
			Vector3 Normal = Character->GetGroundNormal();
			Normal -= WorldUp * Vector3::Dot(Normal, WorldUp);
			float Dot = Normal.Dot(InMovementDirection);
		if (Dot < 0.0f && Normal.LengthSquare() > PE_SMALL_NUMBER)
				InMovementDirection -= (Dot * Normal) / Normal.LengthSquare();
		}

		// Stance switch
		if (InSwitchStance)
			Character->SetShape(Character->GetShape() == CharacterActor->GetStandingShape() ? CharacterActor->GetCrouchingShape() : CharacterActor->GetStandingShape(), 1.5f * 0.02f);

		if (CharacterActor->ControlMovementDuringJump || Character->IsSupported())
		{
			// Update velocity
			Vector3 CurrentVelocity = Character->GetLinearVelocity();
			Vector3 DesiredVelocity = (InRun ? CharacterActor->CharacterRunSpeed : CharacterActor->CharacterSpeed) * InMovementDirection;
			DesiredVelocity += WorldUp * Vector3::Dot(CurrentVelocity, WorldUp);
			Vector3 NewVelocity = 0.75f * CurrentVelocity + 0.25f * DesiredVelocity;

			// Jump
			if (InJump && Character->IsSupported())
			NewVelocity += WorldUp * (CharacterActor->JumpSpeed - Vector3::Dot(NewVelocity, WorldUp));

			// Update the velocity
			Character->SetLinearVelocity(NewVelocity);
		}

	}
}
