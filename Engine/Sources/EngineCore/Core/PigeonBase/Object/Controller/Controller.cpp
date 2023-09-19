#include "Controller.h"

#include "PigeonBase/Object/Component/CameraAndLight/CameraComponent.h"

namespace PigeonEngine
{
    void PController::BeginAddedToScene(PWorld* World)
    {
        PActor::BeginAddedToScene(World);
    }

    void PController::RemovedFromScene()
    {
        PActor::RemovedFromScene();
    }

    void PController::Init()
    {
        PActor::Init();
        
      
    }

    void PController::Uninit()
    {
        PActor::Uninit();
    }

    void PController::UserBeginPlay()
    {
        PActor::UserBeginPlay();
    }

    void PController::UserTick(FLOAT deltaTime)
    {
        PActor::UserTick(deltaTime);
    }

    void PController::UserEndPlay()
    {
        PActor::UserEndPlay();
    }

    PCameraComponent* PController::GetCamera() const
    {
        return this->MyCamera;
    }

    void PController::SetCamera(PCameraComponent* NewCam)
    {
        if(this->MyCamera)
        {
            this->MyCamera->SetIsMainCamera(FALSE);
        }
        this->MyCamera = NewCam;
        this->MyCamera->SetIsMainCamera(TRUE);
    }

    void PController::ResetToDefaultCamera()
    {
        PCameraComponent* MyCam = GetRootComponent()->AsType<PCameraComponent>();
        if(MyCam)
        {
            SetCamera(MyCam);
        }
    }

    PController::PController()
    {
        MyCamera = new PCameraComponent();
        this->SetRootComponent(MyCamera);
    }
    PController::~PController()
    {
    }
}
