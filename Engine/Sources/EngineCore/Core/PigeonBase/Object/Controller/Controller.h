#pragma once
#include "Input/InputType.h"
#include "PigeonBase/Object/Actor.h"
namespace PigeonEngine
{
    MAKE_DELEGATE_MULTI_TWO_PARAM(OnMouseEvent, IMouse::Event::EType,  const Vector2&);
    MAKE_DELEGATE_MULTI_TWO_PARAM(OnKeyEvent,   IKeyboard::Event::EType, const EKey&);
    class PCameraComponent;
    class PController : public PActor
    {
        friend class PWorld;
    protected:
        // for frame, 
        void BeginAddedToScene(PWorld* World) override;
        void RemovedFromScene() override;

        void Init() override;
        void Uninit() override;
        
        // for users
        void UserBeginPlay() override;
        void UserTick(FLOAT deltaTime) override;
        void UserEndPlay() override;
    protected:
        virtual void OnMouse(IMouse::Event::EType Type) ;
        virtual void OnKey(IKeyboard::Event::EType Type, unsigned char KeyCode) ;
    public:
        OnMouseEvent OnMouseEvent;
        OnKeyEvent   OnKeyEvent;
    private:
        TFunction<void(IMouse::Event::EType Type)> OnMouseDown;
        TFunction<void(IKeyboard::Event::EType Type, unsigned char KeyCode)> OnKeyDown;
    public:
        
        PE_NODISCARD PCameraComponent* GetCamera() const;
        void SetCamera(PCameraComponent* NewCam);
        void ResetToDefaultCamera();
        
    private:
        PCameraComponent* MyCamera = nullptr;

    public:
        void SetActorLocation(const Vector3& Location) override;
        void SetActorRotation(const Quaternion& Rotation) override;
        void SetActorScale(const Vector3& Scale) override;
    public:
        CLASS_VIRTUAL_NOCOPY_BODY(PController)
    };

#if _EDITOR_ONLY

    
    class PEditorController : public PController
    {
    public:
        CLASS_VIRTUAL_NOCOPY_BODY(PEditorController)
        void Init() override;
        void EditorTick(FLOAT deltaTime) override;
        void DrawImGui();
    protected:
        void OnMouse(IMouse::Event::EType Type) override;
        void OnKey(IKeyboard::Event::EType Type, unsigned char KeyCode) override;

    private:
        
        BOOL8 bStart = false;
        FLOAT MovingSpeed = 50.0;
        FLOAT RotationSpeed = 0.05f;
        Vector2 MousePositionLastTick = Vector2::Zero();
    };
#endif
    
}

