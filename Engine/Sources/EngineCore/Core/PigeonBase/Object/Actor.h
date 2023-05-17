#pragma once

#include <set>

#include <CoreMinimal.h>
#include "./Object.h"



namespace PigeonEngine
{
	class PSceneComponent;
	class PActorComponent;
	class PActor : public PObject
	{

		EClass(PActor, PObject)

	public:
		virtual void    Init() override;
		virtual void	FixedTick(FLOAT deltaTime);
	private:

		CLASS_VIRTUAL_NOCOPY_BODY(PActor)
		
	public:
		
		PActor* GetAttachedParentActor() const;
		
		void AttachToActor(PActor* AttachTo, const ETransform& RelativeTransform = ETransform());
		void AttachActorTo(PActor* Actor, const ETransform& RelativeTransform = ETransform());
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

	private:
		// a root component is a root component.
		PSceneComponent* RootComponent = nullptr;

	public:
		// WIP
		// 1 attach rules
		// 2 relative transform
		void        AttachComponent(PSceneComponent* Component, const ETransform& RelativeTransform = ETransform());
		static void AttachComponentToActor(PSceneComponent* Component, PActor* Actor, const ETransform& RelativeTransform = ETransform());

		// if NewComponent is a PSceneComponent, please DO NOT call this directly
		void AddComponent     (PActorComponent* NewComponent);
	private:
		std::set<PActorComponent*> Components;


		
	};

};