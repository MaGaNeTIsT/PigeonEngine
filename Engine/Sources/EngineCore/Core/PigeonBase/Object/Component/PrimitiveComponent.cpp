#include "PrimitiveComponent.h"

namespace PigeonEngine
{

    static void RegisterClassTypes()
    {
        RegisterClassType<PPrimitiveComponent, PSceneComponent>();
    }

    PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

    PPrimitiveComponent::PPrimitiveComponent()
    {
        Bounds = EBoundAABB(Vector3(0), Vector3(0), false);
    }

	PPrimitiveComponent::~PPrimitiveComponent()
	{

	}

    EBoundAABB PPrimitiveComponent::GetBounds() const
    {
        return Bounds;
    }

}
