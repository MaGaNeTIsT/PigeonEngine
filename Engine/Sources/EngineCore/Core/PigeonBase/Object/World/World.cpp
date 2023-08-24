#include "World.h"

#include "../../../../Main/MainManager.h"
#include "PigeonBase/Object/Component/SceneComponent.h"

namespace PigeonEngine
{
    static void RegisterClassTypes()
    {
        RegisterClassType<PWorld, PObject>();
        // RegisterClassType<ESceneTreeNode, TOctreeNode<PActor*>>();
    }

    PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

    void PWorld::Init()
    {
        PObject::Init();
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

    void PWorld::AddActor(PActor* NewActor, PActor* Parent, const ETransform& Trans)
    {
        Check(ENGINE_WORLD_ERROR, "PWorld::AddActor : Adding nullptr to world.", NewActor == nullptr);

        if(!Parent)
        {
            this->RootActors.Add(NewActor);
            this->AllActors.Add(NewActor);
            NewActor->GetRootComponent()->SetComponentWorldTransform(Trans);
            CheckRenderInfoWhenAddingActor(NewActor);
            return;
        }

        NewActor->AttachToActor(Parent, Trans);
        CheckRenderInfoWhenAddingActor(NewActor);
    }

    void PWorld::CheckRenderInfoWhenAddingActor(PActor* NewActor)
    {
        // to be done
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
};