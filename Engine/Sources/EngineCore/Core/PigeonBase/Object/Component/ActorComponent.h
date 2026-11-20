#pragma once

#include <CoreMinimal.h>

#include "Component.h"
#include <PigeonBase/Object/World/WorldTickScheduler.h>

namespace PigeonEngine
{

	class PActor;
	class FWorldTickScheduler;
	class PActorComponent : public PComponent
	{
		friend class FWorldTickScheduler;
	public:
		virtual void Init() override;
		virtual void Uninit() override;
		// Tick flags and priority update the world scheduler entry immediately.
		virtual void SetIsTickable(const BOOL32& bTickable) override;
		virtual void SetTickPriority(const INT32& InPriority) override;
	public:
		virtual BOOL32 ContainTransform() const;

		CLASS_VIRTUAL_NOCOPY_BODY(PActorComponent)

	public:
		PActor* GetOwnerActor() const;
		void    RemoveFromOwnerActor();
		void	OnAddedToScene(PWorld* World);
		void	OnRemovedFromScene();
		// FALSE keeps this component out of level files. Components whose construction is
		// owned by an actor initialization hook (movement, controller input) override this
		// rather than being recreated by the class factory with missing runtime links.
		PE_NODISCARD virtual BOOL32 ShouldSerializeToLevel() const { return TRUE; }
	//protected: //Phyiscs Test note : Time 23/11/5
		void    SetOwnerActor(PActor* NewOwner);
	private:
		PActor* Owner = nullptr;
		FTickHandle VariableTickHandle;
		FTickHandle FixedTickHandle;
		FTickHandle PostPhysicsTickHandle;
#if ENGINE_EDITOR_ONLY
		FTickHandle EditorTickHandle;
#endif

	protected:
		// Final destroy step, after Uninit: by now a subclass has finished every teardown
		// that needed the owner actor, so the back-pointer can go.
		virtual void OnPreDelete() override;

		// Scene hooks that also maintain the world's component registry: joining the
		// scene indexes this component by its unique ID, leaving removes it.
		void BeginAddedToScene(PWorld* World) override;
		void RemovedFromScene() override;

		// Post-physics dispatch. The base registers a scheduler entry in the owning world;
		// subclasses call this from BeginAddedToScene, unregister from RemovedFromScene, and
		// override HandlePostPhysicsTick.
		void RegisterPostPhysicsTick();
		void UnregisterPostPhysicsTick();
		virtual void HandlePostPhysicsTick(FLOAT deltaTime);
	private:
		void RegisterTickDomains();
		void UnregisterTickDomains();
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

	};
}
