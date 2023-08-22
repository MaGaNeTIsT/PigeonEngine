#pragma once
#include <CoreMinimal.h>

#include "EngineCommon.h"
#include "Base/DataStructure/Container/Array.h"
#include "PigeonBase/Object/Actor.h"
#include "PigeonBase/Object/Object.h"

namespace PigeonEngine
{
    class PScene;
    class PActor;
    class EGameTimer;
    enum EWorldType : UINT8
    {
        EWT_GAME,
        EWT_EDITOR,
        EWT_RUN_IN_EDITOR, // maybe useless
    };
    
    /*
     * actual world in runtime
     */
    class PWorld final : public PObject
    {
    public:
        CLASS_VIRTUAL_NOCOPY_BODY(PWorld)
        
    // Begin PObject Interface
    public:
        void	Init() override;
        void	Uninit() override;
        void	Tick(FLOAT deltaTime) override;
#ifdef _EDITOR_ONLY
        void	EditorTick(FLOAT deltaTime) override;
#endif
    public:
        void Destroy() override;
    // End PObject Interface

    // Actors in world management
    public:
        void AddActor(PActor* NewActor, PActor* Parent = nullptr, const ETransform& Trans = ETransform());
    protected:
        void CheckRenderInfoWhenAddingActor(PActor* NewActor);
    private:
        TSet<PActor*> RootActors;
        TSet<PActor*> AllActors;
#if _EDITOR_ONLY
    public:
        void AddSceneToWorld(PScene* NewScene);
        void RemoveSceneFromWorld(PScene* Scene);
        void SetCurrentScene(PScene* Scene);
    private:
        // Current operating scene.(in editor)
        PScene* CurrentScene    = nullptr;
        // Like unreal's persistent scene.
        PScene* PersistentScene = nullptr;
        TArray<PScene*> Scenes;
#endif


    private:
        // Timer
        const EGameTimer* GameTimer = nullptr;
    };

};
