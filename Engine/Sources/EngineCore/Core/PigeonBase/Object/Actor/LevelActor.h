#pragma once
#include "PigeonBase/Object/Actor.h"

namespace PigeonEngine
{
    class PLevelActor : public PActor
    {
		friend class PWorld;
        CLASS_VIRTUAL_NOCOPY_BODY(PLevelActor)

	protected:
		// for frame, 
		virtual void BeginAddedToScene(PWorld* World);
		virtual void RemovedFromScene();
    protected:

		virtual void UserBeginPlay();
		virtual void UserTick(FLOAT deltaTime);
		virtual void UserEndPlay();

    private:
        // No transform in level scriptor
  //       PE_NODISCARD Vector3    GetActorLocation() const override = delete;
  //       PE_NODISCARD Quaternion GetActorRotation() const override = delete;
  //       PE_NODISCARD Vector3    GetActorScale()    const override = delete;
		//
  //       PE_NODISCARD const ETransform& GetActorTransform()      const override = delete;
  //       PE_NODISCARD ETransform        GetActorWorldTransform() const override = delete;
  //
  //       void SetActorLocation (const Vector3& Location) override = delete;
  //       void SetActorRotation (const Quaternion& Rotation) override = delete;
  //       void SetActorScale    (const Vector3& Scale) override = delete;
		//
  //       PE_NODISCARD Vector3 GetActorForwardVector()const override = delete;
  //       PE_NODISCARD Vector3 GetActorRightVector()  const override = delete;
  //       PE_NODISCARD Vector3 GetActorUpVector()     const override = delete;
  //
  //       PE_NODISCARD EMobilityType GetMobility() const override = delete;
  //
  //
  //       // No attachment in level scriptor
		// PE_NODISCARD PActor* GetAttachedParentActor() const override = delete;
		// void DetachFromParentActor() override = delete;
		// void AttachToActor(PActor* Another, const ETransform& RelativeTransform = ETransform()) override = delete;
		// void AttachActorTo(PActor* Another, const ETransform& RelativeTransform = ETransform()) override = delete;
		// static void AttachActorToActor(PActor* Child, PActor* Parent, const ETransform& RelativeTransform = ETransform()) = delete;
  //
		// void AddComponent(PActorComponent* NewComponent, const ETransform& RelativeTransform = ETransform()) override = delete;
		// void DestoyComponent(PActorComponent* Component) override = delete;
		// void ClearComponents() override = delete;
		// static void AttachComponentToActor(PSceneComponent* Component, PActor* Actor, const ETransform& RelativeTransform = ETransform()) = delete;
    };

}

