#include "MovementComponent.h"
#include "../../../../../../../EngineThirdParty/JoltPhysics/Headers/Shapes.h"
#include "../../../../../../../EngineThirdParty/JoltPhysics/Headers/PhysicsManager.h"
#include "../../../../../../../EngineThirdParty/JoltPhysics/Headers/Character/Character.h"
#include "../../../../../../../EngineThirdParty/JoltPhysics/Headers/Character/CharacterBase.h"

namespace PigeonEngine
{
	PMovementComponent::~PMovementComponent()
	{
	}

	void PMovementComponent::Init()
	{
	}

	void PMovementComponent::Uninit()
	{
	}

	void PMovementComponent::HandleInput(Vector3 InMovementDirection, BOOL32 InJump, BOOL32 inSwitchStance)
	{
		FCharacter* character = m_Character->GetPhysicsCharacter();
		character->SetPositionAndRotation(m_Character->GetActorLocation(), m_Character->GetActorRotation());
		FCharacterBase::EGroundState ground_state = character->GetGroundState();
		if (ground_state == FCharacterBase::EGroundState::OnSteepGround
			|| ground_state == FCharacterBase::EGroundState::NotSupported)
		{
			Vector3 normal = character->GetGroundNormal();
			normal.y = (0.0f);
			float dot = normal.Dot(InMovementDirection);
			if (dot < 0.0f)
				InMovementDirection -= (dot * normal) / normal.LengthSquare();
		}

		// Stance switch
		if (inSwitchStance)
			character->SetShape(character->GetShape() == m_Character->GetStandingShape() ? m_Character->GetCrouchingShape() : m_Character->GetStandingShape(), 1.5f * 0.02f);

		if (m_Character->ControlMovementDuringJump || character->IsSupported())
		{
			// Update velocity
			Vector3 current_velocity = character->GetLinearVelocity();
			Vector3 desired_velocity = m_Character->CharacterSpeed * InMovementDirection;
			desired_velocity.y = current_velocity.y;
			Vector3 new_velocity = 0.75f * current_velocity + 0.25f * desired_velocity;

			// Jump
			if (InJump && ground_state == FCharacterBase::EGroundState::OnGround)
				new_velocity += Vector3(0.f, m_Character->JumpSpeed, 0.f);

			// Update the velocity
			character->SetLinearVelocity(new_velocity);
		}

		m_Character->SetActorLocation(character->GetPosition());
		m_Character->SetActorRotation(character->GetRotation());
	}
}