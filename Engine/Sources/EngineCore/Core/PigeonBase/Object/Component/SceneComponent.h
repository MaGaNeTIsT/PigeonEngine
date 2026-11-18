#pragma once
#include <CoreMinimal.h>
#include <EngineCommon.h>
#include "ActorComponent.h"
#include "Base/DataStructure/Container/Set.h"

namespace PigeonEngine
{

	class PWorld;
	class PActor;
	// Render proxy functions START
	struct ERenderTransformInfo
	{
	public:
		Vector3     WorldLocation;
		Quaternion  WorldRotation;
		Vector3     WorldScaling;
	public:
		ERenderTransformInfo(const Vector3& InWorldLocation, const Quaternion& InWorldRotation, const Vector3& InWorldScaling)
			: WorldLocation(InWorldLocation), WorldRotation(InWorldRotation), WorldScaling(InWorldScaling)
		{
		}
		ERenderTransformInfo() = default;
		ERenderTransformInfo(const ERenderTransformInfo& Other)
			: WorldLocation(Other.WorldLocation), WorldRotation(Other.WorldRotation), WorldScaling(Other.WorldScaling)
		{
		}
		void CopyFromOther(const ERenderTransformInfo& Other)
		{
			WorldLocation = Other.WorldLocation;
			WorldRotation = Other.WorldRotation;
			WorldScaling = Other.WorldScaling;
		}
		ERenderTransformInfo& operator=(const ERenderTransformInfo& Other)
		{
			CopyFromOther(Other);
			return (*this);
		}
	};
	// Render proxy functions END

	class PSceneComponent : public PActorComponent
	{
		CLASS_VIRTUAL_NOCOPY_BODY(PSceneComponent)
			friend class PWorld;
		friend class PActor;
	public:
		virtual void Init() override;
		virtual void Uninit() override;
		void    Tick(FLOAT deltaTime) override;
		void	FixedTick(FLOAT deltaTime) override;
#if ENGINE_EDITOR_ONLY
		void	EditorTick(FLOAT deltaTime) override;
#endif

	public:
		/*
		* Non-virtual entry point. A subclass overrides TickRenderImpl instead, and that is deliberate:
		* the deferred applies below have to run for every component before anything else touches its
		* render state, and an override that simply replaced TickRender would silently skip them (two
		* subclasses already did exactly that before this split).
		*/
		void TickRender();
		/*
		* Game thread: records a step that runs every frame until it reports done (InStep returns TRUE
		* once it has applied what it waited for). This is the safe form of "react when this asset
		* exists": the step lives with this component, so a component that is destroyed simply stops
		* running it. Nothing was queued, so nothing can outlive its target - which is exactly the trap
		* a command in EGameThreadCommandQueue carries.
		*
		* TickRender drains these, and every actor runs TickRender over its component tree every frame
		* before its own tickable check, so even a component of a non-tickable actor resolves.
		*/
		void	AddDeferredApply(TFunction<BOOL32()> InStep);
		PE_NODISCARD BOOL32	HasDeferredApply() const { return ((DeferredApplies.Num<UINT32>() > 0u) ? TRUE : FALSE); }
	protected:
		// What a subclass overrides: everything a component does per frame except the deferred applies,
		// which TickRender runs before calling this. An override that does not chain up keeps that
		// behavior, which is what the two existing overrides rely on.
		virtual void	TickRenderImpl();
	private:
		// Retried by TickRender until each step reports done, then dropped. See AddDeferredApply.
		TArray<TFunction<BOOL32()>>	DeferredApplies;
		void	RunDeferredApplies();
	protected:
		void BeginAddedToScene(PWorld* World) override;
		void RemovedFromScene() override;

	protected:
		// Scene-component teardown: leave the parent component, destroy the children.
		// Runs as this component's destroy hook; the render state is released earlier,
		// at scene exit, while the world pointer is still set.
		virtual void OnDestroy() override;

	public:
		PE_NODISCARD  EMobilityType GetMobility() const;
		void                        SetMobility(const EMobilityType& NewMobility);
	private:
		EMobilityType Mobility = EMobilityType::EMT_DYNAMIC;
	public:
		PE_NODISCARD BOOL32 ContainTransform() const override;

		PE_NODISCARD Vector3    GetComponentLocalLocation() const;
		PE_NODISCARD Quaternion GetComponentLocalRotation() const;
		PE_NODISCARD Vector3    GetComponentLocalScale() const;

		PE_NODISCARD Vector3    GetComponentWorldLocation() const;
		PE_NODISCARD Quaternion GetComponentWorldRotation() const;
		PE_NODISCARD Vector3    GetComponentWorldScale() const;

		PE_NODISCARD Vector3 GetComponentForwardVector() const;
		PE_NODISCARD Vector3 GetComponentRightVector() const;
		PE_NODISCARD Vector3 GetComponentUpVector() const;

		PE_NODISCARD const ETransform& GetTransform() const;
		PE_NODISCARD ETransform        GetWorldTransform() const;

		// World-space cache, composed from the component parent chain only. The actor
		// hierarchy no longer participates: a child actor's root component is already
		// parented to its parent actor's root, so the component chain covers it exactly
		// once. World = Local * ParentWorld under the row-vector convention; recomputed
		// lazily after this component or any ancestor changed its local transform.
		PE_NODISCARD const Matrix4x4&   GetCachedLocalToWorldMatrix() const;
		PE_NODISCARD const ETransform&  GetCachedWorldTransform() const;
		// Invalidates this component's cached world transform and every descendant's.
		void MarkWorldTransformDirty();


		void SetComponentLocation(const Vector3& Location);
		void SetComponentRotation(const Quaternion& Rotation);
		void SetComponentScale(const Vector3& Scale);
		void SetComponentTransform(const ETransform& Trans);

		void SetComponentWorldLocation(const Vector3& Location);
		void SetComponentWorldRotation(const Quaternion& Rotation);
		void SetComponentWorldScale(const Vector3& Scale);
		void SetComponentWorldTransform(const ETransform& Trans);

	protected:

		ETransform	Transform;

	private:
		// Lazy world cache; mutable because the const getters compute it on demand.
		mutable Matrix4x4	CachedLocalToWorld = Matrix4x4::Identity();
		mutable ETransform	CachedWorldTransform;
		mutable BOOL32		bWorldTransformDirty = TRUE;
		void UpdateWorldTransformCache() const;
		// Rewrites the local transform so this component keeps InWorldTransform.
		void SetWorldTransformPreservingParent(const ETransform& InWorldTransform);
	protected:
		// Marks this component and every descendant render-transform-dirty. A transform
		// change at any level affects every descendant's world matrix and render state.
		void MarkTransformDirtyRecursive();

	public:

		// Attach this component to another component
		void AttachToComponent(PSceneComponent* Parent, const ETransform& RelativeTransform = ETransform());
		void AttachToComponent(PSceneComponent* Parent, EAttachmentRuleType InRule, const ETransform& RelativeTransform = ETransform());
		// static function to attach
		static void AttachComponentToComponent(PSceneComponent* Child, PSceneComponent* Parent, const ETransform& RelativeTransform = ETransform());
		static void AttachComponentToComponent(PSceneComponent* Child, PSceneComponent* Parent, EAttachmentRuleType InRule, const ETransform& RelativeTransform = ETransform());
		// Get this component's attached parent component.
		PE_NODISCARD PSceneComponent* GetParentComponent() const;

	protected:
		// 
		void DetachFromParentComponent();

	private:
		PSceneComponent* ParentComponent = nullptr;

	public:
		void RemoveChildComponent(PSceneComponent* NewChild);
		PE_NODISCARD const PSceneComponent* GetComponentByUniqueID(const ObjectIdentityType& InUniqueID, const BOOL8& bIncludeChildComponent) const;
		PE_NODISCARD TArray<PSceneComponent*> GetChildrenComponents() const;
	protected:
		void ClearChildren();
	protected:
		// Insertion order, so transform and render walks are deterministic.
		TArray<PSceneComponent*> ChildrenComponents;

	public:
		virtual TSharedPtr<CJsonObject> Serialize() override;

	public:
		// children components wil NOT be involved
		PE_NODISCARD const EBoundAABB&  GetLocalBound() const;
		void                            SetLocalBound(const EBoundAABB& InNewBound);
		// children components WILL be involved
		PE_NODISCARD const EBoundAABB  GetComponentBound() const;

	protected:
		EBoundAABB  LocalBound = EBoundAABB(Vector3(0.f,0.f,0.f), Vector3(0.f,0.f,0.f), TRUE);

		// Render proxy functions START
	public:
		BOOL32          IsRenderTransformDirty() const;
		virtual void	CreateRenderState() override;
		virtual void	DestroyRenderState() override;
		virtual void	SendUpdateRenderState() override;
	protected:
		virtual void    MarkRenderTransformAsDirty();
		virtual void	MarkRenderStateAsDirty() override;
		virtual void	CleanMarkRenderStateDirty() override;
	protected:
		BOOL32          RenderTransformDirty = FALSE;
		// Set when CreateRenderState ran through the scene-component base, so teardown
		// only calls DestroyRenderState for a state that was actually created.
		BOOL32          bRenderStateCreated = FALSE;
		// Render proxy functions END

		// ImGui
#if ENGINE_EDITOR_ONLY
	public:
		virtual void GenerateComponentOutline(const PActorComponent* WorldCurrentSelectedComponent);
		virtual void GenerateComponentDetail();
		virtual void OnSelectedByImGui();
#endif
	};

};
