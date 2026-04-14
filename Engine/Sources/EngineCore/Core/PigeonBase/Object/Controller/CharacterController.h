#pragma once
#pragma once
#include "PigeonBase/Object/Controller/Controller.h"
#include "PigeonBase/Object/Actor/LevelCharacter.h"
namespace PigeonEngine
{
	class PCharacterInputComponent;
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
    public:
        void SetCharacter(PCharacter* InCharacter);
        PE_NODISCARD PCharacterInputComponent* GetInputComponent() const;
    public:
        CLASS_VIRTUAL_NOCOPY_BODY(PCharacterController)

    private:
        PCharacter* Character = NULL;
		PCharacterInputComponent* InputComponent = nullptr;
    };
    
}

