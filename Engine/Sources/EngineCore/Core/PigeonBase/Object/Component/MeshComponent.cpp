#include "MeshComponent.h"

namespace PigeonEngine
{
    PMeshComponent::PMeshComponent()
    {
        Bounds = EBoundAABB(Vector3(0), Vector3(0), false);
    }
    EBoundAABB PMeshComponent::GetBounds() const
    {
        return Bounds;
    }

}
