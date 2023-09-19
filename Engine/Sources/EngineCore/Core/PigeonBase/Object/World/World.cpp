#include "World.h"
#include "../../../../Main/MainManager.h"
#include <Renderer/RenderInterface.h>
#include <PigeonBase/Object/Component/SceneComponent.h>
#include <PigeonBase/Object/Component/Primitive/PrimitiveComponent.h>

#include "PigeonBase/Object/Actor/LevelActor.h"

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
       
    }
    PWorld::~PWorld()
    {
    }

    void PWorld::Init()
    {
        GameTimer = EMainManager::GetManagerSingleton()->GetGameTimer();
		this->RootActor = new PActor();
		this->RootActor->BeginAddedToScene(this);
        this->RootActor->Init();
		this->LevelScriptor = new PLevelActor();
		this->LevelScriptor->BeginAddedToScene(this);
        this->LevelScriptor->Init();
    }

    void PWorld::Uninit()
    {
        PObject::Uninit();
    }

    void PWorld::Tick(FLOAT deltaTime)
    {
        if (!GameTimer)
        {
            GameTimer = EMainManager::GetManagerSingleton()->GetGameTimer();
        }
        RootActor->FixedTick(static_cast<FLOAT>(GameTimer->GetDeltaTime()));

        LevelScriptor->FixedTick(static_cast<FLOAT>(GameTimer->GetDeltaTime()));
    }

    void PWorld::EditorTick(FLOAT deltaTime)
    {
    }

  
    void PWorld::Destroy()
    {
        PObject::Destroy();
    }

    void PWorld::AddActor(PActor* NewActor, const ETransform& Trans)
    {
		Check(ENGINE_WORLD_ERROR, "PWorld::AddActor : Adding nullptr to world.", NewActor != nullptr);
		Check(ENGINE_WORLD_ERROR, "PWorld::AddActor : Adding world root to world.", NewActor != RootActor);
        
        
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

    void PWorldManager::Init()
    {
        World = new PWorld();
    }

    void PWorldManager::Uninit()
    {
        if(World)
        {
            World->Destroy();
            World = nullptr;
        }
    }

    //Render scene state END

    PWorldManager::PWorldManager()
    {
       
    }
    PWorldManager::~PWorldManager()
    {
    }

};