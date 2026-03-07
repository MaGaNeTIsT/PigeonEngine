#pragma once
#include "Input/InputType.h"
#include "PigeonBase/Object/Controller/Controller.h"
#include "PigeonBase/Object/Actor/LevelCharacter.h"
namespace PigeonEngine
{
    struct ECharacterMoveInput
    {
        FLOAT XInput;
        FLOAT YInput;
        BOOL8 bJump;
        BOOL8 bRun;
        BOOL8 bCrouch;
    };
    class PCharacter;
    class PCharacterController : public PController
    {
        friend class PWorld;
    protected:
        // for frame, 
        void BeginAddedToScene(PWorld* World) override;
        void RemovedFromScene() override;
        
        // for users
        void UserBeginPlay() override;
        void UserTick(FLOAT deltaTime) override;
        void UserEndPlay() override;

        void HandleInput(IKeyboard::Event::EType InType, const EKey& InKey);
    private:
        const TFunction<void(IKeyboard::Event::EType Type, BYTE KeyCode)> OnKeyDown;
    public:
        void SetCharacter(PCharacter* InCharacter);
    public:
        CLASS_VIRTUAL_NOCOPY_BODY(PCharacterController)

    private:
        PCharacter* Character = NULL;
        ECharacterMoveInput CharacterMoveInput;
    };
    
}

