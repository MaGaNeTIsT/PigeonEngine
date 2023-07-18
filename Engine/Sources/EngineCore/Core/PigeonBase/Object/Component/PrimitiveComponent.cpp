#include "PrimitiveComponent.h"

namespace PigeonEngine
{
    
    PPrimitiveComponent::PPrimitiveComponent()
    {
        Bounds = EBoundAABB(Vector3(0), Vector3(0), false);
    }
    EBoundAABB PPrimitiveComponent::GetBounds() const
    {
        return Bounds;
    }

}
