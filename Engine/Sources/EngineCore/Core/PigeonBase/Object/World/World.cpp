#include "World.h"
#include "../../../../Main/MainManager.h"
#include "../../../../../../EngineThirdParty/JoltPhysics/Headers/PhysicsManager.h"
#include <Renderer/RenderInterface.h>
#include <PigeonBase/Object/Component/SceneComponent.h>
#include "PigeonBase/Object/Actor/LevelActor.h"
#include "PigeonBase/Object/Controller/Controller.h"
#include "PigeonBase/Object/Component/CameraAndLight/CameraComponent.h"
#if ENGINE_EDITOR_ONLY
#include <Editor/EditorLogManager.h>
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
		if (RootActor)
		{
			RootActor->Tick(deltaTime);
		}

	}

	void PWorld::FixedTick(FLOAT deltaTime)
	{
		if (RootActor)
		{
			RootActor->FixedTick(deltaTime);
		}
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
		// AllActors.Clear();
	}

	TArray<PActor*> PWorld::GetAllActors() const
	{
		// The registry is flat, so this no longer recurses (and copies) the attachment
		// tree on every call - the editor picking and the navigation bake both used to
		// pay that per frame / per bake.
		TArray<PActor*> Result;
		for (TSet<PActor*>::TConstIterator It = AllActors.Begin(); It != AllActors.End(); It++)
		{
			Result.Add(*It);
		}
		return Result;
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
		// Every actor is in the flat registry, so scanning each actor's own components
		// covers the whole world. The old loop only scanned the root-level actor list,
		// which made components on nested actors invisible to every post-physics
		// handler that looks its component up by unique ID.
		for (TSet<PActor*>::TConstIterator It = AllActors.Begin(); It != AllActors.End(); It++)
		{
			const PActor* Actor = (*It);
			if (!Actor)
			{
				continue;
			}
			const PActorComponent* Ret = Actor->GetComponentByUniqueID(InUniqueID, bIncludeChildComponent);
			if (Ret)
			{
				return Ret;
			}
		}
		return nullptr;
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
		AllActors.Add(NewActor);
		ActorById.Add(NewActor->GetUniqueID(), NewActor);
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
		AllActors.Remove(Actor);
		// TMap::Remove goes through find-then-erase, which must not see a missing key.
		if (ActorById.ContainsKey(Actor->GetUniqueID()))
		{
			ActorById.Remove(Actor->GetUniqueID());
		}
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
				ActorsToDestroy[Index]->Destroy();
			}
		}
		bFlushingPendingDestroy = FALSE;
	}


#if ENGINE_EDITOR_ONLY
	void PWorld::AddSceneToWorld(PScene* NewScene)
	{
		Scenes.Add(NewScene);
	}

	void PWorld::RemoveSceneFromWorld(PScene* Scene)
	{
		Scenes.Remove(Scene);
	}

	void PWorld::SetCurrentScene(PScene* Scene)
	{
		CurrentScene = Scene;
	}

	void PWorld::EditorTick(FLOAT deltaTime)
	{
		RootActor->EditorTick(deltaTime);
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
			TSet<PSceneComponent*> Children = Root->GetChildrenComponents();
			if (Children.Num() == 1)
			{
				DefaultComp = *Children.begin();
			}
		}

		this->SetSelectedComponent(DefaultComp);
	}
	void PWorld::SetSelectedComponent(PActorComponent* Selected)
	{
		this->ImguiSelectedComponent = Selected;
		if (this->ImguiSelectedComponent)
		{
			this->ImguiSelectedComponent->OnSelectedByImGui();
		}
	}

	void PWorld::ValidateSelection()
	{
		// A destroyed actor or component leaves the selection pointers dangling.
		// RemovedFromScene clears a dying object's world pointer, so a mismatch here is
		// the cheap per-frame validity check - and a valid selection can only ever point
		// at an object of this world.
		if (ImguiSelectedActor && (ImguiSelectedActor->GetWorld() != this))
		{
			SetSelectedActor(nullptr);
		}
		if (ImguiSelectedComponent)
		{
			const PActor* SelectedOwner = ImguiSelectedComponent->GetOwnerActor();
			if (!SelectedOwner || (SelectedOwner->GetWorld() != this))
			{
				SetSelectedComponent(nullptr);
			}
		}
	}

	void PWorld::GenerateWorldOutline()
	{
		if (!RootActor)
		{
			return;
		}
		ValidateSelection();
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
	}
#endif
};
