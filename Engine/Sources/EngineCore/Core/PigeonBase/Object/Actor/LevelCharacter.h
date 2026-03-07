#pragma once
#include <CoreMinimal.h>
#include "Pawn.h"

namespace PigeonEngine
{
	class PCharacter : public PPawn
	{
		friend class PWorld;
		CLASS_VIRTUAL_NOCOPY_BODY(PCharacter)
	public:
		/// <summary>
		/// Init a level character use settings,host point.
		/// </summary>
		/// <param name="InCharacterSettings"></param>
		virtual void InitCharacter(class FCharacterSettings* InCharacterSettings);
		virtual void UninitCharacter();

		class FShape* GetStandingShape();
		class FShape* GetCrouchingShape();
		class FCharacter* GetPhysicsCharacter();
		class PMovementComponent* GetMovementComponent();

#if _EDITOR_ONLY
		void EditorTick(FLOAT deltaTime) override;
#endif

	protected:
		// for frame, 
		virtual void BeginAddedToScene(PWorld* World) override;
		virtual void RemovedFromScene() override;
	protected:

		virtual void UserBeginPlay();
		virtual void UserTick(FLOAT deltaTime) override;
		virtual void UserEndPlay();
	public:
		// Character size
		FLOAT			CharacterHeightStanding			= 135.f;
		FLOAT			CharacterRadiusStanding			= 30.f;
		FLOAT			CharacterHeightCrouching		= 80.f;
		FLOAT			CharacterRadiusCrouching		= 30.f;

		// Character movement properties
		BOOL32			ControlMovementDuringJump		= TRUE;	///< If false the character cannot change movement direction in mid air
		FLOAT			CharacterSpeed					= 6.0f;
		FLOAT			CharacterRunSpeed				= 12.0f;
		FLOAT			JumpSpeed						= 4.0f;

	protected:
		class PMovementComponent*	MoveMentComponent	= nullptr;
		class FCharacter*			Character			= nullptr;
		// The different stances for the character
		class FShape*				StandingShape		= nullptr;
		class FShape*				CrouchingShape		= nullptr;
	};
}