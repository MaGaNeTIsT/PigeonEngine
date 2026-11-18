#pragma once

#include <CoreMinimal.h>

#include "Component.h"

namespace PigeonEngine
{

	class PActor;
	class PActorComponent : public PComponent
	{
	public:
		virtual void Init() override;
		virtual void Uninit() override;
		// Setting either re-sorts the owner actor's cached tick order, so changing them
		// after AddComponent takes effect immediately.
		virtual void SetTickGroup(const ETickGroupType& InGroup) override;
		virtual void SetTickPriority(const INT32& InPriority) override;
	public:
		virtual BOOL32 ContainTransform() const;

		CLASS_VIRTUAL_NOCOPY_BODY(PActorComponent)

	public:
		PActor* GetOwnerActor() const;
		void    RemoveFromOwnerActor();
		void	OnAddedToScene(PWorld* World);
		void	OnRemovedFromScene();
	//protected: //Phyiscs Test note : Time 23/11/5
		void    SetOwnerActor(PActor* NewOwner);
	private:
		PActor* Owner = nullptr;

	protected:
		// Final destroy step, after Uninit: by now a subclass has finished every teardown
		// that needed the owner actor, so the back-pointer can go.
		virtual void OnPreDelete() override;

		// Scene hooks that also maintain the world's component registry: joining the
		// scene indexes this component by its unique ID, leaving removes it.
		void BeginAddedToScene(PWorld* World) override;
		void RemovedFromScene() override;

		// Post-physics tick. The base registers one handler on EWorldTickManager that
		// re-resolves this component by ID every fixed tick - the pattern the physics,
		// movement and trigger components each used to copy - and dispatches to the
		// virtual hook. Subclasses register from BeginAddedToScene, unregister from
		// RemovedFromScene, and override HandlePostPhysicsTick.
		void RegisterPostPhysicsTick();
		void UnregisterPostPhysicsTick();
		virtual void HandlePostPhysicsTick(FLOAT deltaTime);
	private:
		TFunction<void(FLOAT)> PostPhysicsTickHandler;
		BOOL32 bPostPhysicsTickRegistered = FALSE;

		// Render proxy functions START
	public:
		virtual BOOL32	IsVisible() const;
		BOOL32			ShouldRender() const;
		BOOL32			IsRenderStateDirty() const;
		virtual void	CreateRenderState();
		virtual void	DestroyRenderState();
		virtual void	SendUpdateRenderState();
	protected:
		virtual void	MarkRenderStateAsDirty();
		virtual void	CleanMarkRenderStateDirty();
	protected:
		BOOL32	RenderStateDirty = FALSE;
		// Render proxy functions END

#if ENGINE_EDITOR_ONLY
	public:
		virtual void GenerateComponentOutline(const PActorComponent* WorldCurrentSelectedComponent);
		virtual void GenerateComponentDetail();
		virtual void OnSelectedByImGui();
#endif

	};
}
