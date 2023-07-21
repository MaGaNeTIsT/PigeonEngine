#include "Octree.h"
namespace PigeonEngine
{
    template <typename T>
    TOctreeNodeContent<T>::TOctreeNodeContent()
    {
        
    }

    template <typename T>
    TOctreeNodeContent<T>::~TOctreeNodeContent()
    {
        Content.Clear();
    }

    template <typename T>
    TOctreeNodeContent<T>::TOctreeNodeContent(const TArray<T>& InContent)
        :
    Content(InContent)
    {
        
    }

    template <typename T>
    UINT TOctreeNodeContent<T>::GetContentNum() const
    {
        return Content.Length();
    }

    template <typename T>
    TArray<T>& TOctreeNodeContent<T>::GetContent()
    {
        return Content;
    }

    template <typename T>
    void TOctreeNodeContent<T>::SetContent(const TArray<T>& InContent)
    {
        this->Content = InContent;
    }

    template <typename T>
    void TOctreeNodeContent<T>::AddItem(const T& InItem)
    {
        Content.Add(InItem);
    }

    template <typename T>
    void TOctreeNodeContent<T>::RemoveItem(const T& InItem)
    {
        Content.Remove(InItem);
    }

    

    template <typename T>
    TOctreeNode<T>::TOctreeNode(const TSharedPtr<TOctreeNode<T>>& InParent, const UINT& CurrentDepth,
        const UINT& MaxDepth, const UINT& IndexInSibling)
    {
       
        if(CurrentDepth < MaxDepth)
        {
            for(int i = 0; i < 8; ++i)
            {
                TSharedPtr<TOctreeNode<T>> NewNode = TSharedPtr<T>::MakeShared(new TOctreeNode<T>(this, CurrentDepth + 1, MaxDepth, i));
                this->Children.Add(NewNode);
            }
        }
        // Content = TSharedPtr<T>::MakeShared(new TOctreeNodeContent<T>());
        if(Parent)
        {
            this->Coordinate.Append(Parent->GetCoordinate());
        }
        this->Coordinate.Append(IndexInSibling);
    }

    template <typename T>
    TOctreeNode<T>::TOctreeNode(const TSharedPtr<TOctreeNode<T>>& InParent, const UINT& IndexInSibling)
    {
        if(Parent)
        {
            this->Coordinate.Append(Parent->GetCoordinate());
        }
        this->Coordinate.Append(IndexInSibling);
    }

    template <typename T>
    BOOL TOctreeNode<T>::IsLeafNode() const
    {
        return Children.Length() == 0;
        
    }

    template <typename T>
    BOOL TOctreeNode<T>::AddChildNode()
    {
        const UINT ChildrenCount = Children.Length();
        Check(ENGINE_OCTREE_ERROR, "TOctreeNode<T>::AddChildNode :this node's children >= 8, so you can not add more. ", ChildrenCount > 7);
        if(ChildrenCount == 8)
        {
            return false;
        }
        TSharedPtr<TOctreeNode<T>> NewNode = TSharedPtr<T>::MakeShared(new TOctreeNode<T>(this, ChildrenCount - 1));
        this->Children.Add(NewNode);
        return true;
    }

    template <typename T>
    BOOL TOctreeNode<T>::IsRootNode() const
    {
        return Parent.IsValid();
    }

    template <typename T>
    const TArray<UINT>& TOctreeNode<T>::GetCoordinate() const
    {
        return this->Coordinate;
    }

    template <typename T>
    TSharedPtr<TOctreeNode<T>> TOctreeNode<T>::GetNode(const TArray<UINT>& Coordinate) const
    {
        TArray<UINT> TargetCoordinate = Coordinate;
        const UINT Index = TargetCoordinate.Last();
        Check(ENGINE_OCTREE_ERROR, "TOctreeNode<T>::GetNode : At least one coordinate's element greater than 7", Index > 7);
        TargetCoordinate.Pop();
        if(TargetCoordinate.Length() > 0)
        {
           return Children[Index]->GetNode(TargetCoordinate);
        }
        else
        {
            return this;
        }
    }

    template <typename T>
    BOOL TOctreeNode<T>::HasContent() const
    {
        return Content.IsValid();
    }

    template <typename T>
    TArray<T>& TOctreeNode<T>::GetContent()
    {
        Check(ENGINE_OCTREE_ERROR, "TOctreeNode<T>::GetContent : Content is not valid, so you can not get it.", Content.IsValid());
        return Content->GetContent();
    }

    template <typename T>
    void TOctreeNode<T>::SetContent(const TArray<T>& InContent)
    {
        if(!Content.IsValid())
            Content = TSharedPtr<T>::MakeShared(new TOctreeNodeContent<T>());
        Content->SetContent(InContent);
    }

    template <typename T>
    void TOctreeNode<T>::AddItemToContent(const T& InItem)
    {
        if(!Content.IsValid())
            Content = TSharedPtr<T>::MakeShared(new TOctreeNodeContent<T>());
        Content->AddItem(InItem);
    }

    template <typename T>
    void TOctreeNode<T>::RemoveItemInContent(const T& InItem)
    {
        Check(ENGINE_OCTREE_ERROR, "TOctreeNode<T>::RemoveItemInContent : Content is not valid, so you can not remove item.", Content.IsValid());
        Content->RemoveItem(InItem);
        if(Content->GetContentNum() == 0)
        {
            Content = nullptr;
        }
    }

    template <typename T>
    TOctree<T>::TOctree(const UINT& InDepth)
        :
    Depth(InDepth)
    {
        Check(ENGINE_OCTREE_ERROR, "TOctree<T>::TOctree(const UINT& InDepth) : Are you sure you want a octree whitch depth is SMALLER than 1?", Depth < 1);
        Root = TSharedPtr<T>::MakeShared(new TOctreeNode<T>(nullptr, 1, Depth, 0));
    }

    template <typename T>
    BOOL TOctree<T>::IsValidTree() const
    {
        return Root.IsValid();
    }

    template <typename T>
    TSharedPtr<TOctreeNode<T>> TOctree<T>::GetNode(const TArray<UINT>& Coordinate) const
    {
        Check(ENGINE_OCTREE_ERROR, "TOctree<T>::GetNode : Octree has no valid root", Root);
        Check(ENGINE_OCTREE_ERROR, "TOctree<T>::GetNode : Octree has no such coordinates : Depth is larger than this tree.", Coordinate.Length() > Depth);
        
        return Root->GetNode(Coordinate.Reverse().Pop());
    }
}
