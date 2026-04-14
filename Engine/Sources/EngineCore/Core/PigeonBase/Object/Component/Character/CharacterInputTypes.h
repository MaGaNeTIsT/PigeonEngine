#pragma once
#include <CoreMinimal.h>
#include "Input/InputType.h"

namespace PigeonEngine
{
	class FCharacterInputTypes final
	{
	public:
		struct FMoveInput
		{
			FLOAT XInput = 0.f;
			FLOAT YInput = 0.f;
			BOOL8 bJump = FALSE;
			BOOL8 bJumpPressed = FALSE;
			BOOL8 bRun = FALSE;
			BOOL8 bCrouch = FALSE;
			BOOL8 bCrouchPressed = FALSE;
		};

		struct FBindings
		{
			EKey MoveForward = EKeysBuiltIn::Up;
			EKey MoveBackward = EKeysBuiltIn::Down;
			EKey MoveLeft = EKeysBuiltIn::Left;
			EKey MoveRight = EKeysBuiltIn::Right;
			EKey Jump = EKeysBuiltIn::SpaceBar;
			EKey Run = EKeysBuiltIn::LeftShift;
			EKey Crouch = EKeysBuiltIn::LeftControl;
		};
	};

	using ECharacterMoveInput = FCharacterInputTypes::FMoveInput;
	using FCharacterInputBindings = FCharacterInputTypes::FBindings;
}
