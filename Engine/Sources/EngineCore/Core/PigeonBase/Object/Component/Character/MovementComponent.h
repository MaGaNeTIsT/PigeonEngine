#pragma once
#include "../Component.h"
#include "../ActorComponent.h"
#include "../../Actor/LevelCharacter.h"
#include "PigeonBase/Object/Controller/CharacterController.h"


namespace PigeonEngine 
{
	enum class MoveType : UINT8
	{
		Move,
		Stop,
		None
	};

	struct EMoveState
	{
		Vector3 Direction;
		MoveType MoveType;
		int Order;
	};

	class PMovementComponent :public PActorComponent
	{
	public:
		PMovementComponent() :m_Character(nullptr) { POBJ_DEBUGNAME_SET(this, "MovementComponent"); }
		PMovementComponent(PCharacter* InCharacter) :m_Character(InCharacter) {};
		virtual ~PMovementComponent();

		virtual void Init() override;
		virtual void Uninit() override;

		void SetCharacter(PCharacter* InCharacter) { m_Character = InCharacter; };

		void HandleInput(const ECharacterMoveInput& Input);

	private:
		virtual void HandleInputInternal(Vector3 InMovementDirection, BOOL32 InJump, BOOL32 InSwitchStance, BOOL32 InRun);

	public:


	private:
		PCharacter* m_Character = nullptr;
	};
}