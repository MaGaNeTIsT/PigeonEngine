#pragma once
#include "Array.h"
#include "Base/DataStructure/Pointer/SharedPtr.h"

namespace PigeonEngine
{
    template <typename T>
    class TOctreeNodeContent
    {
    public:
        TOctreeNodeContent();
        ~TOctreeNodeContent();
        explicit TOctreeNodeContent(const TArray<T>& InContent);
        
    public:
        ENGINE_NODISCARD UINT GetContentNum() const;
        
        TArray<T>& GetContent();
        void       SetContent(const TArray<T>& InContent);
        
        void AddItem   (const T& InItem);
        void RemoveItem(const T& InItem);
    private:
        TArray<T> Content;
    };

    
    template <typename T>
    class TOctreeNode
    {
    public:
        TOctreeNode() = delete;
        TOctreeNode(const TSharedPtr<TOctreeNode<T>>& InParent, const UINT& CurrentDepth, const UINT& MaxDepth, const UINT& IndexInSibling);
        TOctreeNode(const TSharedPtr<TOctreeNode<T>>& InParent, const UINT& IndexInSibling);

    public:
        
        ENGINE_NODISCARD BOOL IsRootNode() const;
    private:
        TSharedPtr<TOctreeNode<T>>         Parent = nullptr;
    public:
        ENGINE_NODISCARD BOOL IsLeafNode() const;
        
        BOOL AddChildNode();
    private:
        TArray<TSharedPtr<TOctreeNode<T>>> Children;

    public:
        ENGINE_NODISCARD const TArray<UINT>& GetCoordinate()const;
        TSharedPtr<TOctreeNode<T>> GetNode(const TArray<UINT>& Coordinate)const;
    private:
        TArray<UINT> Coordinate;
        
    public:
        ENGINE_NODISCARD BOOL HasContent() const;
        
        TArray<T>& GetContent();
        void       SetContent(const TArray<T>& InContent);
        
        void AddItemToContent   (const T& InItem);
        void RemoveItemInContent(const T& InItem);
    private:
        TSharedPtr<TOctreeNodeContent<T>>  Content;
    };

    
    template <typename T>
    class TOctree
    {
    public:
        TOctree() = default;
        explicit TOctree(const UINT& InDepth);

    public:
        ENGINE_NODISCARD BOOL IsValidTree() const;
        TSharedPtr<TOctreeNode<T>> GetNode(const TArray<UINT>& Coordinate)const;
    private:
        TSharedPtr<TOctreeNode<T>> Root = nullptr;
        UINT Depth = 0;
    };

}
 