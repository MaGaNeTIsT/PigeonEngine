#pragma once

#include <set>

#include <CoreMinimal.h>
#include "./Object.h"
#include "EngineCommon.h"
#include "Base/DataStructure/Container/Set.h"


namespace PigeonEngine
{
	class PSceneComponent;
	class PActorComponent;
	class PActor : public PObject
	{
	
	private:

		CLASS_VIRTUAL_NOCOPY_BODY(PActor)
		
	public:
		void    Init() override;
		void    Uninit() override;
		void	Tick(FLOAT deltaTime) override;
		void	FixedTick(FLOAT deltaTime);
#if _EDITOR_ONLY
		void	EditorTick(FLOAT deltaTime) override;
#endif
		void    Destroy() override;
	public:
		 
		PActor* GetAttachedParentActor() const;

		// Attach this actor to another;
		void AttachToActor(PActor* Another, const ETransform& RelativeTransform = ETransform());
		// Attach another actor to this;
		void AttachActorTo(PActor* Another, const ETransform& RelativeTransform = ETransform());
		static void AttachActorToActor(PActor* Actor, PActor* AttachTo, const ETransform& RelativeTransform = ETransform());
		
	private:

		PActor* AttachedParentActor = nullptr;
		
	public:
		PSceneComponent* GetRootComponent() const;
		void             SetRootComponent(PSceneComponent* NewRoot);

		Vector3    GetActorLocation() const;
		Quaternion GetActorRotation() const;
		Vector3    GetActorScale() const;

		Vector3 GetActorForwardVector()const;
		Vector3 GetActorRightVector()const;
		Vector3 GetActorUpVector()const;

		EMobilityType GetMobility() const;

	private:
		// a root component is a root component.
		PSceneComponent* RootComponent = nullptr;

	public:
		
		void        AttachComponent(PSceneComponent* Component, const ETransform& RelativeTransform = ETransform());
		static void AttachComponentToActor(PSceneComponent* Component, PActor* Actor, const ETransform& RelativeTransform = ETransform());

		// Add a component, will not care about the transform
		void AddComponent     (PActorComponent* NewComponent);
		void DestoyComponent  (PActorComponent* Component);
		void ClearComponents  ();
	private:
		TSet<PActorComponent*> Components;

	public:

		EBoundAABB GetComponentsBoundingBox();
		
	};

};