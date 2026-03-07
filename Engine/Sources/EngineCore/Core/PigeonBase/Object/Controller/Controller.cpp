#include "Controller.h"

#include "../../../../Editor/EditorLogManager.h"
#include "../../../../HID/Input/InputType.h"
#include "PigeonBase/Object/Component/CameraAndLight/CameraComponent.h"

#if _EDITOR_ONLY
#include <imgui.h>
#endif

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
        auto func = [this](IMouse::Event::EType Type)
        {
            this->OnMouse(Type);
        }; 
        OnMouseDown = func;

        auto func1 =  [this](IKeyboard::Event::EType Type, BYTE KeyCode)
        {
            this->OnKey(Type, KeyCode);
        }; 
        OnKeyDown = func1;
        
        EInput::MouseEvent.Add(OnMouseDown);
        EInput::KeyEvent.Add(OnKeyDown);
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

    void PController::OnMouse(IMouse::Event::EType Type)
    {
        Vector2 MousePosition = Vector2();
        MousePosition.x = static_cast<FLOAT>(EInput::Controller.GetMousePosition().first);
        MousePosition.y = static_cast<FLOAT>(EInput::Controller.GetMousePosition().second);
        this->OnMouseEvent.Broadcast(Type, MousePosition);
    }

    void PController::OnKey(IKeyboard::Event::EType Type, BYTE KeyCode)
    {
        this->OnKeyEvent.Broadcast(Type, EKeysBuiltIn::GetKeyByKeyCode(KeyCode));
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
        MyCamera = new PCameraComponent(0.f, 0.f, EEngineSettings::ENGINE_SCREEN_WIDTH, EEngineSettings::ENGINE_SCREEN_HEIGHT);
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
    }

    void PEditorController::EditorTick(FLOAT deltaTime)
    {
        PController::EditorTick(deltaTime);

        DrawImGui();
        if(!bStart)
        {
            return;
        }
        
        // rotation
        {
            Vector2 MousePosition = Vector2::Zero();
            MousePosition.x = static_cast<FLOAT>(EInput::Controller.GetMousePosition().first);
            MousePosition.y = static_cast<FLOAT>(EInput::Controller.GetMousePosition().second);
            
            const Vector2 DeltaMouse = MousePosition - MousePositionLastTick;
            
            Quaternion CurrentRotation = this->GetActorRotation();
            
            const Vector3 WorldUpDir    = Vector3(0.f, 1.f, 0.f);
            const Vector3 ActorRightDir = this->GetActorRightVector();
            Quaternion A = MakeQuaternion(ActorRightDir, DeltaMouse.y * RotationSpeed * deltaTime);
            Quaternion B = MakeQuaternion(WorldUpDir,    DeltaMouse.x * RotationSpeed * deltaTime);
            Quaternion Final = Quaternion::MultiplyQuaternion(CurrentRotation, A);
            Final = Quaternion::MultiplyQuaternion(Final, B);
            this->SetActorRotation(Final);
            MousePositionLastTick = MousePosition;
        }
        
        
        // position
        {
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
        PController::OnMouse(Type);
        if(Type == IMouse::Event::EType::RPress)
        {
            MousePositionLastTick = Vector2();
            MousePositionLastTick.x = static_cast<FLOAT>(EInput::Controller.GetMousePosition().first);
            MousePositionLastTick.y = static_cast<FLOAT>(EInput::Controller.GetMousePosition().second);
            bStart = TRUE;
        }

        if(Type == IMouse::Event::EType::RRelease)
        {
            bStart = FALSE;
        }
        if(!bStart)
        {
            return;
        }
        if(Type == IMouse::Event::EType::WheelUp)
        {
            this->MovingSpeed += 10.0f;
        }
        if(Type == IMouse::Event::EType::WheelDown)
        {
            this->MovingSpeed += -10.0f;
        }
        
    }

    void PEditorController::OnKey(IKeyboard::Event::EType Type, BYTE KeyCode)
    {
       //  EEditorLogManager* LogsManager = EEditorLogManager::GetManagerSingleton();
        PController::OnKey(Type, KeyCode);
        EString TestLog;
        switch (Type)
        {
        case IKeyboard::Event::EType::Press:
            {
                TestLog += "Press ";
                break;
            }
        case IKeyboard::Event::EType::Release:
            {
                TestLog += "Release ";
                break;
            }
        }

        
    }

    void PEditorController::DrawImGui()
    {
        ImGui::Begin("EditorController", FALSE, ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar);
        ImGui::Text("Editor Controller Setup");
        /*const BOOL8 bChangedX = */ImGui::DragScalar("##MovingSpeed", ImGuiDataType_Float, &this->MovingSpeed, 0.05f, NULL, NULL, "MovingSpeed : %.6f");
        /*const BOOL8 bChangedY = */ImGui::DragScalar("##RotationSpeed", ImGuiDataType_Float, &this->RotationSpeed, 0.001f, NULL, NULL, "RotationSpeed : %.6f");
        ImGui::End();
    }


#endif
    
}
