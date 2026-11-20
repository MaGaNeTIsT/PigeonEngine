#include "World.h"
#include <Development/Profiler.h>
#include "../../../../Main/MainManager.h"
#include "../../../../../../EngineThirdParty/JoltPhysics/Headers/PhysicsManager.h"
#include <Renderer/RenderInterface.h>
#include <PigeonBase/Object/Component/SceneComponent.h>
#include <PigeonBase/Object/Component/ActorComponent.h>
#include <Reflection/LevelSerializer.h>
#include <RapidJson/Headers/JsonObject.h>
#include <Editor/EditorLogManager.h>
#include "PigeonBase/Object/Actor/LevelActor.h"
#include "PigeonBase/Object/Actor/LevelCharacter.h"
#include "PigeonBase/Object/Controller/Controller.h"
#include "PigeonBase/Object/Component/CameraAndLight/CameraComponent.h"
#if ENGINE_EDITOR_ONLY
#include <Editor/WorldEditorCommand.h>
#include <imgui.h>
#endif


namespace PigeonEngine
{
	namespace
	{
		Vector3 NormalizeOrDefault(const Vector3& InVector, const Vector3& InDefaultVector)
		{
			if (InVector.LengthSquare() <= PE_SMALL_NUMBER)
			{
				return InDefaultVector;
			}

			return Vector3::Normalize(InVector);
		}

#if ENGINE_EDITOR_ONLY
		BOOL32 IsClassDerivedFrom(const size_t& InClassHash, const size_t& InBaseHash)
		{
			for (size_t ClassHash = InClassHash; ClassHash != static_cast<size_t>(-1); ClassHash = GetClassParentHash(ClassHash))
			{
				if (ClassHash == InBaseHash)
				{
					return TRUE;
				}
			}
			return FALSE;
		}
#endif
	}

	static void RegisterClassTypes()
	{
		RegisterClassType<PWorld, PObject>();
		// RegisterClassType<ESceneTreeNode, TOctreeNode<PActor*>>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	PWorld::PWorld()
	{
		this->RenderScene = nullptr;
	}
	PWorld::~PWorld()
	{
	}

	void PWorld::Init()
	{
#if ENGINE_EDITOR_ONLY
		EditorController = nullptr;
#endif
		Controller = nullptr;

		if (RootActor)
		{
			RootActor->Destroy();
			RootActor = nullptr;
		}

		// The scheduler must exist before the root and default controller join the scene,
		// because their BeginAddedToScene hooks register their first entries.
		TickScheduler.Initialize(*this);
		this->RootActor = CreateRootActor();
		this->RootActor->SetActorScale(Vector3(1,1,1));
		this->RootActor->SetIsTickable(TRUE);
		if (!this->RootActor->IsInitialized())
		{
			this->RootActor->Init();
		}
		this->RootActor->BeginAddedToScene(this);

		CreateDefaultController();

		ApplyGravitySettings();

		SetInitialized(TRUE);
	}

	void PWorld::Uninit()
	{
		PObject::Uninit();
	}

	void PWorld::Tick(FLOAT deltaTime)
	{
		PE_PROFILE_SCOPE(PROFILER_SCOPE_WORLD_TICK);
		TickRenderTree();
		TickGameplay(deltaTime);
		PE_PROFILE_COUNTER(PROFILER_COUNTER_WORLD_ACTOR_NUM, GetActorCount());
		PE_PROFILE_COUNTER(PROFILER_COUNTER_WORLD_COMPONENT_NUM, GetComponentCount());
	}

	void PWorld::TickRenderTree()
	{
		if (RootActor)
		{
			// Render updates first and independently of tickability: deferred asset applies
			// and render-state sends must resolve even on a non-tickable actor's subtree.
			PE_PROFILE_SCOPE(PROFILER_SCOPE_WORLD_RENDER_TREE);
			RootActor->TickRenderTree();
		}
	}

	void PWorld::TickGameplay(FLOAT deltaTime)
	{
		if (RootActor)
		{
			PE_PROFILE_SCOPE(PROFILER_SCOPE_WORLD_GAMEPLAY);
			PE_PROFILE_SCOPE(PROFILER_SCOPE_WORLD_TICK_SCHEDULER);
			TickScheduler.Tick(ETickDomain::VARIABLE, deltaTime);
		}
	}

	void PWorld::FixedTick(FLOAT deltaTime)
	{
		PE_PROFILE_SCOPE(PROFILER_SCOPE_WORLD_FIXED_TICK);
		PE_PROFILE_SCOPE(PROFILER_SCOPE_WORLD_FIXED_SCHEDULER);
		TickScheduler.Tick(ETickDomain::FIXED, deltaTime);
	}

	void PWorld::PostPhysicsTick(FLOAT deltaTime)
	{
		PE_PROFILE_SCOPE(PROFILER_SCOPE_WORLD_POST_PHYSICS_TICK);
		PE_PROFILE_SCOPE(PROFILER_SCOPE_WORLD_POST_PHYSICS_SCHEDULER);
		TickScheduler.Tick(ETickDomain::POST_PHYSICS, deltaTime);
	}

	PActor* PWorld::CreateRootActor()
	{
		return new PLevelActor();
	}

	void PWorld::CreateDefaultController()
	{
#if ENGINE_EDITOR_ONLY
		this->EditorController = new PEditorController();
		this->EditorController->SetIsTickable(TRUE);
		POBJ_DEBUGNAME_SET(this->EditorController, "EditorController");
		// Set transform BEFORE AddActor so that BeginAddedToScene ->
		// PCameraComponent::CreateRenderState -> RScene::AddCamera captures
		// the real camera location/rotation when it builds the initial
		// ERenderViewMatrices. Otherwise SetupProxy is called with identity
		// and the first frame's view CB stays at identity until the
		// SetActorLocation-triggered UpdateCamera lambda is drained - which
		// can be a frame or more late.
		this->EditorController->SetActorLocation(Vector3(0.0f, 350.0f, -500.0f));
		this->EditorController->SetActorRotation(MakeQuaternion(Euler(40.0f, 0.0f, 0.0f)));
		this->AddActor(EditorController);
#else
		this->Controller = new PController();
		this->Controller->SetIsTickable(TRUE);
		POBJ_DEBUGNAME_SET(this->Controller, "Controller");
		this->Controller->SetActorLocation(Vector3(0.0f, 350.0f, -500.0f));
		this->Controller->SetActorRotation(MakeQuaternion(Euler(40.0f, 0.0f, 0.0f)));
		this->AddActor(Controller);
#endif
	}

	BOOL32 PWorld::ShouldAffectGlobalPhysics() const
	{
		return TRUE;
	}

	void PWorld::OnDestroy()
	{
		// Anything still queued dies with the world, through the same orchestrated path.
		FlushPendingDestroy();
#if ENGINE_EDITOR_ONLY
		EditorController = nullptr;
		// The whole world is going away; nothing can stay selected in it.
		SetSelectedActor(nullptr);
#endif
		Controller = nullptr;
		if(RootActor)
		{
			RootActor->Destroy();
			RootActor = nullptr;
		}
		// Every actor/component unregistered through RemovedFromScene by now. Clearing the
		// scheduler is the final safety net and drops the owner sequence tables.
		TickScheduler.Shutdown();
		// AllActors.Clear();
	}

	FWorldTickScheduler& PWorld::GetTickScheduler()
	{
		return TickScheduler;
	}

	const FWorldTickScheduler& PWorld::GetTickScheduler() const
	{
		return TickScheduler;
	}

	TArray<PActor*> PWorld::GetAllActors() const
	{
		// The registry is flat and insertion ordered, so this is one array copy - the
		// editor picking and the navigation bake both used to walk and copy the tree.
		return AllActors;
	}

	UINT32 PWorld::GetActorCount() const
	{
		return AllActors.Num<UINT32>();
	}

	UINT32 PWorld::GetComponentCount() const
	{
		return ComponentById.Num<UINT32>();
	}

	const PActor* PWorld::GetActorByUniqueID(const ObjectIdentityType& InUniqueID, const BOOL8& bIncludeChildActor) const
	{
		// The flat registry indexes every actor in the world, so the recursive child
		// flag no longer changes what is found - it is kept for signature compatibility.
		(void)bIncludeChildActor;
		if (const PActor* const* Found = ActorById.FindValueAsPtr(InUniqueID))
		{
			return (*Found);
		}
		return nullptr;
	}

	const PActorComponent* PWorld::GetComponentByUniqueID(const ObjectIdentityType& InUniqueID,
		const BOOL8& bIncludeChildComponent) const
	{
		// Direct map lookup: every component that is in the scene is indexed here by its
		// own unique ID, so the per-fixed-tick handler lookups no longer scan actors.
		(void)bIncludeChildComponent;
		if (const PActorComponent* const* Found = ComponentById.FindValueAsPtr(InUniqueID))
		{
			return (*Found);
		}
		return nullptr;
	}

	PActor* PWorld::ResolveActor(const FObjectHandle& InHandle) const
	{
		if (!InHandle.IsValid())
		{
			return nullptr;
		}
		PActor* const* Found = ActorById.FindValueAsPtr(InHandle.UniqueID);
		if (Found && ((*Found)->GetGeneration() == InHandle.Generation))
		{
			return (*Found);
		}
		return nullptr;
	}

	PActorComponent* PWorld::ResolveComponent(const FObjectHandle& InHandle) const
	{
		if (!InHandle.IsValid())
		{
			return nullptr;
		}
		PActorComponent* const* Found = ComponentById.FindValueAsPtr(InHandle.UniqueID);
		if (Found && ((*Found)->GetGeneration() == InHandle.Generation))
		{
			return (*Found);
		}
		return nullptr;
	}

	void PWorld::RegisterComponent(PActorComponent* Component)
	{
		if (!Component)
		{
			return;
		}
		// Re-adding an actor walks its components again; only a first registration is an
		// event.
		const BOOL32 bWasRegistered = ComponentById.ContainsKey(Component->GetUniqueID());
		ComponentById.Add(Component->GetUniqueID(), Component);
		if (!bWasRegistered)
		{
			OnComponentAdded.Broadcast(Component);
		}
	}

	void PWorld::UnregisterComponent(PActorComponent* Component)
	{
		if (!Component)
		{
			return;
		}
		// TMap::Remove goes through find-then-erase, which must not see a missing key.
		if (ComponentById.ContainsKey(Component->GetUniqueID()))
		{
			ComponentById.Remove(Component->GetUniqueID());
			OnComponentRemoved.Broadcast(Component);
		}
#if ENGINE_EDITOR_ONLY
		if (ImguiSelectedComponent == Component)
		{
			SetSelectedComponent(nullptr);
		}
#endif
	}

	void PWorld::AddActor(PActor* NewActor, const ETransform& Trans)
	{
		// Historic overload: the attach keeps whatever transform the caller already set.
		AddActor(NewActor, EAttachmentRuleType::KEEP_CURRENT, Trans);
	}

	void PWorld::AddActor(PActor* NewActor, EAttachmentRuleType InRule, const ETransform& RelativeTransform)
	{
		PE_CHECK(ENGINE_WORLD_ERROR, "PWorld::AddActor : Adding nullptr to world.", NewActor != nullptr);
		PE_CHECK(ENGINE_WORLD_ERROR, "PWorld::AddActor : Adding world root to world.", NewActor != RootActor);

		// An actor belongs to one world. One that already lives in another world is
		// rejected; one already in this world falls through - the attach re-parents and
		// the scene-add is idempotent - and the registry add below deduplicates.
		if (PWorld* CurrentWorld = NewActor->GetWorld())
		{
			if (CurrentWorld != this)
			{
				PE_CHECK(ENGINE_WORLD_ERROR, "PWorld::AddActor : Actor already belongs to another world.", FALSE);
				return;
			}
		}

		NewActor->AttachToActor(RootActor, InRule, RelativeTransform);
		// The attach rule decides the transform: KEEP_CURRENT (what the historic overload
		// uses) keeps whatever the caller set before the add, KEEP_WORLD preserves the
		// world transform, KEEP_RELATIVE applies RelativeTransform.
		if (!NewActor->IsInitialized())
		{
			NewActor->Init();
		}
		NewActor->BeginAddedToScene(this);
		// The component hooks registered everything while the scene-add ran; index the
		// actor last so listeners see a fully registered object.
		if (!AllActors.Contains(NewActor))
		{
			AllActors.Add(NewActor);
		}
		ActorById.Add(NewActor->GetUniqueID(), NewActor);
		OnActorAdded.Broadcast(NewActor);
	}

	BOOL32 PWorld::SaveLevel(const EString& InPath)
	{
		EString LevelName = InPath;
		const INT32 LastSlash = LevelName.RightFind(EString(PE_TEXT("/")));
		if (LastSlash >= 0)
		{
			LevelName = LevelName.Right(LevelName.Length() - static_cast<UINT32>(LastSlash) - 1u);
		}
		const INT32 LastDot = LevelName.RightFind(EString(PE_TEXT(".")));
		if (LastDot > 0)
		{
			LevelName = LevelName.Left(static_cast<UINT32>(LastDot));
		}

		if (!ELevelSerializer::SaveWorldToFile(*this, LevelName, InPath))
		{
			PE_LOG_ERROR(EString(PE_TEXT("Level save failed: ")) + InPath);
			return FALSE;
		}
		PE_LOG_LOG(EString(PE_TEXT("Level saved: ")) + InPath);
		return TRUE;
	}

	BOOL32 PWorld::LoadLevel(const EString& InPath)
	{
		EString DocumentText;
		if (!EFileHelper::ReadFileAsString(InPath, DocumentText))
		{
			PE_LOG_ERROR(EString(PE_TEXT("Level load: cannot read ")) + InPath);
			return FALSE;
		}
		CJsonObject Document(DocumentText);
		if (!Document.IsValid())
		{
			PE_LOG_ERROR(EString(PE_TEXT("Level load: invalid JSON in ")) + InPath);
			return FALSE;
		}

		FLevelHeader Header;
		const BOOL32 bHasHeader = ELevelSerializer::ReadLevelHeader(Document, Header);

		ClearLevelActors();
		TArray<PActor*> CreatedActors;
		if (!ELevelSerializer::LoadActors(*this, Document, &CreatedActors))
		{
			return FALSE;
		}

		if (bHasHeader)
		{
			SetUpVector(Header.UpVector);
			FWorldGravitySettings LoadedGravitySettings;
			LoadedGravitySettings.GravityDirection = Header.GravityDirection;
			LoadedGravitySettings.GravityStrength = Header.GravityStrength;
			LoadedGravitySettings.bUseUpVector = Header.bUseUpVector;
			SetGravitySettings(LoadedGravitySettings);
		}

#if ENGINE_EDITOR_ONLY
		ImguiSelectedActor = nullptr;
		ImguiSelectedComponent = nullptr;
#endif
		PE_LOG_LOG(EString(PE_TEXT("Level loaded: ")) + InPath + EString(PE_TEXT(", actors ")) + EString::FromInt(CreatedActors.Num<UINT32>()));
		return TRUE;
	}

	void PWorld::ClearLevelActors()
	{
		if (!RootActor)
		{
			return;
		}
		const TArray<PActor*> Actors = AllActors;
		PActor* DefaultController = GetController();
		for (UINT32 Index = 0u, Num = Actors.Num<UINT32>(); Index < Num; Index++)
		{
			PActor* Actor = Actors[Index];
			if ((!Actor) || (Actor == DefaultController))
			{
				continue;
			}
			// Only top-level level actors: a child is destroyed through its ancestor.
			if (Actor->GetAttachedParentActor() != RootActor)
			{
				continue;
			}
			DestroyActor(Actor);
		}
		FlushPendingDestroy();
	}

	void PWorld::RemoveActor(PActor* Actor)
	{
		PE_CHECK(ENGINE_WORLD_ERROR, "PWorld::RemoveActor : Removing nullptr from world.", Actor != nullptr);
		if (Actor->GetWorld() != this)
		{
			// Not this world's actor: its parent lives in another tree, and the checks
			// below would reach across worlds. PE_CHECK only reports in some builds, so
			// the return matters as much as the check.
			PE_CHECK(ENGINE_WORLD_ERROR, "PWorld::RemoveActor : Actor does not belong to this world.", FALSE);
			return;
		}

		// Leave the parent's child list without DetachFromParentActor, which re-homes
		// the actor under this world's root; removal wants it out of the tree entirely.
		if (PActor* Parent = Actor->GetAttachedParentActor())
		{
			Parent->ChildrenActors.Remove(Actor);
			Actor->AttachedParentActor = nullptr;
		}
		Actor->GetRootComponent()->DetachFromParentComponent();
		if (Actor->IsAddedToScene())
		{
			Actor->RemovedFromScene();
		}
		UnregisterActor(Actor);
	}

	void PWorld::UnregisterActor(PActor* Actor)
	{
		if (!Actor)
		{
			return;
		}
		// Every removal path can reach this twice (RemoveActor, then the actor's own
		// RemovedFromScene); only a real unregistration raises the event.
		const BOOL32 bWasRegistered = ActorById.ContainsKey(Actor->GetUniqueID());
		AllActors.Remove(Actor);
		if (bWasRegistered)
		{
			ActorById.Remove(Actor->GetUniqueID());
			OnActorRemoved.Broadcast(Actor);
		}
#if ENGINE_EDITOR_ONLY
		if (ImguiSelectedActor == Actor)
		{
			SetSelectedActor(nullptr);
		}
#endif
	}

	void PWorld::DestroyActor(PActor* Actor)
	{
		PE_CHECK(ENGINE_WORLD_ERROR, "PWorld::DestroyActor : Destroying nullptr actor.", Actor != nullptr);
		PE_CHECK(ENGINE_WORLD_ERROR, "PWorld::DestroyActor : Destroying the world root.", Actor != RootActor);
		PE_CHECK(ENGINE_WORLD_ERROR, "PWorld::DestroyActor : Destroying the active controller.", Actor != GetController());
		if (Actor->IsPendingDestroy())
		{
			return;
		}
		if (Actor->GetWorld() != this)
		{
			// Not this world's actor, so not this world's destroy either; queueing it
			// would delete memory another world still walks.
			PE_CHECK(ENGINE_WORLD_ERROR, "PWorld::DestroyActor : Actor does not belong to this world.", FALSE);
			return;
		}
		// Mark-time exit from everything that can reach a dying actor this frame: the
		// tree (no tick walk gets here), the registry (no lookup, editor pick or
		// post-physics handler gets here) and physics. RemovedFromScene also runs now,
		// so UserEndPlay and the component scene cleanup see a whole object.
		RemoveActor(Actor);
		Actor->MarkPendingDestroy();
		// Bodies are keyed by the actor's ID and the removal is idempotent, so the
		// component teardown at flush time finds nothing left and is a no-op.
		FPhysicsManager::GetManagerSingleton()->RemoveBody(Actor->GetUniqueID(), TRUE);
		PendingDestroyActors.Add(Actor);
	}

	void PWorld::DestroyComponent(PActorComponent* Component)
	{
		PE_CHECK(ENGINE_WORLD_ERROR, "PWorld::DestroyComponent : Destroying nullptr component.", Component != nullptr);
		if (Component->IsPendingDestroy())
		{
			return;
		}
		if (const PActor* Owner = Component->GetOwnerActor())
		{
			if (Owner->GetWorld() && (Owner->GetWorld() != this))
			{
				// The component hangs under another world's actor; queueing it here would
				// delete memory that world still walks.
				PE_CHECK(ENGINE_WORLD_ERROR, "PWorld::DestroyComponent : Component's owner belongs to another world.", FALSE);
				return;
			}
		}
		Component->MarkPendingDestroy();
		// Scene exit at mark time ends sensor bodies and post-physics handlers
		// immediately; the memory stays until the flush.
		Component->OnRemovedFromScene();
		if (PActor* Owner = Component->GetOwnerActor())
		{
			Owner->RemoveComponentReference(Component);
		}
		if (PSceneComponent* SceneComponent = dynamic_cast<PSceneComponent*>(Component))
		{
			SceneComponent->DetachFromParentComponent();
		}
		PendingDestroyComponents.Add(Component);
	}

	void PWorld::FlushPendingDestroy()
	{
		if (bFlushingPendingDestroy)
		{
			return;
		}
		if ((PendingDestroyActors.Num<UINT32>() == 0u) && (PendingDestroyComponents.Num<UINT32>() == 0u))
		{
			return;
		}
		bFlushingPendingDestroy = TRUE;
		TArray<PActor*> ActorsToDestroy;
		ActorsToDestroy.Append(PendingDestroyActors);
		PendingDestroyActors.Empty();
		TArray<PActorComponent*> ComponentsToDestroy;
		ComponentsToDestroy.Append(PendingDestroyComponents);
		PendingDestroyComponents.Empty();
		for (UINT32 Index = 0u, Num = ComponentsToDestroy.Num<UINT32>(); Index < Num; Index++)
		{
			if (ComponentsToDestroy[Index])
			{
				ComponentsToDestroy[Index]->Destroy();
			}
		}
		for (UINT32 Index = 0u, Num = ActorsToDestroy.Num<UINT32>(); Index < Num; Index++)
		{
			if (ActorsToDestroy[Index])
			{
				// Last point where the actor is still a valid object: listeners get to
				// drop their references before the destroy orchestration runs.
				OnActorDestroyed.Broadcast(ActorsToDestroy[Index]);
				ActorsToDestroy[Index]->Destroy();
			}
		}
		bFlushingPendingDestroy = FALSE;
	}


#if ENGINE_EDITOR_ONLY
	void PWorld::EditorTick(FLOAT deltaTime)
	{
		if (RootActor)
		{
			PE_PROFILE_SCOPE(PROFILER_SCOPE_WORLD_EDITOR_TICK);
			PE_PROFILE_SCOPE(PROFILER_SCOPE_WORLD_EDITOR_SCHEDULER);
			TickScheduler.Tick(ETickDomain::EDITOR, deltaTime);
		}
	}

#endif

	PController* PWorld::GetController() const
	{
#if ENGINE_EDITOR_ONLY
		return this->EditorController;
#else
		return this->Controller;
#endif
	}

	const Vector3& PWorld::GetUpVector() const
	{
		return UpVector;
	}

	void PWorld::SetUpVector(const Vector3& InUpVector)
	{
		UpVector = NormalizeOrDefault(InUpVector, Vector3::YVector());
		OnUpVectorChanged.Broadcast(UpVector);
		if (GravitySettings.bUseUpVector)
		{
			ApplyGravitySettings();
		}
	}

	Vector3 PWorld::GetGravity() const
	{
		const Vector3 WorldUp = NormalizeOrDefault(UpVector, Vector3::YVector());
		const FLOAT GravityStrength = EMath::Abs(GravitySettings.GravityStrength);

		if (GravitySettings.bUseUpVector)
		{
			return -WorldUp * GravityStrength;
		}

		const Vector3 GravityDirection = NormalizeOrDefault(GravitySettings.GravityDirection, -WorldUp);
		return GravityDirection * GravityStrength;
	}

	const FWorldGravitySettings& PWorld::GetGravitySettings() const
	{
		return GravitySettings;
	}

	void PWorld::SetGravity(const Vector3& InGravity)
	{
		GravitySettings.bUseUpVector = FALSE;
		GravitySettings.GravityStrength = Vector3::Length(InGravity);
		GravitySettings.GravityDirection = GravitySettings.GravityStrength <= PE_SMALL_NUMBER ? -GetUpVector() : Vector3::Normalize(InGravity);
		ApplyGravitySettings();
	}

	void PWorld::SetGravitySettings(const FWorldGravitySettings& InGravitySettings)
	{
		GravitySettings = InGravitySettings;
		GravitySettings.GravityStrength = EMath::Abs(GravitySettings.GravityStrength);
		GravitySettings.GravityDirection = NormalizeOrDefault(GravitySettings.GravityDirection, -GetUpVector());
		ApplyGravitySettings();
	}

	BOOL32 PWorld::IsGravityUsingUpVector() const
	{
		return GravitySettings.bUseUpVector;
	}

	void PWorld::SetGravityUseUpVector(BOOL32 bInUseUpVector)
	{
		GravitySettings.bUseUpVector = bInUseUpVector;
		ApplyGravitySettings();
	}

	void PWorld::ApplyGravitySettings()
	{
		const Vector3 WorldGravity = GetGravity();
		if (ShouldAffectGlobalPhysics())
		{
			FPhysicsManager::GetManagerSingleton()->SetGravity(WorldGravity);
		}
		OnGravityChanged.Broadcast(WorldGravity);
	}

	const EGameTimer* PWorld::GetGameTimer() const
	{
		return EMainManager::GetManagerSingleton()->GetGameTimer();;
	}

	//Render scene state START
	void PWorld::BindRenderScene(RSceneInterface* InScene)
	{
		if (RenderScene)
		{
			RenderScene->ClearAll();
			RenderScene = nullptr;
		}
		RenderScene = InScene;
	}
	RSceneInterface* PWorld::GetRenderScene()
	{
		return RenderScene;
	}
	const RSceneInterface* PWorld::GetRenderScene() const
	{
		return RenderScene;
	}
	void PWorld::UpdateCameraViewports(UINT32 InWidth, UINT32 InHeight)
	{
		PController* ActiveController = GetController();
		if (!ActiveController)
		{
			return;
		}
		PCameraComponent* Camera = ActiveController->GetCamera();
		if (!Camera)
		{
			return;
		}
		Camera->SetViewport(0.f, 0.f, static_cast<FLOAT>(InWidth), static_cast<FLOAT>(InHeight));
	}
#if ENGINE_EDITOR_ONLY

	void PWorld::SetSelectedActor(PActor* Selected)
	{
		this->ImguiSelectedActor = Selected;
		this->ImguiSelectedActorHandle = MakeObjectHandle(Selected);
		if (!this->ImguiSelectedActor)
		{
			this->SetSelectedComponent(nullptr);
			return;
		}

		PSceneComponent* Root = this->ImguiSelectedActor->GetRootComponent();
		PActorComponent* DefaultComp = Root;

		// If root has exactly one child component, auto-select it so properties
		// like Materials are immediately visible without an extra click.
		if (Root)
		{
			TArray<PSceneComponent*> Children = Root->GetChildrenComponents();
			if (Children.Num() == 1)
			{
				DefaultComp = Children[0];
			}
		}

		this->SetSelectedComponent(DefaultComp);
	}
	void PWorld::SetSelectedComponent(PActorComponent* Selected)
	{
		this->ImguiSelectedComponent = Selected;
		this->ImguiSelectedComponentHandle = MakeObjectHandle(Selected);
		if (this->ImguiSelectedComponent)
		{
			this->ImguiSelectedComponent->OnSelectedByImGui();
		}
	}

	PActor* PWorld::GetSelectedActor() const
	{
		return ImguiSelectedActorHandle.IsValid() ? ResolveActor(ImguiSelectedActorHandle) : nullptr;
	}

	PActorComponent* PWorld::GetSelectedComponent() const
	{
		return ImguiSelectedComponentHandle.IsValid() ? ResolveComponent(ImguiSelectedComponentHandle) : nullptr;
	}

	void PWorld::ValidateSelection()
	{
		// Handles, not raw pointers, are the authority: a destroyed object leaves the
		// registries at mark time, so resolution fails before its memory is freed.
		if (ImguiSelectedActorHandle.IsValid())
		{
			PActor* ResolvedActor = ResolveActor(ImguiSelectedActorHandle);
			if (!ResolvedActor)
			{
				SetSelectedActor(nullptr);
			}
			else
			{
				ImguiSelectedActor = ResolvedActor;
			}
		}
		else if (ImguiSelectedActor)
		{
			SetSelectedActor(nullptr);
		}

		if (ImguiSelectedComponentHandle.IsValid())
		{
			PActorComponent* ResolvedComponent = ResolveComponent(ImguiSelectedComponentHandle);
			if (!ResolvedComponent)
			{
				SetSelectedComponent(nullptr);
			}
			else
			{
				ImguiSelectedComponent = ResolvedComponent;
			}
		}
		else if (ImguiSelectedComponent)
		{
			SetSelectedComponent(nullptr);
		}
	}

	void PWorld::SetEditorCommandStack(FWorldEditorCommandStack* InCommands)
	{
		EditorCommandStack = InCommands;
	}

	FWorldEditorCommandStack* PWorld::GetEditorCommandStack() const
	{
		return EditorCommandStack;
	}

	void PWorld::GenerateWorldOutline()
	{
		if (!RootActor)
		{
			return;
		}
		ValidateSelection();

		if (EditorCommandStack)
		{
			if (ImGui::Button("Create Actor"))
			{
				ImGui::OpenPopup("CreateActorPopup");
			}
			if (ImGui::BeginPopup("CreateActorPopup"))
			{
				std::vector<ERTTIManager::ESerializableClassInfo> ClassInfos;
				ERTTIManager::GetManagerSingleton()->GetSerializableClassInfos(ClassInfos);
				const size_t ActorBaseHash = GetClassHashCode<PActor>();
				const size_t LevelActorHash = GetClassHashCode<PLevelActor>();
				const size_t ControllerBaseHash = GetClassHashCode<PController>();
				const size_t CharacterBaseHash = GetClassHashCode<PCharacter>();
				for (UINT32 Index = 0u, Num = static_cast<UINT32>(ClassInfos.size()); Index < Num; Index++)
				{
					const ERTTIManager::ESerializableClassInfo& ClassInfo = ClassInfos[Index];
					if ((!ClassInfo.bHasFactory) ||
						(!IsClassDerivedFrom(ClassInfo.ClassHash, ActorBaseHash)) ||
						(ClassInfo.ClassHash == LevelActorHash) ||
						IsClassDerivedFrom(ClassInfo.ClassHash, ControllerBaseHash) ||
						IsClassDerivedFrom(ClassInfo.ClassHash, CharacterBaseHash))
					{
						continue;
					}
					if (ImGui::MenuItem(ClassInfo.StableName.c_str()))
					{
						PActor* NewActor = CreateClassObjectByStableName<PActor>(ClassInfo.StableName.c_str());
						if (NewActor)
						{
							POBJ_DEBUGNAME_SET(NewActor, EString(ClassInfo.StableName.c_str()));
							AddActor(NewActor);
							SetSelectedActor(NewActor);
							PushSpawnActorCommand(*EditorCommandStack, *this, NewActor);
						}
					}
				}
				ImGui::EndPopup();
			}
			ImGui::Separator();
		}

		for (PActor* Actor : RootActor->GetAllActorsAttached())
		{
			Actor->GenerateWorldOutline(ImguiSelectedActor);
		}
	}

	void PWorld::GenerateDetail()
	{
		ValidateSelection();
		if (!ImguiSelectedActor)
		{
			return;
		}

		ImguiSelectedActor->GenerateDetails(ImguiSelectedComponent);
		if (!ImguiSelectedComponent)
		{
			return;
		}
		ImguiSelectedComponent->GenerateComponentDetail();
		if (EditorCommandStack &&
			(ImguiSelectedComponent != ImguiSelectedActor->GetRootComponent()) &&
			(ImguiSelectedActor->GetWorld() == this))
		{
			ImGui::Separator();
			if (ImGui::Button("Remove Component"))
			{
				PushDeleteComponentCommand(*EditorCommandStack, *this, ImguiSelectedComponent);
			}
		}
	}
#endif
};
