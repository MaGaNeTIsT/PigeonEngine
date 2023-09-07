#include "StaticMeshSceneProxy.h"
#include <PigeonBase/Object/Component/StaticMeshComponent.h>

namespace PigeonEngine
{

    static void RegisterClassTypes()
    {
        RegisterClassType<RStaticMeshSceneProxy, RMeshSceneProxy>();
    }

    PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

    RStaticMeshSceneProxy::RStaticMeshSceneProxy(PStaticMeshComponent* InComponent)
    {
    }

};