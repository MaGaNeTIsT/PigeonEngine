#include "RenderOctree.h"

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<ROctree, EObjectBase>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	ROctree::ROctree()
		: OctreeTargetCellSize(Vector3::Zero())
		, OctreeOrigin(Vector3::Zero())
		, OctreeSize(Vector3::Zero())
		, OctreeUsedCellSize(Vector3::Zero())
		, OctreeMaxDepth(0u)
	{
		for (UINT i = 0u; i < EOctreeAxisType::OCTREE_AXIS_COUNT; i++)
		{
			OctreePerAxisCellNum[i] = 0u;
			OctreeAxisDepth[i] = 0u;
		}
	}
	ROctree::ROctree(const ROctree& Other)
		: OctreeTargetCellSize(Other.OctreeTargetCellSize)
		, OctreeOrigin(Other.OctreeOrigin)
		, OctreeSize(Other.OctreeSize)
		, OctreeUsedCellSize(Other.OctreeUsedCellSize)
		, OctreeMaxDepth(Other.OctreeMaxDepth)
		, Nodes(Other.Nodes)
		, Elements(Other.Elements)
		, LayerInfos(Other.LayerInfos)
	{
		for (UINT i = 0u; i < EOctreeAxisType::OCTREE_AXIS_COUNT; i++)
		{
			OctreePerAxisCellNum[i] = Other.OctreePerAxisCellNum[i];
			OctreeAxisDepth[i] = Other.OctreeAxisDepth[i];
		}
	}
	ROctree::~ROctree()
	{
	}
	ROctree& ROctree::operator=(const ROctree& Other)
	{
		Nodes.Clear();
		Elements.Clear();
		LayerInfos.Clear();
		OctreeTargetCellSize	= Other.OctreeTargetCellSize;
		OctreeOrigin			= Other.OctreeOrigin;
		OctreeSize				= Other.OctreeSize;
		OctreeUsedCellSize		= Other.OctreeUsedCellSize;
		OctreeMaxDepth			= Other.OctreeMaxDepth;
		Nodes					= Other.Nodes;
		Elements				= Other.Elements;
		LayerInfos				= Other.LayerInfos;
		for (UINT i = 0u; i < EOctreeAxisType::OCTREE_AXIS_COUNT; i++)
		{
			OctreePerAxisCellNum[i]	= Other.OctreePerAxisCellNum[i];
			OctreeAxisDepth[i]		= Other.OctreeAxisDepth[i];
		}
		return (*this);
	}
	const Vector3& ROctree::GetOctreeTargetCellSize()const
	{
		return OctreeTargetCellSize;
	}
	const Vector3& ROctree::GetOctreeOrigin()const
	{
		return OctreeOrigin;
	}
	const Vector3& ROctree::GetOctreeSize()const
	{
		return OctreeSize;
	}
	const Vector3& ROctree::GetOctreeUsedCellSize()const
	{
		return OctreeUsedCellSize;
	}
	UINT ROctree::GetOctreeAxisCellNum(EOctreeAxisType InAxisType)const
	{
		Check((ENGINE_RENDER_CORE_ERROR), ("Check octree axis type failed."), (InAxisType < EOctreeAxisType::OCTREE_AXIS_COUNT));
		return (OctreePerAxisCellNum[(InAxisType)]);
	}
	UINT ROctree::GetOctreeAxisDepth(EOctreeAxisType InAxisType)const
	{
		Check((ENGINE_RENDER_CORE_ERROR), ("Check octree axis type failed."), (InAxisType < EOctreeAxisType::OCTREE_AXIS_COUNT));
		return (OctreeAxisDepth[(InAxisType)]);
	}
	UINT ROctree::GetOctreeMaxDepth()const
	{
		return OctreeMaxDepth;
	}
	const TArray<ROctreeNode>& ROctree::GetOctreeNodes()const
	{
		return Nodes;
	}
	const TArray<ROctreeElement>& ROctree::GetOctreeElements()const
	{
		return Elements;
	}
	const TArray<ROctreeLayerInfo>& ROctree::GetOctreeLayerInfos()const
	{
		return LayerInfos;
	}

};