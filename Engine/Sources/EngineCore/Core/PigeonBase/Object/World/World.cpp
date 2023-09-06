#include "World.h"

#include "../../../../Main/MainManager.h"
#include <Renderer/RenderInterface.h>
#include <PigeonBase/Object/Component/SceneComponent.h>
#include <PigeonBase/Object/Component/PrimitiveComponent.h>

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
    void PWorld::AddStaticPrimitive(PPrimitiveComponent* InPrimitiveComponent)
    {
        Check((ENGINE_RENDER_CORE_ERROR), ("Try adding static primitive to scene, but render scene is null."), (!!RenderScene));
#if _EDITOR_ONLY
        if (RenderScene)
#endif
        {
            RenderScene->AddStaticPrimitive(InPrimitiveComponent);
        }
    }
    void PWorld::RemoveStaticPrimitive(PPrimitiveComponent* InPrimitiveComponent)
    {
        Check((ENGINE_RENDER_CORE_ERROR), ("Try adding static primitive to scene, but render scene is null."), (!!RenderScene));
#if _EDITOR_ONLY
        if (RenderScene)
#endif
        {
            RenderScene->RemoveStaticPrimitive(InPrimitiveComponent);
        }
    }
    void PWorld::AddDynamicPrimitive(PPrimitiveComponent* InPrimitiveComponent)
    {
        Check((ENGINE_RENDER_CORE_ERROR), ("Try adding static primitive to scene, but render scene is null."), (!!RenderScene));
#if _EDITOR_ONLY
        if (RenderScene)
#endif
        {
            RenderScene->AddDynamicPrimitive(InPrimitiveComponent);
        }
    }
    void PWorld::RemoveDynamicPrimitive(PPrimitiveComponent* InPrimitiveComponent)
    {
        Check((ENGINE_RENDER_CORE_ERROR), ("Try adding static primitive to scene, but render scene is null."), (!!RenderScene));
#if _EDITOR_ONLY
        if (RenderScene)
#endif
        {
            RenderScene->RemoveDynamicPrimitive(InPrimitiveComponent);
        }
    }
    void PWorld::UpdateDynamicPrimitive(PPrimitiveComponent* InPrimitiveComponent)
    {
        Check((ENGINE_RENDER_CORE_ERROR), ("Try adding static primitive to scene, but render scene is null."), (!!RenderScene));
#if _EDITOR_ONLY
        if (RenderScene)
#endif
        {
            RenderScene->UpdateDynamicPrimitive(InPrimitiveComponent);
        }
    }
    //Render scene state END

};