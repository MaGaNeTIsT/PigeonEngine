#pragma once

#include <CoreMinimal.h>

namespace PigeonEngine
{

	class RPrimitiveProxy;

	struct ROctreeElement
	{
		ROctreeElement()noexcept : NodeIndex(-1) {}
		ROctreeElement(const ROctreeElement& Other)noexcept : NodeIndex(Other.NodeIndex), Primitives(Other.Primitives) {}
		ROctreeElement& operator=(const ROctreeElement& Other)
		{
			NodeIndex	= Other.NodeIndex;
			Primitives	= Other.Primitives;
			return (*this);
		}

		INT32						NodeIndex;
		TArray<RPrimitiveProxy*>	Primitives;
	};

	struct ROctreeNode
	{
		ROctreeNode()noexcept : Origin(Vector3::Zero()), ElementIndex(-1), ParentIndex(-1) {}
		ROctreeNode(const ROctreeNode& Other)noexcept : Origin(Other.Origin), ElementIndex(Other.ElementIndex), ParentIndex(Other.ParentIndex), ChildrenIndex(Other.ChildrenIndex) {}
		ROctreeNode& operator=(const ROctreeNode& Other)
		{
			ChildrenIndex.Clear();
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
		//static BOOL32		SplitOctreeInternal(const Vector3& InOrigin, const Vector3& InExtent, const Vector3& InMinExtent, Vector3& OutOctreeOrigin, Vector3& OutOctreeSize, Vector3& OutOctreeCellSize, TArray<UINT32>& OutOctreePerAxisCellNum, TArray<UINT32>& OutOctreeAxisDepth, UINT32& OutOctreeMaxDepth, TArray<ROctreeLayerInfo>& OutOctreeLayerInfos, TArray<ROctreeElement>& OutOctreeElements, TArray<ROctreeNode>& OutOctreeNodes);
	public:
		const Vector3&					GetOctreeTargetCellSize()const;
		const Vector3&					GetOctreeOrigin()const;
		const Vector3&					GetOctreeSize()const;
		const Vector3&					GetOctreeUsedCellSize()const;
		UINT32							GetOctreeAxisCellNum(EOctreeAxisType InAxisType)const;
		UINT32							GetOctreeAxisDepth(EOctreeAxisType InAxisType)const;
		UINT32							GetOctreeMaxDepth()const;
		const TArray<ROctreeNode>&		GetOctreeNodes()const;
		const TArray<ROctreeElement>&	GetOctreeElements()const;
		const TArray<ROctreeLayerInfo>&	GetOctreeLayerInfos()const;
	public:
		void SetTargetCellSize(const Vector3& InTargetCellSize);
	protected:
		Vector3						OctreeTargetCellSize;
		Vector3						OctreeOrigin;
		Vector3						OctreeSize;
		Vector3						OctreeUsedCellSize;
		UINT32						OctreePerAxisCellNum[EOctreeAxisType::OCTREE_AXIS_COUNT];
		UINT32						OctreeAxisDepth[EOctreeAxisType::OCTREE_AXIS_COUNT];
		UINT32						OctreeMaxDepth;
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