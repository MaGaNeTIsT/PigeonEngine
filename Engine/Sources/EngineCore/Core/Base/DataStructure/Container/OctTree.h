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
        explicit TOctTreeNodeContent(const TArray<T>& InContent);
        
    public:
        TArray<T>& GetContent();
        void       SetContent(const TArray<T>& InContent);
        
        void AddItem   (const T& InItem);
        void RemoveItem(const T& InItem);
    private:
        TArray<T> Content;
    };

    
    template <typename T>
    class TOctTreeNode
    {
    public:
        TOctTreeNode() = delete;
        TOctTreeNode(const TSharedPtr<TOctTreeNode<T>>& InParent, const UINT& CurrentDepth, const UINT& MaxDepth);
        
    public:
        ENGINE_NODISCARD BOOL IsLeafNode() const;
        ENGINE_NODISCARD BOOL IsRootNode() const;

    private:
        TSharedPtr<TOctTreeNode<T>>         Parent = nullptr;
        TArray<TSharedPtr<TOctTreeNode<T>>> Children;
        
    public:
        TArray<T>& GetContent();
        void       SetContent(const TArray<T>& InContent);
        
        void AddItemToContent   (const T& InItem);
        void RemoveItemInContent(const T& InItem);
    private:
        TSharedPtr<TOctTreeNodeContent<T>>  Content;
    };

    
    template <typename T>
    class TOctTree
    {
    public:
        TOctTree() = default;
        explicit TOctTree(const UINT& InDepth);

    public:
        ENGINE_NODISCARD BOOL IsValidTree() const;
    private:
        TSharedPtr<TOctTreeNode<T>> Root = nullptr;
        UINT Depth = 0;
    };

}
