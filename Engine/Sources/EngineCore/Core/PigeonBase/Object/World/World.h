#pragma once
#include <CoreMinimal.h>

#include "EngineCommon.h"
#include "Base/DataStructure/Container/Array.h"
#include "Base/DataStructure/Container/Octree.h"
#include "PigeonBase/Object/Object.h"

namespace PigeonEngine
{
    class PScene;
    class PActor;
    
    class ESceneTreeNode : public TOctreeNode<PActor*>
    {
    public:
        ESceneTreeNode() = delete;
        ~ESceneTreeNode();
    public:
        EBoundAABB GetBoundBox() const;
        void SetBoundBox(const EBoundAABB& InBox);

        BOOL IsInBox(const EBoundAABB& Other) const;

    private:
        EBoundAABB Box;
    };
    
    class ESceneTree : public TOctree<PActor*>
    {
        
    };
    
   
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
        //EClass(PWorld, PObject)
    public:
        CLASS_VIRTUAL_NOCOPY_BODY(PWorld)
        
    public:
        
        void AddActor(PActor* NewActor, const ETransform& Trans = ETransform());
    private:
        ESceneTree* StaticTree = nullptr;

        ESceneTree* DynamicTree = nullptr;
        // all the loaded scene.
        TArray<PScene*> AllScenes;

#if _EDITOR_ONLY
        // Current operating scene.(in editor)
        PScene* CurrentScene    = nullptr;
        // Like unreal's persistent scene.
        PScene* PersistentScene = nullptr;
#endif

       
    };

};
