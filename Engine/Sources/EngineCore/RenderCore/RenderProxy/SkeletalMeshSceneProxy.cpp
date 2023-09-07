#include "SkeletalMeshSceneProxy.h"
#include <PigeonBase/Object/Component/SkeletalMeshComponent.h>

namespace PigeonEngine
{

    static void RegisterClassTypes()
    {
        RegisterClassType<RSkeletonMeshSceneProxy, RMeshSceneProxy>();
    }

    PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

    RSkeletonMeshSceneProxy::RSkeletonMeshSceneProxy(PSkeletalMeshComponent* InComponent)
    {
    }

};