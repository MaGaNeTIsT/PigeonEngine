#include "World.h"
#include "../../../../Main/MainManager.h"
#include "../../../../../../EngineThirdParty/JoltPhysics/Headers/PhysicsManager.h"
#include <Renderer/RenderInterface.h>
#include <PigeonBase/Object/Component/SceneComponent.h>
#include "PigeonBase/Object/Actor/LevelActor.h"
#include "PigeonBase/Object/Controller/Controller.h"

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
#if _EDITOR_ONLY
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
#if _EDITOR_ONLY
        this->EditorController = new PEditorController();
        this->EditorController->SetIsTickable(TRUE);
        POBJ_DEBUGNAME_SET(this->EditorController, "EditorController");
        this->AddActor(EditorController);
        this->EditorController->SetActorLocation(Vector3(0.0f, 350.0f, -500.0f));
        this->EditorController->SetActorRotation(MakeQuaternion(Euler(40.0f, 0.0f, 0.0f)));
#else
        this->Controller = new PController();
        this->Controller->SetIsTickable(TRUE);
        POBJ_DEBUGNAME_SET(this->Controller, "Controller");
        this->AddActor(Controller);
        this->Controller->SetActorLocation(Vector3(0.0f, 350.0f, -500.0f));
        this->Controller->SetActorRotation(MakeQuaternion(Euler(40.0f, 0.0f, 0.0f)));
#endif
    }

    BOOL32 PWorld::ShouldAffectGlobalPhysics() const
    {
        return TRUE;
    }

    void PWorld::Destroy()
    {
#if _EDITOR_ONLY
		EditorController = nullptr;
#endif
		Controller = nullptr;
        if(RootActor)
        {
            RootActor->Destroy();
            RootActor = nullptr;
        }
        // AllActors.Clear();
        PObject::Destroy();
    }

    TArray<PActor*> PWorld::GetAllActors() const
    {
        return this->RootActor->GetAllActorsAttached(TRUE);
    }

    const PActor* PWorld::GetActorByUniqueID(const ObjectIdentityType& InUniqueID, const BOOL8& bIncludeChildActor) const
    {
        PE_CHECK(ENGINE_WORLD_ERROR, "PWorld::GetActorByUniqueId : RootActor is nullptr.", RootActor != nullptr);
        for(auto& elem : RootActor->ChildrenActors)
        {
            const PActor* Ret = elem->GetActorByUniqueID(InUniqueID, bIncludeChildActor);
            if(Ret)
            {
                return Ret;
            }
        }
        return nullptr;
        
    }

    const PActorComponent* PWorld::GetComponentByUniqueID(const ObjectIdentityType& InUniqueID,
        const BOOL8& bIncludeChildComponent) const
    {
        for(auto& elem : RootActor->ChildrenActors)
        {
            const PActorComponent* Ret = elem->GetComponentByUniqueID(InUniqueID, bIncludeChildComponent);
            if(Ret)
            {
                return Ret;
            }
        }
        return nullptr;
    }

    void PWorld::AddActor(PActor* NewActor, const ETransform& Trans)
    {
		PE_CHECK(ENGINE_WORLD_ERROR, "PWorld::AddActor : Adding nullptr to world.", NewActor != nullptr);
		PE_CHECK(ENGINE_WORLD_ERROR, "PWorld::AddActor : Adding world root to world.", NewActor != RootActor);
        
        
        NewActor->AttachToActor(RootActor);
        NewActor->GetRootComponent()->SetComponentWorldTransform(Trans);
		if (!NewActor->IsInitialized())
		{
			NewActor->Init();
		}
        NewActor->BeginAddedToScene(this);
        // AllActors.Add(NewActor);
    }

  
#if _EDITOR_ONLY
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
        // Controller->EditorTick(deltaTime);
    }

#endif

	PController* PWorld::GetController() const
	{
#if _EDITOR_ONLY
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
    const RSceneInterface* PWorld::GetRenderScene()const
    {
        return RenderScene;
    }
#if _EDITOR_ONLY
  
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

	void PWorld::GenerateWorldOutline()
    {
        for(const auto& elem : RootActor->GetAllActorsAttached())
        {
            elem->GenerateWorldOutline(ImguiSelectedActor);
        }
    }

    void PWorld::GenerateDetail()
    {
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