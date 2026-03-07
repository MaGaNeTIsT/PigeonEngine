#pragma once

#include <CoreMinimal.h>
#include <EngineCommon.h>

namespace PigeonEngine
{

	class RPrimitiveSceneProxy;

	struct ROctreeElement
	{
		ROctreeElement()noexcept : NodeIndex(-1) {}
		ROctreeElement(const ROctreeElement& Other)noexcept : NodeIndex(Other.NodeIndex), Primitives(Other.Primitives), PrimitiveMapping(Other.PrimitiveMapping) {}
		ROctreeElement& operator=(const ROctreeElement& Other)
		{
			NodeIndex			= Other.NodeIndex;
			Primitives			= Other.Primitives;
			PrimitiveMapping	= Other.PrimitiveMapping;
			return (*this);
		}

		INT32								NodeIndex;
		TArray<const RPrimitiveSceneProxy*>	Primitives;
		TMap<ObjectIdentityType, INT32>		PrimitiveMapping;
	};

	struct ROctreeNode
	{
		ROctreeNode()noexcept : Origin(Vector3::Zero()), ElementIndex(-1), ParentIndex(-1) {}
		ROctreeNode(const ROctreeNode& Other)noexcept : Origin(Other.Origin), ElementIndex(Other.ElementIndex), ParentIndex(Other.ParentIndex), ChildrenIndex(Other.ChildrenIndex) {}
		ROctreeNode& operator=(const ROctreeNode& Other)
		{
			ChildrenIndex.Empty();
			Origin			= Other.Origin;
			ElementIndex	= Other.ElementIndex;
			ParentIndex		= Other.ParentIndex;
			ChildrenIndex	= Other.ChildrenIndex;
			return (*this);
		}

		Vector3			Origin;
		INT32			ElementIndex;
		INT32			ParentIndex;
		TArray<INT32>	ChildrenIndex;
	};

	struct ROctreeLayerInfo
	{
		ROctreeLayerInfo()noexcept : NodeNum(0), Extent(Vector3::Zero()), StartIndex(-1) {}
		ROctreeLayerInfo(const ROctreeLayerInfo& Other)noexcept : NodeNum(Other.NodeNum), Extent(Other.Extent), StartIndex(Other.StartIndex) {}
		ROctreeLayerInfo& operator=(const ROctreeLayerInfo& Other)
		{
			NodeNum		= Other.NodeNum;
			Extent		= Other.Extent;
			StartIndex	= Other.StartIndex;
			return (*this);
		}

		INT32		NodeNum;
		Vector3		Extent;
		INT32		StartIndex;
	};

	class ROctree final : public EObjectBase
	{
	public:
		enum EOctreeAxisType : UINT8
		{
			OCTREE_AXIS_X		= 0,
			OCTREE_AXIS_Y,
			OCTREE_AXIS_Z,
			OCTREE_AXIS_COUNT
		};
	public:
		static BOOL32	SplitOctreeInternal(const Vector3& InOrigin, const Vector3& InExtent, const Vector3& InMinExtent, Vector3& OutOctreeOrigin, Vector3& OutOctreeSize, Vector3& OutOctreeCellSize, TArray<UINT32>& OutOctreePerAxisCellNum, TArray<UINT32>& OutOctreeAxisDepth, UINT32& OutOctreeMaxDepth, TArray<ROctreeLayerInfo>& OutOctreeLayerInfos, TArray<ROctreeElement>& OutOctreeElements, TArray<ROctreeNode>& OutOctreeNodes);
	public:
		BOOL32	AddPrimitive(const RPrimitiveSceneProxy* InSceneProxy);
		BOOL32	AddPrimitives(const TArray<RPrimitiveSceneProxy*>& InPrimitives, TArray<INT32>& OutErrorPrimitives);
		BOOL32	RemovePrimitive(const RPrimitiveSceneProxy* InSceneProxy);
		BOOL32	RemovePrimitives(const TArray<RPrimitiveSceneProxy*>& InPrimitives, TArray<INT32>& OutErrorPrimitives);
		UINT32	GetPrimitiveNum()const;
		void	ClearPrimitives();
		BOOL32	RebuildOctreeForWholeLevel(const EBoundAABB* InBounds);
		BOOL32	FinalizeOctree();
	public:
		template<typename _TCustomType, typename _TLambdaType, typename _TConditionType>
		void BackwardRecursionNode(INT32 InNodeIndex, _TCustomType InCustomValue, const _TLambdaType& InFunc, const _TConditionType& InCond)
		{
			if ((InNodeIndex < 0) || (InNodeIndex >= (Nodes.Num())))
			{
				return;
			}
			ROctreeNode& TempNode = Nodes[InNodeIndex];
			if (InCond(TempNode, InCustomValue))
			{
				_TCustomType NewCustomValue = InFunc(TempNode, InCustomValue);
				if (TempNode.ChildrenIndex.Num() > 0)
				{
					for (INT32 ChildIndex = 0, ChildNum = TempNode.ChildrenIndex.Num(); ChildIndex < ChildNum; ChildIndex++)
					{
						BackwardRecursionNode<_TCustomType, _TLambdaType, _TConditionType>(TempNode.ChildrenIndex[ChildIndex], NewCustomValue, InFunc, InCond);
					}
				}
			}
		}
		template<typename _TCustomType, typename _TLambdaType, typename _TConditionType>
		void BackwardRecursionNode(INT32 InNodeIndex, _TCustomType InCustomValue, const _TLambdaType& InFunc, const _TConditionType& InCond)const
		{
			if ((InNodeIndex < 0) || (InNodeIndex >= ((INT32)(Nodes.Length()))))
			{
				return;
			}
			const ROctreeNode& TempNode = Nodes[InNodeIndex];
			if (InCond(TempNode, InCustomValue))
			{
				_TCustomType NewCustomValue = InFunc(TempNode, InCustomValue);
				if (TempNode.ChildrenIndex.Num() > 0)
				{
					for (INT32 ChildIndex = 0, ChildNum = TempNode.ChildrenIndex.Num(); ChildIndex < ChildNum; ChildIndex++)
					{
						BackwardRecursionNode<_TCustomType, _TLambdaType, _TConditionType>(TempNode.ChildrenIndex[ChildIndex], NewCustomValue, InFunc, InCond);
					}
				}
			}
		}
		template<typename _TLambdaType, typename _TConditionType>
		void BackwardRecursionNode(INT32 InNodeIndex, const _TLambdaType& InFunc, const _TConditionType& InCond)
		{
			if ((InNodeIndex < 0) || (InNodeIndex >= ((INT32)(Nodes.Length()))))
			{
				return;
			}
			ROctreeNode& TempNode = Nodes[InNodeIndex];
			if (InCond(TempNode))
			{
				InFunc(TempNode);
				if (TempNode.ChildrenIndex.Num() > 0)
				{
					for (INT32 ChildIndex = 0, ChildNum = TempNode.ChildrenIndex.Num(); ChildIndex < ChildNum; ChildIndex++)
					{
						BackwardRecursionNode<_TLambdaType, _TConditionType>(TempNode.ChildrenIndex[ChildIndex], InFunc, InCond);
					}
				}
			}
		}
		template<typename _TLambdaType, typename _TConditionType>
		void BackwardRecursionNode(INT32 InNodeIndex, const _TLambdaType& InFunc, const _TConditionType& InCond)const
		{
			if ((InNodeIndex < 0) || (InNodeIndex >= ((INT32)(Nodes.Length()))))
			{
				return;
			}
			const ROctreeNode& TempNode = Nodes[InNodeIndex];
			if (InCond(TempNode))
			{
				InFunc(TempNode);
				if (TempNode.ChildrenIndex.Num() > 0)
				{
					for (INT32 ChildIndex = 0, ChildNum = TempNode.ChildrenIndex.Num(); ChildIndex < ChildNum; ChildIndex++)
					{
						BackwardRecursionNode<_TLambdaType, _TConditionType>(TempNode.ChildrenIndex[ChildIndex], InFunc, InCond);
					}
				}
			}
		}
		template<typename _TCustomType, typename _TLambdaType, typename _TConditionType>
		void ForwardRecursionNode(INT32 InNodeIndex, _TCustomType InCustomValue, const _TLambdaType& InFunc, const _TConditionType& InCond)
		{
			if ((InNodeIndex < 0) || (InNodeIndex >= (Nodes.Num())))
			{
				return;
			}
			ROctreeNode& TempNode = Nodes[InNodeIndex];
			if (InCond(TempNode, InCustomValue))
			{
				_TCustomType NewCustomValue = InFunc(TempNode, InCustomValue);
				ForwardRecursionNode<_TCustomType, _TLambdaType, _TConditionType>(TempNode.ParentIndex, NewCustomValue, InFunc, InCond);
			}
		}
		template<typename _TCustomType, typename _TLambdaType, typename _TConditionType>
		void ForwardRecursionNode(INT32 InNodeIndex, _TCustomType InCustomValue, const _TLambdaType& InFunc, const _TConditionType& InCond)const
		{
			if ((InNodeIndex < 0) || (InNodeIndex >= (Nodes.Num())))
			{
				return;
			}
			const ROctreeNode& TempNode = Nodes[InNodeIndex];
			if (InCond(TempNode, InCustomValue))
			{
				_TCustomType NewCustomValue = InFunc(TempNode, InCustomValue);
				ForwardRecursionNode<_TCustomType, _TLambdaType, _TConditionType>(TempNode.ParentIndex, NewCustomValue, InFunc, InCond);
			}
		}
		template<typename _TLambdaType, typename _TConditionType>
		void ForwardRecursionNode(INT32 InNodeIndex, const _TLambdaType& InFunc, const _TConditionType& InCond)
		{
			if ((InNodeIndex < 0) || (InNodeIndex >= (Nodes.Num())))
			{
				return;
			}
			ROctreeNode& TempNode = Nodes[InNodeIndex];
			if (InCond(TempNode))
			{
				InFunc(TempNode);
				ForwardRecursionNode<_TLambdaType, _TConditionType>(TempNode.ParentIndex, InFunc, InCond);
			}
		}
		template<typename _TLambdaType, typename _TConditionType>
		void ForwardRecursionNode(INT32 InNodeIndex, const _TLambdaType& InFunc, const _TConditionType& InCond)const
		{
			if ((InNodeIndex < 0) || (InNodeIndex >= (Nodes.Num())))
			{
				return;
			}
			const ROctreeNode& TempNode = Nodes[InNodeIndex];
			if (InCond(TempNode))
			{
				InFunc(TempNode);
				ForwardRecursionNode<_TLambdaType, _TConditionType>(TempNode.ParentIndex, InFunc, InCond);
			}
		}
	public:
		const Vector3&					GetTargetCellSize()const;
		const Vector3&					GetBoundOrigin()const;
		const Vector3&					GetBoundSize()const;
		const Vector3&					GetUsedCellSize()const;
		UINT32							GetAxisCellNum(EOctreeAxisType InAxisType)const;
		UINT32							GetAxisDepth(EOctreeAxisType InAxisType)const;
		UINT32							GetMaxDepth()const;
		const TArray<ROctreeNode>&		GetNodes()const;
		const TArray<ROctreeElement>&	GetElements()const;
		const TArray<ROctreeLayerInfo>&	GetLayerInfos()const;
	public:
		void	SetTargetCellSize(const Vector3& InTargetCellSize);
	protected:

	protected:
		Vector3						TargetCellSize;
		Vector3						BoundOrigin;
		Vector3						BoundSize;
		Vector3						UsedCellSize;
		UINT32						PerAxisCellNum[EOctreeAxisType::OCTREE_AXIS_COUNT];
		UINT32						PerAxisDepth[EOctreeAxisType::OCTREE_AXIS_COUNT];
		UINT32						MaxDepth;
		TArray<ROctreeNode>			Nodes;
		TArray<ROctreeElement>		Elements;
		TArray<ROctreeLayerInfo>	LayerInfos;
	public:
		ROctree();
		ROctree(const ROctree& Other);
		virtual ~ROctree();
		ROctree& operator=(const ROctree& Other);
	};

};