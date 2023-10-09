#include "World.h"
#include "../../../../Main/MainManager.h"
#include <Renderer/RenderInterface.h>
#include <PigeonBase/Object/Component/SceneComponent.h>
#include "PigeonBase/Object/Actor/LevelActor.h"
#include "PigeonBase/Object/Controller/Controller.h"

namespace PigeonEngine
{

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
        if(Controller)
        {
            Controller->Destroy();
            Controller = nullptr;
        }
       
        if(RootActor)
        {
            RootActor->Destroy();
            RootActor = nullptr;
        }
		this->RootActor = new PLevelActor();
        this->RootActor->SetActorScale(Vector3(1,1,1));
        this->Controller = new PController();
        this->Controller->SetIsTickable(TRUE);
        this->Controller->SetName("Controller");
        this->AddActor(Controller);
        // this->Controller->BeginAddedToScene(this);

        this->Controller->Init();
        this->RootActor->Init();
        this->RootActor->BeginAddedToScene(this);
    }

    void PWorld::Uninit()
    {
        // PObject::Uninit();
        this->Destroy();
    }

    void PWorld::Tick(FLOAT deltaTime)
    {
#if _EDITOR_ONLY
        EditorTick(deltaTime);
        return;
#endif
        FixTick(deltaTime);
        
    }

    void PWorld::EditorTick(FLOAT deltaTime)
    {
        RootActor->EditorTick(deltaTime);
        Controller->EditorTick(deltaTime);
    }

    void PWorld::FixTick(FLOAT deltaTime)
    {
        RootActor->FixedTick(deltaTime);
        Controller->FixedTick(deltaTime);
    }

    void PWorld::Destroy()
    {
        if(RootActor)
        {
            // RootActor->DestroyActorsAttached();
            RootActor->Destroy();
            RootActor = nullptr;
        }
        // AllActors.Clear();
        PObject::Destroy();
    }

    void PWorld::AddActor(PActor* NewActor, const ETransform& Trans)
    {
		PE_CHECK(ENGINE_WORLD_ERROR, "PWorld::AddActor : Adding nullptr to world.", NewActor != nullptr);
		PE_CHECK(ENGINE_WORLD_ERROR, "PWorld::AddActor : Adding world root to world.", NewActor != RootActor);
        
        
        NewActor->AttachToActor(RootActor);
        NewActor->GetRootComponent()->SetComponentWorldTransform(Trans);
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
#endif

	PigeonEngine::PController* PWorld::GetController() const
	{
        return this->Controller;
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
        this->SetSelectedComponent(this->ImguiSelectedActor ? this->ImguiSelectedActor->GetRootComponent() : nullptr);

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