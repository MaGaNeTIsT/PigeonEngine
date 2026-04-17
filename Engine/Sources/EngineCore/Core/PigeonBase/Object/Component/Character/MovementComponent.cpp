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
        if (m_Character)
		{
			LastSyncedPosition = m_Character->GetActorLocation();
			LastSyncedRotation = m_Character->GetActorRotation();
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
		if (m_Character && m_Character->GetWorld() && !bPostPhysicsTickRegistered)
		{
			PWorld* World = m_Character->GetWorld();
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
		if (!m_Character)
		{
			return;
		}

		if (FCharacterVirtual* Character = m_Character->GetPhysicsCharacter())
		{
            const Vector3 NewPosition = Character->GetPosition();
			const Quaternion NewRotation = Character->GetRotation();
          if (!bHasSyncedTransform || PhysicsUtility::HasTransformChanged(NewPosition, NewRotation, LastSyncedPosition, LastSyncedRotation))
			{
				m_Character->SetActorLocation(NewPosition);
				m_Character->SetActorRotation(NewRotation);
				LastSyncedPosition = NewPosition;
				LastSyncedRotation = NewRotation;
				bHasSyncedTransform = TRUE;
			}
		}
	}

	void PMovementComponent::HandleInput(const ECharacterMoveInput& Input)
	{
		Vector3 MovementDirection = m_Character->GetActorForwardVector() * EMath::Clamp(Input.YInput, -1.f, 1.f) + m_Character->GetActorRightVector() * EMath::Clamp(Input.XInput, -1.f, 1.f);
		MovementDirection.Normalize();
		HandleInputInternal(MovementDirection, Input.bJump, Input.bCrouch, Input.bRun);
	}

	void PMovementComponent::HandleInputInternal(Vector3 InMovementDirection, BOOL32 InJump, BOOL32 InSwitchStance, BOOL32 InRun)
	{
		FCharacterVirtual* Character = m_Character->GetPhysicsCharacter();
		const Vector3 WorldUp = GetCharacterWorldUp(m_Character);
		//character->SetPositionAndRotation(m_Character->GetActorLocation(), m_Character->GetActorRotation());
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
			Character->SetShape(Character->GetShape() == m_Character->GetStandingShape() ? m_Character->GetCrouchingShape() : m_Character->GetStandingShape(), 1.5f * 0.02f);

		if (m_Character->ControlMovementDuringJump || Character->IsSupported())
		{
			// Update velocity
			Vector3 CurrentVelocity = Character->GetLinearVelocity();
            Vector3 DesiredVelocity = (InRun ? m_Character->CharacterRunSpeed : m_Character->CharacterSpeed) * InMovementDirection;
			DesiredVelocity += WorldUp * Vector3::Dot(CurrentVelocity, WorldUp);
			Vector3 NewVelocity = 0.75f * CurrentVelocity + 0.25f * DesiredVelocity;

			// Jump
			if (InJump && Character->IsSupported())
             NewVelocity += WorldUp * (m_Character->JumpSpeed - Vector3::Dot(NewVelocity, WorldUp));

			// Update the velocity
			Character->SetLinearVelocity(NewVelocity);
		}

	}
}