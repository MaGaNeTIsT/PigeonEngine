#pragma once

#include <CoreMinimal.h>
#include "Base/DataStructure/Container/Set.h"
#include "Object.h"
#include "EngineCommon.h"


namespace PigeonEngine
{
	class PSceneComponent;
	class PActorComponent;
	class PWorld;
	class PActor : public PObject
	{
		friend class PWorld;

		CLASS_VIRTUAL_NOCOPY_BODY(PActor)

	public:
		void    Init() override;
		void    Uninit() override;
		void	Tick(FLOAT deltaTime) override;
		void	FixedTick(FLOAT deltaTime);
		// Render update pass, independent of tickability: every scene component's deferred
		// applies, dirty sends and transform walks run here once per frame, including the
		// subtrees of non-tickable actors.
		void	TickRenderTree();
#if ENGINE_EDITOR_ONLY
		void	EditorTick(FLOAT deltaTime) override;
#endif

	protected:
		// for frame, 
		void BeginAddedToScene(PWorld* World) override;
		void RemovedFromScene() override;

		// Structural teardown, after RemovedFromScene (so UserEndPlay has seen the whole
		// object): leave the parent actor, destroy attached actors, destroy components.
		virtual void OnDestroy() override;

		// for users
		virtual void UserBeginPlay();
		virtual void UserTick(FLOAT deltaTime);
		virtual void UserEndPlay();

	public:
		PE_NODISCARD PSceneComponent* GetRootComponent() const;
		void                          SetRootComponent(PSceneComponent* NewRoot);
		// Loader path: install a root component that already carries its serialized local
		// transform. Same as SetRootComponent except the old root's transform is not
		// carried over onto the new root. Only valid before the actor joins a world.
		void                          ReplaceRootComponentForLoad(PSceneComponent* NewRoot);
		// Every component in insertion order, including the root (serializer/editor
		// enumeration; the returned array is a copy).
		PE_NODISCARD TArray<PActorComponent*> GetAllComponents() const;
		// FALSE keeps this actor out of level files. The world owns its controller, and a
		// character's physics setup is created by InitCharacter rather than by the factory,
		// so both are out of the level format until their init hooks are serializable.
		PE_NODISCARD virtual BOOL32    ShouldSerializeToLevel() const;

		PE_NODISCARD virtual Vector3    GetActorLocation() const;
		PE_NODISCARD virtual Quaternion GetActorRotation() const;
		PE_NODISCARD virtual Vector3    GetActorScale() const;

		PE_NODISCARD virtual Vector3    GetActorWorldLocation() const;
		PE_NODISCARD virtual Quaternion GetActorWorldRotation() const;
		PE_NODISCARD virtual Vector3    GetActorWorldScale() const;

		PE_NODISCARD virtual const ETransform& GetActorTransform() const;
		PE_NODISCARD virtual ETransform GetActorWorldTransform() const;

		virtual void SetActorLocation (const Vector3& Location);
		virtual void SetActorRotation (const Quaternion& Rotation);
		virtual void SetActorScale    (const Vector3& Scale);

		PE_NODISCARD virtual Vector3 GetActorForwardVector() const;
		PE_NODISCARD virtual Vector3 GetActorRightVector() const;
		PE_NODISCARD virtual Vector3 GetActorUpVector() const;

		PE_NODISCARD virtual EMobilityType GetMobility() const;

	private:
		// a root component is a root component.
		PSceneComponent* RootComponent = nullptr;

	public:
		virtual void AddComponent     (PActorComponent* NewComponent, const ETransform& RelativeTransform = ETransform());
		PE_NODISCARD const PActorComponent* GetComponentByUniqueID(const ObjectIdentityType& UniqueID, const BOOL8& bIncludeChildComponent) const ;
		void         DestroyComponent (PActorComponent* Component);
		// Deprecated misspelling of DestroyComponent, kept for existing callers.
		void         DestoyComponent  (PActorComponent* Component) { DestroyComponent(Component); }
		// Called from a component's final destroy step (OnPreDelete): the actor drops the
		// reference so its list never keeps a pointer to freed memory. Public because the
		// component side of the teardown lives in PActorComponent, which is not a friend.
		void         RemoveComponentReference(PActorComponent* Component);
		// Components of this actor in insertion order. FindComponent returns the first
		// component that is a TComponent, subclasses included; GetComponents returns all.
		template <typename TComponent>
		PE_NODISCARD TArray<TComponent*> GetComponents() const;
		template <typename TComponent>
		PE_NODISCARD TComponent* FindComponent() const;
protected:
	virtual void ClearComponents  ();
protected:
		static void AttachComponentToActor(PSceneComponent* Component, PActor* Actor, const ETransform& RelativeTransform = ETransform());
	private:
		// contains all actor components, including RootComponent. Insertion order, so
		// tick and iteration order is deterministic (the old TSet iterated in pointer
		// order, which changed with the allocator).
		TArray<PActorComponent*> Components;
		// Components sorted by (tick group, priority), rebuilt whenever the list changes.
		// Registration order is the tie-breaker, so the order is stable across runs.
		TArray<PActorComponent*> TickOrder;
	public:
		// Re-sorts the cached tick order after a component changed its tick group or
		// priority. Called by PActorComponent::SetTickGroup/SetTickPriority.
		void RefreshTickOrder();
	private:
		void RebuildTickOrder();

	public:

		PE_NODISCARD EBoundAABB GetBounds() const;

	public:

		PE_NODISCARD virtual PActor* GetAttachedParentActor() const;

		virtual void DetachFromParentActor();
		// Attach this actor to another;
		virtual void AttachToActor(PActor* Parent, const ETransform& RelativeTransform = ETransform());
		virtual void AttachToActor(PActor* Parent, EAttachmentRuleType InRule, const ETransform& RelativeTransform = ETransform());
		// Attach another actor to this;
		virtual void AttachActorTo(PActor* Child, const ETransform& RelativeTransform = ETransform());
		virtual void AttachActorTo(PActor* Child, EAttachmentRuleType InRule, const ETransform& RelativeTransform = ETransform());
		static void AttachActorToActor(PActor* Child, PActor* Parent, const ETransform& RelativeTransform = ETransform());
		static void AttachActorToActor(PActor* Child, PActor* Parent, EAttachmentRuleType InRule, const ETransform& RelativeTransform = ETransform());

		PE_NODISCARD TArray<PActor*> GetAllActorsAttached(BOOL8 bRecursive = FALSE) const;

		PE_NODISCARD const PActor* GetActorByUniqueID(const ObjectIdentityType& UniqueID, const BOOL8& bIncludeChildActor) const ;
		void DetachActorsAttached();
		void DestroyActorsAttached();
	protected:

		PActor* AttachedParentActor = nullptr;
		TArray<PActor*> ChildrenActors;

		// Imgui
#if ENGINE_EDITOR_ONLY
	public:
		virtual void GenerateWorldOutline(const PActor* WorldCurrentSelectedActor);
		virtual void GenerateDetails(const PActorComponent* WorldCurrentSelectedComponent);
#endif

	};

	template <typename TComponent>
	TArray<TComponent*> PActor::GetComponents() const
	{
		TArray<TComponent*> Result;
		for (UINT32 Index = 0u, Num = Components.Num<UINT32>(); Index < Num; Index++)
		{
			if (TComponent* CastComponent = dynamic_cast<TComponent*>(Components[Index]))
			{
				Result.Add(CastComponent);
			}
		}
		return Result;
	}

	template <typename TComponent>
	TComponent* PActor::FindComponent() const
	{
		for (UINT32 Index = 0u, Num = Components.Num<UINT32>(); Index < Num; Index++)
		{
			if (TComponent* CastComponent = dynamic_cast<TComponent*>(Components[Index]))
			{
				return CastComponent;
			}
		}
		return nullptr;
	}

};
