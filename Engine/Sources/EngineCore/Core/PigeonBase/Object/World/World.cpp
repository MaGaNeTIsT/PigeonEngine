#include "World.h"
namespace PigeonEngine
{
    
    ESceneTreeNode::~ESceneTreeNode()
    {
    }

    EBoundAABB ESceneTreeNode::GetBoundBox() const
    {
        return Box;
    }

    BOOL ESceneTreeNode::IsInBox(const EBoundAABB& Other) const
    {
        if(!Box.IsValid)
            return false;
        return Box.AABBMin.x > Other.AABBMin.x && Box.AABBMin.y > Other.AABBMin.y && Box.AABBMin.z > Other.AABBMin.z && Box.AABBMax.x < Other.AABBMax.x && Box.AABBMax.y < Other.AABBMax.y && Box.AABBMax.z < Other.AABBMax.z;
    }

    void PWorld::AddActor(PActor* NewActor, const ETransform& Trans)
    {
        
        Check(ENGINE_WORLD_ERROR, "TOctreeNode<T>::GetContent : Content is not valid, so you can not get it.", NewActor==nullptr);
        
    }

    void ESceneTreeNode::SetBoundBox(const EBoundAABB& InBox)
    {
        Box = InBox;
    }
};