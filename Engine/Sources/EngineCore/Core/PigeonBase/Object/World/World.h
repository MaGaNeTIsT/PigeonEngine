#pragma once
#include <CoreMinimal.h>

#include "Base/DataStructure/Container/Array.h"
// #include "Base/DataStructure/Container/Set.h"
#include "PigeonBase/Object/Object.h"

namespace PigeonEngine
{
    class PScene;
    class PActor;
    enum EWorldType : UINT8
    {
        EWT_GAME,
        EWT_EDITOR
    };
    
    /*
     *
     */
    class PWorld : public PObject
    {
        EClass(PWorld, PObject)
    public:
        CLASS_VIRTUAL_NOCOPY_BODY(PWorld)
        
        void AddSceneToWorld(PScene* NewScene, const ETransform& RelativeTransform = ETransform());
        void AddActorToWorld(PActor* NewActor, const ETransform& RelativeTransform = ETransform());
    private:
        // Current operating scene.(in editor)
        PScene* CurrentScene    = nullptr;
        // Like unreal's persistent scene.
        PScene* PersistentScene = nullptr;
        // all the loaded scene.
        TArray<PScene*> AllScenes;
    };

};
