#pragma once

#include <CoreMinimal.h>
#include <Base/Delegate/Delegate.h>
#include <Base/DataStructure/Pointer/SharedPtr.h>
#include <Base/DataStructure/Container/Map.h>
#include <EngineCommon.h>
#include <PigeonBase/Object/Object.h>
#include <PigeonBase/Object/Actor.h>

namespace PigeonEngine
{
	class EGameTimer;
	class PActorComponent;
	class PStaticMeshComponent;
	class PSkeletalMeshComponent;
	class RSceneInterface;
	class PLevelActor;
	class FWorldEditorCommandStack;
	enum EWorldType : UINT8
	{
		EWT_GAME,
		EWT_EDITOR,
		EWT_RUN_IN_EDITOR, // maybe useless
	};

	class FWorldGravitySettings
	{
	public:
		Vector3 GravityDirection = -Vector3::YVector();
		FLOAT GravityStrength = 981.f;
#if ENGINE_EDITOR_ONLY
		BOOL32 bUseUpVector = TRUE;
#else
		BOOL32 bUseUpVector = FALSE;
#endif
	};

	MAKE_DELEGATE_MULTI_ONE_PARAM(FOnWorldUpVectorChanged, const Vector3&)
	MAKE_DELEGATE_MULTI_ONE_PARAM(FOnWorldGravityChanged, const Vector3&)
	MAKE_DELEGATE_MULTI_ONE_PARAM(FOnWorldActorAdded, PActor*)
	MAKE_DELEGATE_MULTI_ONE_PARAM(FOnWorldActorRemoved, PActor*)
	MAKE_DELEGATE_MULTI_ONE_PARAM(FOnWorldActorDestroyed, PActor*)
	MAKE_DELEGATE_MULTI_ONE_PARAM(FOnWorldComponentAdded, PActorComponent*)
	MAKE_DELEGATE_MULTI_ONE_PARAM(FOnWorldComponentRemoved, PActorComponent*)

	/*
	 * actual world in runtime
	 */
	class PWorld : public PObject
	{
	public:
		CLASS_VIRTUAL_NOCOPY_BODY(PWorld)

	// Begin PObject Interface
	public:
		void	Init() override;
		void	Uninit() override;
		void	Tick(FLOAT deltaTime) override;
		void	FixedTick(FLOAT deltaTime);

	protected:
		// Destroys the root actor (which cascades through the whole actor tree) and drops
		// the controller references, as the world's destroy hook.
		virtual void OnDestroy() override;
	// End PObject Interface

	// Actors in world management
	public:
		PE_NODISCARD TArray<PActor*>        GetAllActors() const;
		PE_NODISCARD UINT32                 GetActorCount() const;
		PE_NODISCARD UINT32                 GetComponentCount() const;
		// Tick is split so the render-update pass can be measured and optimized apart from
		// the gameplay tick. Tick still calls both in order.
		void                                TickRenderTree();
		void                                TickGameplay(FLOAT deltaTime);
		PE_NODISCARD const PActor*          GetActorByUniqueID(const ObjectIdentityType& UniqueID, const BOOL8& bIncludeChildActor) const ;
		PE_NODISCARD const PActorComponent* GetComponentByUniqueID(const ObjectIdentityType& UniqueID, const BOOL8& bIncludeChildComponent) const ;
	// Handle resolution: looks the object up in the registries and validates its
	// generation, so a reference to a destroyed object resolves to nullptr instead of a
	// dangling pointer. Cross-object references are meant to be stored as handles.
	PE_NODISCARD PActor* ResolveActor(const FObjectHandle& InHandle) const;
	PE_NODISCARD PActorComponent* ResolveComponent(const FObjectHandle& InHandle) const;
	template <typename TComponent>
	PE_NODISCARD TComponent* ResolveComponent(const FObjectHandle& InHandle) const
	{
		return dynamic_cast<TComponent*>(ResolveComponent(InHandle));
	}
		void AddActor(PActor* NewActor, const ETransform& Trans = ETransform());
		// Attach-rule version: KEEP_WORLD adds an actor whose transform the caller already
		// set in world space without the world root re-basing it; KEEP_RELATIVE applies the
		// provided transform relative to the world root.
		void AddActor(PActor* NewActor, EAttachmentRuleType InRule, const ETransform& RelativeTransform = ETransform());
		// Level IO: SaveLevel writes every ShouldSerializeToLevel actor plus the world
		// header; LoadLevel reads a file, clears the current level actors (root and default
		// controller stay), loads the file and applies the header. Both go through the
		// reflection serializer, so a binary backend can replace the file encoding later.
		BOOL32 SaveLevel(const EString& InPath);
		BOOL32 LoadLevel(const EString& InPath);
		// Destroys every level actor, keeping the world root and its controller. Child actors
		// are destroyed through their top-level ancestor.
		void ClearLevelActors();
	// Takes the actor out of the world and every lookup without destroying it; the
	// caller keeps ownership.
	void RemoveActor(PActor* Actor);
	// Queues the actor for destruction at the next flush. The actor stops ticking,
	// simulating and being looked up immediately - it leaves the tree, the registry and
	// the physics at mark time - but its memory stays valid until the flush, so loops
	// and callbacks that still hold it are safe.
	void DestroyActor(PActor* Actor);
	// Queues a component the same way; its scene registration ends at mark time.
	void DestroyComponent(PActorComponent* Component);
	// Runs the queued destructions. Called at the top of every Update, once the ticks,
	// physics callbacks and editor updates of the previous pass have all unwound.
	void FlushPendingDestroy();
	// Registry maintenance: drops an actor from the flat index. Called from the actor's
	// own RemovedFromScene - every destroy path funnels through it while the world
	// pointer is still set - and from RemoveActor; a no-op for an unregistered actor.
	void UnregisterActor(PActor* Actor);
	// Component registry maintenance, driven by the component scene hooks: a component
	// that joins the scene is indexed by its unique ID here (so the by-ID lookup is O(1)
	// instead of walking every actor), and leaving the scene removes it.
	void RegisterComponent(PActorComponent* Component);
	void UnregisterComponent(PActorComponent* Component);

	// Lifecycle events. Subscribe with Add (returns a FDelegateHandle) and Remove(handle)
	// before the listener dies; components conventionally do that in Uninit. Added fires
	// after the object is fully registered, Removed when it leaves the scene, Destroyed
	// just before a queued actor's memory is released.
	FOnWorldActorAdded			OnActorAdded;
	FOnWorldActorRemoved		OnActorRemoved;
	FOnWorldActorDestroyed		OnActorDestroyed;
	FOnWorldComponentAdded		OnComponentAdded;
	FOnWorldComponentRemoved	OnComponentRemoved;
private:
	PActor* RootActor = nullptr;
	// Flat registry of every actor added to this world (root excluded), maintained by
	// AddActor/RemoveActor/UnregisterActor. Insertion order, so iteration is
	// deterministic. The attachment tree remains the structural truth; this is the
	// iteration and lookup index built on top of it.
	TArray<PActor*> AllActors;
	TMap<ObjectIdentityType, PActor*> ActorById;
	// Every component currently in the scene, keyed by its own unique ID.
	TMap<ObjectIdentityType, PActorComponent*> ComponentById;
	// Objects queued by DestroyActor/DestroyComponent. The queues are copied out at
	// flush time, so a destroy requested during the flush itself (from a UserEndPlay or
	// an OnDestroy) lands in the live queues and waits for the next one.
	TArray<PActor*> PendingDestroyActors;
	TArray<PActorComponent*> PendingDestroyComponents;
	BOOL32 bFlushingPendingDestroy = FALSE;

	public:
		class PController* GetController() const;
		PE_NODISCARD const Vector3& GetUpVector() const;
		void SetUpVector(const Vector3& InUpVector);
		PE_NODISCARD Vector3 GetGravity() const;
		PE_NODISCARD const FWorldGravitySettings& GetGravitySettings() const;
		void SetGravity(const Vector3& InGravity);
		void SetGravitySettings(const FWorldGravitySettings& InGravitySettings);
		PE_NODISCARD BOOL32 IsGravityUsingUpVector() const;
		void SetGravityUseUpVector(BOOL32 bInUseUpVector);
		FOnWorldUpVectorChanged OnUpVectorChanged;
		FOnWorldGravityChanged OnGravityChanged;
	protected:
		virtual PActor* CreateRootActor();
		virtual void CreateDefaultController();
		virtual BOOL32 ShouldAffectGlobalPhysics() const;
	private:
		class PController* Controller = nullptr;
		Vector3 UpVector = Vector3::YVector();
		FWorldGravitySettings GravitySettings;
	private:
		void ApplyGravitySettings();
	public:
		const EGameTimer* GetGameTimer() const;

		//Render scene state START
	public:
		void                    BindRenderScene(RSceneInterface* InScene);
		RSceneInterface*        GetRenderScene();
		const RSceneInterface*  GetRenderScene() const;
		void                    UpdateCameraViewports(UINT32 InWidth, UINT32 InHeight);
	private:
		RSceneInterface*    RenderScene = nullptr;
		//Render scene state END

#if ENGINE_EDITOR_ONLY
		// IMGUI
	public:
		void EditorTick(FLOAT deltaTime) override;
		void SetSelectedActor(PActor* Selected);
		void SetSelectedComponent(PActorComponent* Selected);
		// Resolve through the registry so a selection whose object was destroyed reads as
		// null immediately, not one frame later.
		PE_NODISCARD PActor* GetSelectedActor() const;
		PE_NODISCARD PActorComponent* GetSelectedComponent() const;
	public:
		void GenerateWorldOutline();
		void GenerateDetail();
		// The editor installs its command stack here so the outline/details panels can record
		// their mutations. Null in a world that is not being edited.
		void SetEditorCommandStack(FWorldEditorCommandStack* InCommands);
		PE_NODISCARD FWorldEditorCommandStack* GetEditorCommandStack() const;
	private:
		FWorldEditorCommandStack* EditorCommandStack = nullptr;
	private:
		// Drops the ImGui selection when the selected actor or component no longer
		// belongs to this world (a destroyed object leaves the registry at mark time), so
		// the panels never dereference freed objects.
		void ValidateSelection();
	private:
		FObjectHandle ImguiSelectedActorHandle;
		FObjectHandle ImguiSelectedComponentHandle;
		PActor* ImguiSelectedActor = nullptr;
		PActorComponent* ImguiSelectedComponent = nullptr;
	private:
		class PEditorController* EditorController = nullptr;
#endif

	};


};
