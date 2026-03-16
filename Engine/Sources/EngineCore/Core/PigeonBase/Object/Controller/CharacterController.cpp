#include "CharacterController.h"

#include "../../../../Editor/EditorLogManager.h"
#include "../../../../HID/Input/InputType.h"
#include "PigeonBase/Object/Component/CameraAndLight/CameraComponent.h"
#include "PigeonBase/Object/Component/Character/MovementComponent.h"

#if _EDITOR_ONLY
#include <imgui.h>
#endif

namespace PigeonEngine
{
    void PCharacterController::BeginAddedToScene(PWorld* World)
    {
        PController::BeginAddedToScene(World);
        EInput::KeyEvent.Add(TFunction<void(IKeyboard::Event::EType, BYTE)>([this](IKeyboard::Event::EType Type, const BYTE& KeyCode)
        {
            HandleInput(Type, EKeysBuiltIn::GetKeyByKeyCode(KeyCode));
        }));
    }

    void PCharacterController::RemovedFromScene()
    {
        PController::RemovedFromScene();
    }

    void PCharacterController::UserBeginPlay()
    {
        PController::UserBeginPlay();
    }

    void PCharacterController::UserTick(FLOAT deltaTime)
    {
        PController::UserTick(deltaTime);
        if (Character)
        {
            if (PMovementComponent* MovementComponent = Character->GetMovementComponent())
            {
                MovementComponent->HandleInput(CharacterMoveInput);
            }
        }
    }

    void PCharacterController::UserEndPlay()
    {
        PController::UserEndPlay();
    }

    void PCharacterController::HandleInput(IKeyboard::Event::EType InType, const EKey& InKey)
    {
        if (InKey.GetKeyCode() == EKeysBuiltIn::Up.GetKeyCode())
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
        if (InKey.GetKeyCode() == EKeysBuiltIn::Down.GetKeyCode())
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
        if (InKey.GetKeyCode() == EKeysBuiltIn::Left.GetKeyCode())
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
        if (InKey.GetKeyCode() == EKeysBuiltIn::Right.GetKeyCode())
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

        if (InKey.GetKeyCode() == EKeysBuiltIn::SpaceBar.GetKeyCode())
        { 
            switch (InType)
            {
            case PigeonEngine::IKeyboard::Event::EType::Press:
                CharacterMoveInput.bJump = TRUE;
                break;
            case PigeonEngine::IKeyboard::Event::EType::Release:
                CharacterMoveInput.bJump = FALSE;
                break;
            }
        }

        if (InKey.GetKeyCode() == EKeysBuiltIn::LeftShift.GetKeyCode())
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

        if (InKey.GetKeyCode() == EKeysBuiltIn::LeftControl.GetKeyCode())
        {
            switch (InType)
            {
            case PigeonEngine::IKeyboard::Event::EType::Press:
                CharacterMoveInput.bCrouch = TRUE;
                break;
            case PigeonEngine::IKeyboard::Event::EType::Release:
                CharacterMoveInput.bCrouch = FALSE;
                break;
            }
        }
    }

    void PCharacterController::SetCharacter(PCharacter* InCharacter)
    {
        Character = InCharacter;
    }

    PCharacterController::PCharacterController()
    {
        CharacterMoveInput.XInput = 0.f;
        CharacterMoveInput.YInput = 0.f;
        CharacterMoveInput.bJump = FALSE;
        CharacterMoveInput.bRun = FALSE;
        CharacterMoveInput.bCrouch = FALSE;
    }
    PCharacterController::~PCharacterController()
    {
    }
}
