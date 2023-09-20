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
        
		this->RootActor = new PActor();
        this->RootActor->SetIsTickable(TRUE);
		this->RootActor->BeginAddedToScene(this);
        this->RootActor->Init();

        this->Controller = new PController();
        this->Controller->SetIsTickable(TRUE);
        this->Controller->BeginAddedToScene(this);
        this->Controller->Init();

		this->LevelScriptor = new PLevelActor();
        this->LevelScriptor->SetIsTickable(TRUE);
		this->LevelScriptor->BeginAddedToScene(this);
        this->LevelScriptor->Init();


    }

    void PWorld::Uninit()
    {
        PObject::Uninit();
    }

    void PWorld::Tick(FLOAT deltaTime)
    {
        RootActor->FixedTick(deltaTime);
        LevelScriptor->FixedTick(deltaTime);
        this->Controller->FixedTick(deltaTime);
    }

    void PWorld::EditorTick(FLOAT deltaTime)
    {
        RootActor->EditorTick(deltaTime);

        LevelScriptor->EditorTick(deltaTime);
    }

  
    void PWorld::Destroy()
    {
        LevelScriptor->Destroy();
        RootActor->Destroy();
        Controller->Destroy();
        PObject::Destroy();
    }

    void PWorld::AddActor(PActor* NewActor, const ETransform& Trans)
    {
		PE_CHECK(ENGINE_WORLD_ERROR, "PWorld::AddActor : Adding nullptr to world.", NewActor != nullptr);
		PE_CHECK(ENGINE_WORLD_ERROR, "PWorld::AddActor : Adding world root to world.", NewActor != RootActor);
        
        
        NewActor->AttachToActor(RootActor);
        NewActor->GetRootComponent()->SetComponentWorldTransform(Trans);
        NewActor->BeginAddedToScene(this);
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



};