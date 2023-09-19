﻿#pragma once
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


    private:
        // No transform in level scriptor
        //PE_NODISCARD Vector3    GetActorLocation() const override = delete;
        //PE_NODISCARD Quaternion GetActorRotation() const override = delete;
        //PE_NODISCARD Vector3    GetActorScale()    const override = delete;
		
        //PE_NODISCARD const ETransform& GetActorTransform()      const override = delete;
        //PE_NODISCARD ETransform        GetActorWorldTransform() const override = delete;

        //void SetActorLocation (const Vector3& Location) override = delete;
        //void SetActorRotation (const Quaternion& Rotation) override = delete;
        //void SetActorScale    (const Vector3& Scale) override = delete;
		
        //PE_NODISCARD Vector3 GetActorForwardVector()const override = delete;
        //PE_NODISCARD Vector3 GetActorRightVector()  const override = delete;
        //PE_NODISCARD Vector3 GetActorUpVector()     const override = delete;

        //PE_NODISCARD EMobilityType GetMobility() const override = delete;


        //// No attachment in level scriptor
        //PE_NODISCARD PActor* GetAttachedParentActor() const override = delete;
        //void DetachFromParentActor() override = delete;
        //void AttachToActor(PActor* Another, const ETransform& RelativeTransform = ETransform()) override = delete;
        //void AttachActorTo(PActor* Another, const ETransform& RelativeTransform = ETransform()) override = delete;
        //static void AttachActorToActor(PActor* Child, PActor* Parent, const ETransform& RelativeTransform = ETransform()) = delete;

        //void AddComponent(PActorComponent* NewComponent, const ETransform& RelativeTransform = ETransform()) override = delete;
        //void DestoyComponent(PActorComponent* Component) override = delete;
        //void ClearComponents() override = delete;
        //static void AttachComponentToActor(PSceneComponent* Component, PActor* Actor, const ETransform& RelativeTransform = ETransform()) = delete;
    public:
        CLASS_VIRTUAL_NOCOPY_BODY(PController)
    };
}

