#include "CharacterInputComponent.h"
#include "../../../../../HID/Input/Input.h"
#include "MovementComponent.h"
#include "PigeonBase/Object/Actor/LevelCharacter.h"
#include "PigeonBase/Object/Controller/CharacterController.h"

namespace PigeonEngine
{
	PCharacterInputComponent::PCharacterInputComponent(PCharacter* InCharacter, PCharacterController* InController)
		: Character(InCharacter)
		, Controller(InController)
	{
		POBJ_DEBUGNAME_SET(this, "CharacterInputComponent");
	}

	PCharacterInputComponent::~PCharacterInputComponent()
	{
	}

	void PCharacterInputComponent::Init()
	{
		PActorComponent::Init();
		if (Controller && !bInputRegistered)
		{
			KeyInputHandler = TFunction<void(IKeyboard::Event::EType, BYTE)>([this](IKeyboard::Event::EType Type, const BYTE& KeyCode)
			{
				HandleInput(Type, EKeysBuiltIn::GetKeyByKeyCode(KeyCode));
			});
			EInput::KeyEvent.Add(KeyInputHandler);
			bInputRegistered = TRUE;
		}
	}

	void PCharacterInputComponent::Uninit()
	{
		PActorComponent::Uninit();
	}

	void PCharacterInputComponent::FixedTick(FLOAT deltaTime)
	{
		PActorComponent::FixedTick(deltaTime);
		if (!Character)
		{
			return;
		}

		if (PMovementComponent* MovementComponent = Character->GetMovementComponent())
		{
			MovementComponent->HandleInput(ConsumeMoveInput());
		}
	}

	void PCharacterInputComponent::SetCharacter(PCharacter* InCharacter)
	{
		Character = InCharacter;
	}

	void PCharacterInputComponent::SetController(PCharacterController* InController)
	{
		Controller = InController;
	}

	void PCharacterInputComponent::SetInputBindings(const FCharacterInputBindings& InBindings)
	{
		InputBindings = InBindings;
	}

	const FCharacterInputBindings& PCharacterInputComponent::GetInputBindings() const
	{
		return InputBindings;
	}

	void PCharacterInputComponent::HandleInput(IKeyboard::Event::EType InType, const EKey& InKey)
	{
		if (InKey.GetKeyCode() == InputBindings.MoveForward.GetKeyCode())
		{
			switch (InType)
			{
			case PigeonEngine::IKeyboard::Event::EType::Press:
				CharacterMoveInput.YInput += 1.f;
				break;
			case PigeonEngine::IKeyboard::Event::EType::Release:
				CharacterMoveInput.YInput -= 1.f;
				break;
			}
		}
		if (InKey.GetKeyCode() == InputBindings.MoveBackward.GetKeyCode())
		{
			switch (InType)
			{
			case PigeonEngine::IKeyboard::Event::EType::Press:
				CharacterMoveInput.YInput -= 1.f;
				break;
			case PigeonEngine::IKeyboard::Event::EType::Release:
				CharacterMoveInput.YInput += 1.f;
				break;
			}
		}
		if (InKey.GetKeyCode() == InputBindings.MoveLeft.GetKeyCode())
		{
			switch (InType)
			{
			case PigeonEngine::IKeyboard::Event::EType::Press:
				CharacterMoveInput.XInput -= 1.f;
				break;
			case PigeonEngine::IKeyboard::Event::EType::Release:
				CharacterMoveInput.XInput += 1.f;
				break;
			}
		}
		if (InKey.GetKeyCode() == InputBindings.MoveRight.GetKeyCode())
		{
			switch (InType)
			{
			case PigeonEngine::IKeyboard::Event::EType::Press:
				CharacterMoveInput.XInput += 1.f;
				break;
			case PigeonEngine::IKeyboard::Event::EType::Release:
				CharacterMoveInput.XInput -= 1.f;
				break;
			}
		}

		if (InKey.GetKeyCode() == InputBindings.Jump.GetKeyCode())
		{
			switch (InType)
			{
			case PigeonEngine::IKeyboard::Event::EType::Press:
				CharacterMoveInput.bJump = TRUE;
				CharacterMoveInput.bJumpPressed = TRUE;
				break;
			case PigeonEngine::IKeyboard::Event::EType::Release:
				CharacterMoveInput.bJump = FALSE;
				break;
			}
		}

		if (InKey.GetKeyCode() == InputBindings.Run.GetKeyCode())
		{
			switch (InType)
			{
			case PigeonEngine::IKeyboard::Event::EType::Press:
				CharacterMoveInput.bRun = TRUE;
				break;
			case PigeonEngine::IKeyboard::Event::EType::Release:
				CharacterMoveInput.bRun = FALSE;
				break;
			}
		}

		if (InKey.GetKeyCode() == InputBindings.Crouch.GetKeyCode())
		{
			switch (InType)
			{
			case PigeonEngine::IKeyboard::Event::EType::Press:
				CharacterMoveInput.bCrouch = TRUE;
				CharacterMoveInput.bCrouchPressed = TRUE;
				break;
			case PigeonEngine::IKeyboard::Event::EType::Release:
				CharacterMoveInput.bCrouch = FALSE;
				break;
			}
		}
	}

	ECharacterMoveInput PCharacterInputComponent::ConsumeMoveInput()
	{
		ECharacterMoveInput CurrentInput = CharacterMoveInput;
		CurrentInput.bJump = CharacterMoveInput.bJump;
		CurrentInput.bCrouch = CharacterMoveInput.bCrouchPressed;
		CharacterMoveInput.bJumpPressed = FALSE;
		CharacterMoveInput.bCrouchPressed = FALSE;
		return CurrentInput;
	}
}
