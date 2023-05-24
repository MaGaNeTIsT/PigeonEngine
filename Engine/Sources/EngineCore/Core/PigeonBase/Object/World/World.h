#pragma once
#include <CoreMinimal.h>

#include "Base/DataStructure/Container/Set.h"
#include "PigeonBase/Object/Object.h"

namespace PigeonEngine
{
    class PScene;
    class PWorldNode : PObject
    {
        EClass(PWorldNode, PObject)
     public:
        CLASS_VIRTUAL_NOCOPY_BODY(PWorldNode)

    private:
        PScene* Scene = nullptr;
    };
    
    class PWorld : public PObject
    {
        EClass(PWorld, PObject)
    public:
        CLASS_VIRTUAL_NOCOPY_BODY(PWorld)

        void AddSceneToWorld(PScene* NewScene, const ETransform& RelativeTransform = ETransform());
    private:
        TSet<PWorldNode> Nodes;
    };

};
