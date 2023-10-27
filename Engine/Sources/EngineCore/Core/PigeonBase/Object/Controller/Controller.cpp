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
        this->SetIsTickable(TRUE);
        SetCamera(MyCamera);
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
        this->MyCamera->SetIsTickable(TRUE);
    }

    void PController::ResetToDefaultCamera()
    {
        PCameraComponent* MyCam = GetRootComponent()->AsType<PCameraComponent>();
        if(MyCam)
        {
            SetCamera(MyCam);
        }
    }

    void PController::SetActorLocation(const Vector3& Location)
    {
        PActor::SetActorLocation(Location);
    }

    void PController::SetActorRotation(const Quaternion& Rotation)
    {
        PActor::SetActorRotation(Rotation);
    }

    void PController::SetActorScale(const Vector3& Scale)
    {
        PActor::SetActorScale(Scale);
    }

    PController::PController()
    {
        MyCamera = new PCameraComponent(0.f, 0.f, ESettings::ENGINE_SCREEN_WIDTH, ESettings::ENGINE_SCREEN_HEIGHT);
        this->SetRootComponent(MyCamera);
        
    }
    PController::~PController()
    {
    }
#if _EDITOR_ONLY
    PEditorController::PEditorController()
    {
        PController();
    }
    PEditorController::~PEditorController()
    {
    }

    void PEditorController::Init()
    {
        PController::Init();
        auto func = [this](IMouse::Event::EType Type)
        {
            this->OnMouse(Type);
        }; 
        OnMouseDown = func;
        
        EInput::MouseEvent.Add(OnMouseDown);

    }

    void PEditorController::EditorTick(FLOAT deltaTime)
    {
        PController::EditorTick(deltaTime);
        if(!bStart)
        {
            return;
        }

        // rotation
        {
            FLOAT RotationSpeed = 5.0f;
            Vector2 MousePosition = Vector2::Zero();
            MousePosition.x = static_cast<FLOAT>(EInput::Controller.GetMousePosition().first);
            MousePosition.y = static_cast<FLOAT>(EInput::Controller.GetMousePosition().second);
            const Vector2 DeltaMouse = MousePosition - MousePositionLastTick;
            Euler CurrentRotation = MakeEuler(this->GetActorRotation());
            CurrentRotation.Yaw    = EMath::FMod((CurrentRotation.Yaw   + DeltaMouse.x * RotationSpeed * deltaTime), 360.f);
            CurrentRotation.Pitch  = EMath::FMod((CurrentRotation.Pitch + DeltaMouse.y * RotationSpeed * deltaTime), 360.f);
            this->SetActorRotation(MakeQuaternion(CurrentRotation));
            MousePositionLastTick = MousePosition;
      
        }
        
        // position
        {
            FLOAT MovingSpeed = 50.0f;
            Vector3 MovingDirection = Vector3::Zero();
            const BOOL8 bWPressed = EInput::Controller.IsKeyPressed(0x57);
            const BOOL8 bAPressed = EInput::Controller.IsKeyPressed(0x41);
            const BOOL8 bSPressed = EInput::Controller.IsKeyPressed(0x53);
            const BOOL8 bDPressed = EInput::Controller.IsKeyPressed(0x44);
            const BOOL8 bQPressed = EInput::Controller.IsKeyPressed(0x51);
            const BOOL8 bEPressed = EInput::Controller.IsKeyPressed(0x45);
            
            MovingDirection.z = bWPressed ? (bSPressed ? 0.0f :  1.0f) : (bSPressed ? -1.0f : 0.0f);
            MovingDirection.x = bAPressed ? (bDPressed ? 0.0f : -1.0f) : (bDPressed ?  1.0f : 0.0f);
            MovingDirection.y = bQPressed ? (bEPressed ? 0.0f :  1.0f) : (bEPressed ? -1.0f : 0.0f);
            
            Vector3 CurrentLoc = this->GetActorLocation();
            this->SetActorLocation(CurrentLoc + (MovingDirection.x * this->GetActorRightVector() + MovingDirection.y * this->GetActorUpVector() + MovingDirection.z * this->GetActorForwardVector()) * MovingSpeed * deltaTime );
        }
    }

    void PEditorController::OnMouse(IMouse::Event::EType Type)
    {
        if(Type == IMouse::Event::EType::RPress)
        {
            MousePositionLastTick.x = static_cast<FLOAT>(EInput::Controller.GetMousePosition().first);
            MousePositionLastTick.y = static_cast<FLOAT>(EInput::Controller.GetMousePosition().second);
            //EInput::Controller.HideCursor();
            bStart = TRUE;
        }

        if(Type == IMouse::Event::EType::RRelease)
        {
            //EInput::Controller.ShowCursor();
            bStart = FALSE;
        }
    }


#endif
    
}
