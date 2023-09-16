#include "World.h"
#include "../../../../Main/MainManager.h"
#include <Renderer/RenderInterface.h>
#include <PigeonBase/Object/Component/SceneComponent.h>
#include <PigeonBase/Object/Component/Primitive/PrimitiveComponent.h>

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
    }

    void PWorld::Uninit()
    {
        PObject::Uninit();
    }

    void PWorld::Tick(FLOAT deltaTime)
    {
        Check(ENGINE_WORLD_ERROR, "PWorld::AddActor : Adding nullptr to world.", GameTimer == nullptr);
        for(const auto& Actor : AllActors)
        {
            Actor->FixedTick(static_cast<FLOAT>(GameTimer->GetDeltaTime()));
        }
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
        Check(ENGINE_WORLD_ERROR, "PWorld::AddActor : Adding nullptr to world.", NewActor == nullptr);
        
        this->RootActors.Add(NewActor);
        this->AllActors.Add(NewActor);
        NewActor->GetRootComponent()->SetComponentWorldTransform(Trans);
        NewActor->BeginAddedToScene();
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
    //Render scene state END

    PWorldManager::PWorldManager()
    {
    }
    PWorldManager::~PWorldManager()
    {
    }

};