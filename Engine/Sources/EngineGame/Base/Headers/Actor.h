#pragma once

#include <set>

#include "../../../EngineCore/Core/Headers/Main.h"
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
		// WIP
		// 1 attach rules
		// 2 relative transform
		void AttachToActor(PActor* AttachTo);
		void AttachActorTo(PActor* Actor);
		static void AttachActorToActor(PActor* Actor, PActor* AttachTo);
		
	private:

		PActor* AttachedParentActor = nullptr;
		
	public:
		PSceneComponent* GetRootComponent() const;
		void             SetRootComponent(PSceneComponent* NewRoot);
		
	private:
		// a root component is a root component.
		PSceneComponent* RootComponent = nullptr;

	public:
		// WIP
		// 1 attach rules
		// 2 relative transform
		void AttachComponent(PSceneComponent* Component);
		static void AttachComponentToActor(PSceneComponent* Component, PActor* Actor);

		// if NewComponent is a PSceneComponent, please DO NOT call this directly
		void AddComponent     (PActorComponent* NewComponent);
	private:
		std::set<PActorComponent*> Components;


		
	};

};