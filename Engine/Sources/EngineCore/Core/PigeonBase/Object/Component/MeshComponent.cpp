#include "MeshComponent.h"
#include <RenderProxy/MeshSceneProxy.h>

namespace PigeonEngine
{

    static void RegisterClassTypes()
    {
        RegisterClassType<PMeshComponent, PPrimitiveComponent>();
    }

    PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

    PMeshComponent::PMeshComponent()
    {
    }
    PMeshComponent::~PMeshComponent()
    {
    }

}