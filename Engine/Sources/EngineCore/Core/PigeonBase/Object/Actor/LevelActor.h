#pragma once
#include "PigeonBase/Object/Actor.h"

namespace PigeonEngine
{
    class PLevelActor : public PActor
    {
        CLASS_VIRTUAL_NOCOPY_BODY(PLevelActor)


    protected:

		virtual void UserBeginPlay();
		virtual void UserTick(FLOAT deltaTime);
		virtual void UserEndPlay();

    private:
        // No transform in level scriptor
        PE_NODISCARD Vector3    GetActorLocation() const = delete;
        PE_NODISCARD Quaternion GetActorRotation() const = delete;
        PE_NODISCARD Vector3    GetActorScale()    const = delete;
		
        PE_NODISCARD const ETransform& GetActorTransform()      const   = delete;
        PE_NODISCARD ETransform        GetActorWorldTransform() const   = delete;

        void SetActorLocation (const Vector3& Location)    = delete;
        void SetActorRotation (const Quaternion& Rotation) = delete;
        void SetActorScale    (const Vector3& Scale)       = delete;
		
        PE_NODISCARD Vector3 GetActorForwardVector()const = delete;
        PE_NODISCARD Vector3 GetActorRightVector()  const = delete;
        PE_NODISCARD Vector3 GetActorUpVector()     const = delete;

        PE_NODISCARD EMobilityType GetMobility() const = delete;


        // No attachment in level scriptor
		PE_NODISCARD PActor* GetAttachedParentActor() const = delete;
		void DetachFromParentActor() = delete;
		void AttachToActor(PActor* Another, const ETransform& RelativeTransform = ETransform()) = delete;
		void AttachActorTo(PActor* Another, const ETransform& RelativeTransform = ETransform()) = delete;
		static void AttachActorToActor(PActor* Child, PActor* Parent, const ETransform& RelativeTransform = ETransform()) = delete;

		void AddComponent(PActorComponent* NewComponent, const ETransform& RelativeTransform = ETransform()) = delete;
		void DestoyComponent(PActorComponent* Component) = delete;
		void ClearComponents() = delete;
		static void AttachComponentToActor(PSceneComponent* Component, PActor* Actor, const ETransform& RelativeTransform = ETransform()) = delete;
    };

}

