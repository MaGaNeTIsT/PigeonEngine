#include "OctTree.h"
namespace PigeonEngine
{
    template <typename T>
    TOctTreeNodeContent<T>::TOctTreeNodeContent()
    {
        
    }

    template <typename T>
    TOctTreeNodeContent<T>::~TOctTreeNodeContent()
    {
        Content.Clear();
    }

    template <typename T>
    TOctTreeNodeContent<T>::TOctTreeNodeContent(const TArray<TSharedPtr<T>>& InContent)
        :
    Content(InContent)
    {
        
    }

    template <typename T>
    TArray<TSharedPtr<T>>& TOctTreeNodeContent<T>::GetContent()
    {
        return Content;
    }

    template <typename T>
    void TOctTreeNodeContent<T>::SetContent(const TArray<TSharedPtr<T>>& InContent)
    {
        this->Content = InContent;
    }

    template <typename T>
    void TOctTreeNodeContent<T>::AddItem(const TSharedPtr<T>& InItem)
    {
        Content.Add(InItem);
    }

    template <typename T>
    void TOctTreeNodeContent<T>::RemoveItem(const TSharedPtr<T>& InItem)
    {
        Content.Remove(InItem);
    }

    template <typename T>
    TOctTreeNode<T>::TOctTreeNode(const TSharedPtr<TOctTreeNode<T>>& InParent, const UINT& CurrentDepth,
        const UINT& MaxDepth, const TArray<TSharedPtr<TOctTreeNode<T>>>& InContent)
        :
    Parent(InParent),
    Content(InContent)
    {
        if(CurrentDepth < MaxDepth)
        {
            for(int i = 0; i < 8; ++i)
            {
                TSharedPtr<TOctTreeNode<T>> NewNode = TSharedPtr<T>::MakeShared(new TOctTreeNode<T>(this, CurrentDepth + 1, MaxDepth));
            }
        }
    }

    template <typename T>
    BOOL TOctTreeNode<T>::IsLeafNode() const
    {
        return Children.Length() == 0;
    }

    template <typename T>
    BOOL TOctTreeNode<T>::IsRootNode() const
    {
        return Parent.IsValid();
    }

    template <typename T>
    TArray<TSharedPtr<T>>& TOctTreeNode<T>::GetContent()
    {
        return Content.GetContent();
    }

    template <typename T>
    void TOctTreeNode<T>::SetContent(const TArray<TSharedPtr<T>>& InContent)
    {
        Content.SetContent(InContent);
    }

    template <typename T>
    void TOctTreeNode<T>::AddItemToContent(const TSharedPtr<T>& InItem)
    {
        Content.AddItem(InItem);
    }

    template <typename T>
    void TOctTreeNode<T>::RemoveItemInContent(const TSharedPtr<T>& InItem)
    {
        Content.RemoveItem(InItem);
    }

    template <typename T>
    TOctTree<T>::TOctTree(const UINT& InDepth)
        :
    Depth(InDepth)
    {
        if(Depth < 1)
        {
            return;
        }
        Root = TSharedPtr<T>::MakeShared(new TOctTreeNode<T>(nullptr, 1, Depth));
    }

    template <typename T>
    BOOL TOctTree<T>::IsValidTree() const
    {
        return Root.IsValid();
    }
}
