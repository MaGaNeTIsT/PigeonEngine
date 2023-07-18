#pragma once
#include "Array.h"
#include "Base/DataStructure/Pointer/SharedPtr.h"

namespace PigeonEngine
{
    template <typename T>
    class TOctTreeNodeContent
    {
    public:
        TOctTreeNodeContent();
        ~TOctTreeNodeContent();
        explicit TOctTreeNodeContent(const TArray<TSharedPtr<T>>& InContent);
        
    public:
        TArray<TSharedPtr<T>>& GetContent();
        void                   SetContent(const TArray<TSharedPtr<T>>& InContent);
        
        void AddItem   (const TSharedPtr<T>& InItem);
        void RemoveItem(const TSharedPtr<T>& InItem);
    private:
        TArray<TSharedPtr<T>> Content;
    };

    
    template <typename T>
    class TOctTreeNode
    {
    public:
        TOctTreeNode() = delete;
        TOctTreeNode(const TSharedPtr<TOctTreeNode<T>>& InParent, const UINT& CurrentDepth, const UINT& MaxDepth, const TArray<TSharedPtr<TOctTreeNode<T>>>& InContent = {});
        
    public:
        ENGINE_NODISCARD BOOL IsLeafNode() const;
        ENGINE_NODISCARD BOOL IsRootNode() const;

    private:
        TSharedPtr<TOctTreeNode<T>>         Parent = nullptr;
        TArray<TSharedPtr<TOctTreeNode<T>>> Children;
        
    public:
        TArray<TSharedPtr<T>>& GetContent();
        void                   SetContent(const TArray<TSharedPtr<T>>& InContent);
        
        void AddItemToContent   (const TSharedPtr<T>& InItem);
        void RemoveItemInContent(const TSharedPtr<T>& InItem);
    private:
        TOctTreeNodeContent<T>  Content;
    };

    
    template <typename T>
    class TOctTree
    {
    public:
        TOctTree() = delete;
        explicit TOctTree(const UINT& InDepth);

    public:
        ENGINE_NODISCARD BOOL IsValidTree() const;
    private:
        TSharedPtr<TOctTreeNode<T>> Root = nullptr;
        UINT Depth = 0;
    };

}
