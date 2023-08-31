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

		INT							NodeIndex;
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

		Vector3		Origin;
		INT			ElementIndex;
		INT			ParentIndex;
		TArray<INT>	ChildrenIndex;
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

		INT		NodeNum;
		Vector3	Extent;
		INT		StartIndex;
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
		const Vector3&					GetOctreeTargetCellSize()const;
		const Vector3&					GetOctreeOrigin()const;
		const Vector3&					GetOctreeSize()const;
		const Vector3&					GetOctreeUsedCellSize()const;
		UINT							GetOctreeAxisCellNum(EOctreeAxisType InAxisType)const;
		UINT							GetOctreeAxisDepth(EOctreeAxisType InAxisType)const;
		UINT							GetOctreeMaxDepth()const;
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
		UINT						OctreePerAxisCellNum[EOctreeAxisType::OCTREE_AXIS_COUNT];
		UINT						OctreeAxisDepth[EOctreeAxisType::OCTREE_AXIS_COUNT];
		UINT						OctreeMaxDepth;
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