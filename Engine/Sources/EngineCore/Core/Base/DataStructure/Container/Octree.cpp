#include "Octree.h"
namespace PigeonEngine
{
    EOctreeBounds::EOctreeBounds(const Vector3& InCenter, const Vector3& InExtends)
        :
    Center(InCenter), Extends(InExtends)
    {
    }

    EOctreeBounds::EOctreeBounds(const EBoundAABB& InBounds)
        :
    Center((InBounds.AABBMax + InBounds.AABBMin) * 0.5), Extends((InBounds.AABBMax - InBounds.AABBMin) * 0.5)
    {
         
    }

    EBoundAABB EOctreeBounds::ToBoundAABB() const
    {

        return EBoundAABB(this->Center - this->Extends, this->Center + this->Extends, TRUE);
    }

    Vector3 EOctreeBounds::GetMax() const
    {
        return this->Center + this->Extends;
    }

    Vector3 EOctreeBounds::GetMin() const
    {
        return this->Center - this->Extends;
    }

    BOOL EOctreeBounds::IsPointInBounds(const Vector3& Point) const
    {
        const Vector3 Min = GetMin();
        const Vector3 Max = GetMax();

        return Point.x >= Min.x && Point.x <= Max.x &&
               Point.y >= Min.y && Point.y <= Max.y &&
               Point.z >= Min.z && Point.z <= Max.z;
    }

    BOOL EOctreeBounds::IsIntercectingWith(const EOctreeBounds& OtherBounds) const
    {
        const Vector3 Min = GetMin();
        const Vector3 Max = GetMax();
        const Vector3 OtherMin = OtherBounds.GetMin();
        const Vector3 OtherMax = OtherBounds.GetMax();
        return !(OtherMax.x < Min.x || OtherMin.x > Max.x ||
                 OtherMax.y < Min.y || OtherMin.y > Max.y ||
                 OtherMax.z < Min.z || OtherMin.z > Max.z);
        
    }
}
