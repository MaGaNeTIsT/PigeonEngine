#pragma once
#include "../Component.h"
#include "../ActorComponent.h"
#include "../../Actor/LevelCharacter.h"


namespace PigeonEngine 
{

	class PMovementComponent :public PActorComponent
	{
	public:
		PMovementComponent() :m_Character(nullptr) { this->SetName("MovementComponent"); };
		PMovementComponent(PCharacter* InCharacter) :m_Character(InCharacter) {};
		virtual ~PMovementComponent();

		virtual void Init() override;
		virtual void Uninit() override;

		void SetCharacter(PCharacter* InCharacter) { m_Character = InCharacter; };

		virtual void HandleInput(Vector3 InMovementDirection, BOOL32 InJump, BOOL32 inSwitchStance);

	private:
		PCharacter* m_Character = nullptr;
	};
}