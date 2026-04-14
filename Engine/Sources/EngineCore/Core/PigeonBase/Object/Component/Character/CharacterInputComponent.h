#pragma once
#pragma once
#include "../ActorComponent.h"
#include "CharacterInputTypes.h"

namespace PigeonEngine
{
	class PCharacter;
	class PCharacterController;

	class PCharacterInputComponent : public PActorComponent
	{
	public:
		PCharacterInputComponent() { POBJ_DEBUGNAME_SET(this, "CharacterInputComponent"); }
		PCharacterInputComponent(PCharacter* InCharacter, PCharacterController* InController);
		virtual ~PCharacterInputComponent();

		virtual void Init() override;
		virtual void Uninit() override;
		virtual void FixedTick(FLOAT deltaTime) override;

		void SetCharacter(PCharacter* InCharacter);
		void SetController(PCharacterController* InController);
		void SetInputBindings(const FCharacterInputBindings& InBindings);
		const FCharacterInputBindings& GetInputBindings() const;

	private:
		void HandleInput(IKeyboard::Event::EType InType, const EKey& InKey);
		ECharacterMoveInput ConsumeMoveInput();

	private:
		PCharacter* Character = nullptr;
		PCharacterController* Controller = nullptr;
		FCharacterInputBindings InputBindings{};
		ECharacterMoveInput CharacterMoveInput{};
		TFunction<void(IKeyboard::Event::EType, BYTE)> KeyInputHandler;
		BOOL32 bInputRegistered = FALSE;
	};
}
