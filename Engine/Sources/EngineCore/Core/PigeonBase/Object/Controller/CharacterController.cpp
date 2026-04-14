#include "CharacterController.h"

#include "../../../../Editor/EditorLogManager.h"
#include "../../../../HID/Input/InputType.h"
#include "PigeonBase/Object/Component/CameraAndLight/CameraComponent.h"
#include "PigeonBase/Object/Component/Character/CharacterInputComponent.h"
#include "PigeonBase/Object/Component/Character/MovementComponent.h"

#if _EDITOR_ONLY
#include <imgui.h>
#endif

namespace PigeonEngine
{
    void PCharacterController::BeginAddedToScene(PWorld* World)
    {
        PController::BeginAddedToScene(World);
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
    }

    void PCharacterController::UserEndPlay()
    {
        PController::UserEndPlay();
    }

    void PCharacterController::SetCharacter(PCharacter* InCharacter)
    {
        Character = InCharacter;
		if (InputComponent)
		{
			InputComponent->SetCharacter(Character);
		}
    }

	PCharacterInputComponent* PCharacterController::GetInputComponent() const
	{
		return InputComponent;
	}

    PCharacterController::PCharacterController()
    {
		InputComponent = new PCharacterInputComponent(nullptr, this);
		InputComponent->SetIsTickable(TRUE);
		this->AddComponent(InputComponent);
    }
    PCharacterController::~PCharacterController()
    {
    }
}
