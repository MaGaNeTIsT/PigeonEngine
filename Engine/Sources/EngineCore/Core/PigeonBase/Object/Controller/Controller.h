#pragma once
#include "PigeonBase/Object/Actor.h"
namespace PigeonEngine
{
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
        void OnMouse(IMouse::Event::EType Type);
    private:
        TFunction<void(IMouse::Event::EType Type)> OnMouseDown;
        BOOL8 bStart = false;
        
        Vector2 MousePositionLastTick = Vector2::Zero();
    };
#endif
    
}

