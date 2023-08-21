#pragma once
#include "Array.h"
#include "EngineCommon.h"
#include "Base/DataStructure/Pointer/SharedPtr.h"

namespace PigeonEngine
{
    // -------------------   Declarations   -----------------------------
    // To describe the node's boundary
    struct EOctreeBounds
    {
        Vector3 Center;
        Vector3 Extends;

        EOctreeBounds(const Vector3& InCenter, const Vector3& InExtends);

        EOctreeBounds(const EBoundAABB& InBounds);

        ENGINE_NODISCARD EBoundAABB ToBoundAABB()const;

        ENGINE_NODISCARD Vector3 GetMax() const;
        ENGINE_NODISCARD Vector3 GetMin() const;

        ENGINE_NODISCARD BOOL IsPointInBounds(const Vector3& Point) const;
        ENGINE_NODISCARD BOOL IsIntercectingWith(const EOctreeBounds& OtherBounds) const;
    };

    // Content in octree's node, can be anything
    template <typename ContentType>
    class TOctreeNodeContent
    {
    public:
        TOctreeNodeContent<ContentType>() = delete;

    public:
        TSharedPtr<ContentType>& GetContent() const;
        void                     SetContent(const TSharedPtr<ContentType>& InContent);
    private:
        TSharedPtr<ContentType> Content;
        
    };

    // Octree's node
    template <typename ContentType>
    class TOctreeNode
    {
    public:
        TOctreeNode<ContentType>() = delete;
        explicit TOctreeNode<ContentType>(const EOctreeBounds& InBounds);
    protected:
        void CreateChildren(TOctreeNode<ContentType>* Whitch);
    private:
        TOctreeNode<ContentType>* Parent;
        TArray<TSharedPtr<TOctreeNode<ContentType>>> Children;
    public:
        ENGINE_NODISCARD const EOctreeBounds& GetBounds()const;
    private:
        EOctreeBounds Bounds;

    public:
        TSharedPtr<ContentType>& GetContent() const;
        void                     SetContent(const TSharedPtr<ContentType>& InContent);
    private:
        TSharedPtr<TOctreeNodeContent<ContentType>> Content;
    };

    // The octree
    template <typename ContentType>
    class TOctree
    {
        
    };

    // -------------------   Implementations   -----------------------------
    template <typename ContentType>
    TSharedPtr<ContentType>& TOctreeNodeContent<ContentType>::GetContent() const
    {
        return Content;
    }

    template <typename ContentType>
    void TOctreeNodeContent<ContentType>::SetContent(const TSharedPtr<ContentType>& InContent)
    {
        this->Content = InContent;
    }

    template <typename ContentType>
    TOctreeNode<ContentType>::TOctreeNode(const EOctreeBounds& InBounds)
        :
    Bounds(InBounds)
    {
        
    }

    template <typename ContentType>
    void TOctreeNode<ContentType>::CreateChildren(TOctreeNode<ContentType>* Whitch)
    {
        Vector3 Center = Whitch->GetBounds().Center ;

        for (int i = 0; i < 8; ++i) {
            Vector3 ChildMin = {i & 1 ? Center.x : Whitch->GetBounds().GetMin().x,
                                i & 2 ? Center.y : Whitch->GetBounds().GetMin().y,
                                i & 4 ? Center.z : Whitch->GetBounds().GetMin().z};
            Vector3 ChildMax = {i & 1 ? Whitch->GetBounds().GetMax().x : Center.x,
                                i & 2 ? Whitch->GetBounds().GetMax().y : Center.y,
                                i & 4 ? Whitch->GetBounds().GetMax().z : Center.z};
            Whitch->Children.Add(TSharedPtr<TOctreeNode<ContentType>>::MakeShared(new TOctreeNode<ContentType>({ChildMin, ChildMax})));
        }
    }

    template <typename ContentType>
    const EOctreeBounds& TOctreeNode<ContentType>::GetBounds() const
    {
        return this->Bounds;
    }

    template <typename ContentType>
    TSharedPtr<ContentType>& TOctreeNode<ContentType>::GetContent() const
    {
        return this->Content ? this->Content->GetContent():nullptr;
    }

    template <typename ContentType>
    void TOctreeNode<ContentType>::SetContent(const TSharedPtr<ContentType>& InContent)
    {
        if(!this->Content)
        {
            this->Content = TSharedPtr<TOctreeNodeContent<ContentType>>::MakeShared(new TOctreeNodeContent<ContentType>(InContent));
            return;
        }
        this->Content->SetContent(InContent);
    }
}
 