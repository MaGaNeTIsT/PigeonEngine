#include "RenderOctree.h"
#include <Base/DataStructure/Transform.h>
#include "RenderProxy/PrimitiveSceneProxy.h"

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<ROctree, EObjectBase>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	static PE_INLINE INT32 GetOctreeArrayIndex(const INT32 InXAxisCellNum, const INT32 InYAxisCellNum, const INT32 InZAxisCellNum, const ROctreeLayerInfo& InLayerInfo, const INT32 InX, const INT32 InY, const INT32 InZ)
	{
		return (((InY * (InXAxisCellNum * InZAxisCellNum)) + (InX * InZAxisCellNum) + InZ) + InLayerInfo.StartIndex);
	}
	static INT32 FindOctreeIndexByWorldLocation(const Vector3& InTestLocation, const Vector3& InOctreeOrigin, const TArray<ROctreeLayerInfo>& InOctreeLayerInfos, const INT32 InOctreeXAxisCellNum, const INT32 InOctreeYAxisCellNum, const INT32 InOctreeZAxisCellNum)
	{
		INT32 Result = -1;
		if (InOctreeLayerInfos.Num() < 2)
		{
			return Result;
		}
		const ROctreeLayerInfo& RootLayerInfo = InOctreeLayerInfos[0];
		const ROctreeLayerInfo& CellLayerInfo = InOctreeLayerInfos[InOctreeLayerInfos.Num() - 1];
		Vector3 CellLayerExtent = CellLayerInfo.Extent * 2.f;
		Vector3 TestLocation = InTestLocation - (InOctreeOrigin - RootLayerInfo.Extent);
		if ((TestLocation.x < 0.f) || (TestLocation.y < 0.f) || (TestLocation.z < 0.f))
		{
			return Result;
		}
		INT32 TestX = EMath::FloorToInt32(TestLocation.x / CellLayerExtent.x);
		INT32 TestY = EMath::FloorToInt32(TestLocation.y / CellLayerExtent.y);
		INT32 TestZ = EMath::FloorToInt32(TestLocation.z / CellLayerExtent.z);
		if ((TestX >= InOctreeXAxisCellNum) || (TestY >= InOctreeYAxisCellNum) || (TestZ >= InOctreeZAxisCellNum))
		{
			return Result;
		}
		return GetOctreeArrayIndex(InOctreeXAxisCellNum, InOctreeYAxisCellNum, InOctreeZAxisCellNum, CellLayerInfo, TestX, TestY, TestZ);
	}

	ROctree::ROctree()
		: TargetCellSize(Vector3::Zero())
		, BoundOrigin(Vector3::Zero())
		, BoundSize(Vector3::Zero())
		, UsedCellSize(Vector3::Zero())
		, MaxDepth(0u)
	{
		for (UINT32 i = 0u; i < EOctreeAxisType::OCTREE_AXIS_COUNT; i++)
		{
			PerAxisCellNum[i]	= 0u;
			PerAxisDepth[i]		= 0u;
		}
	}
	ROctree::ROctree(const ROctree& Other)
		: TargetCellSize(Other.TargetCellSize)
		, BoundOrigin(Other.BoundOrigin)
		, BoundSize(Other.BoundSize)
		, UsedCellSize(Other.UsedCellSize)
		, MaxDepth(Other.MaxDepth)
		, Nodes(Other.Nodes)
		, Elements(Other.Elements)
		, LayerInfos(Other.LayerInfos)
	{
		for (UINT32 i = 0u; i < EOctreeAxisType::OCTREE_AXIS_COUNT; i++)
		{
			PerAxisCellNum[i]	= Other.PerAxisCellNum[i];
			PerAxisDepth[i]		= Other.PerAxisDepth[i];
		}
	}
	ROctree::~ROctree()
	{
	}
	ROctree& ROctree::operator=(const ROctree& Other)
	{
		Nodes.Empty();
		Elements.Empty();
		LayerInfos.Empty();
		TargetCellSize	= Other.TargetCellSize;
		BoundOrigin		= Other.BoundOrigin;
		BoundSize		= Other.BoundSize;
		UsedCellSize	= Other.UsedCellSize;
		MaxDepth		= Other.MaxDepth;
		Nodes			= Other.Nodes;
		Elements		= Other.Elements;
		LayerInfos		= Other.LayerInfos;
		for (UINT32 i = 0u; i < EOctreeAxisType::OCTREE_AXIS_COUNT; i++)
		{
			PerAxisCellNum[i]	= Other.PerAxisCellNum[i];
			PerAxisDepth[i]		= Other.PerAxisDepth[i];
		}
		return (*this);
	}
	BOOL32 ROctree::SplitOctreeInternal(const Vector3& InOrigin, const Vector3& InExtent, const Vector3& InMinExtent, Vector3& OutOctreeOrigin, Vector3& OutOctreeSize, Vector3& OutOctreeCellSize, TArray<UINT32>& OutOctreePerAxisCellNum, TArray<UINT32>& OutOctreeAxisDepth, UINT32& OutOctreeMaxDepth, TArray<ROctreeLayerInfo>& OutOctreeLayerInfos, TArray<ROctreeElement>& OutOctreeElements, TArray<ROctreeNode>& OutOctreeNodes)
	{
		if ((InMinExtent.x < 1.f) || (InMinExtent.y < 1.f) || (InMinExtent.z < 1.f))
		{
			return FALSE;
		}
		if ((InExtent.x < InMinExtent.x) || (InExtent.y < InMinExtent.y) || (InExtent.z < InMinExtent.z))
		{
			return FALSE;
		}
		auto GenerateDimension = [](const UINT32 InExp, const FLOAT InExtent, const FLOAT InMinExtent, INT32& DimensionExp, INT32& DimensionCellNum, FLOAT& UsedMinExtent)->void
		{
			FLOAT SplitCellNum = EMath::LogX(EMath::TruncToFloat(InExp), InExtent / InMinExtent);
			DimensionExp = EMath::CeilToInt32(SplitCellNum);
			DimensionExp = EMath::Max(1, DimensionExp);
			DimensionCellNum = (INT32)(EMath::Pow(EMath::TruncToFloat(InExp), EMath::TruncToFloat(DimensionExp)));
			UsedMinExtent = (InExtent / ((FLOAT)DimensionCellNum)) / 2.f;
		};
		const Vector3 TempAnchor = InOrigin - InExtent;
		const FLOAT TempExtent[3] = { InExtent.x * 2.f, InExtent.y * 2.f, InExtent.z * 2.f };
		const FLOAT TempMinExtent[3] = { InMinExtent.x * 2.f, InMinExtent.y * 2.f, InMinExtent.z * 2.f };

		INT32 DimensionXYZExp[3]; INT32 DimensionXYZ[3]; FLOAT UsedMinExtent[3];
		INT32 UsedDimensionExp = 1;
		for (INT32 DimensionIndex = 0; DimensionIndex < 3; DimensionIndex++)
		{
			GenerateDimension(2u, TempExtent[DimensionIndex], TempMinExtent[DimensionIndex], DimensionXYZExp[DimensionIndex], DimensionXYZ[DimensionIndex], UsedMinExtent[DimensionIndex]);
			UsedDimensionExp = EMath::Max(DimensionXYZExp[DimensionIndex], UsedDimensionExp);
		}
		{
			OutOctreeOrigin = InOrigin;
			OutOctreeSize = Vector3(TempExtent[0], TempExtent[1], TempExtent[2]);
			OutOctreeCellSize = Vector3(UsedMinExtent[0] * 2.f, UsedMinExtent[1] * 2.f, UsedMinExtent[2] * 2.f);
			OutOctreeMaxDepth = (UINT32)UsedDimensionExp;
			for (INT32 DimensionIndex = 0; DimensionIndex < 3; DimensionIndex++)
			{
				OutOctreePerAxisCellNum[DimensionIndex] = (UINT32)(DimensionXYZ[DimensionIndex]);
				OutOctreeAxisDepth[DimensionIndex] = (UINT32)(DimensionXYZExp[DimensionIndex] + 1);
			}
			if (OutOctreeLayerInfos.Num() > 0)
			{
				OutOctreeLayerInfos.Empty();
			}
			OutOctreeLayerInfos.Reserve((UsedDimensionExp + 1));
		}
		{
			INT32 LayerCellNum = 0;
			for (INT32 LayerIndex = UsedDimensionExp; LayerIndex >= 0; LayerIndex--)
			{
				INT32 CurrentLayerCellNum = 1;
				for (INT32 DimensionIndex = 0; DimensionIndex < 3; DimensionIndex++)
				{
					CurrentLayerCellNum *= EMath::Max(1, DimensionXYZ[DimensionIndex] / ((INT32)(EMath::Pow(2.0f, (FLOAT)LayerIndex))));
				}
				LayerCellNum += CurrentLayerCellNum;
			}
			if (OutOctreeElements.Num() > 0u)
			{
				OutOctreeElements.Empty();
			}
			OutOctreeElements.Reserve(LayerCellNum);
			if (OutOctreeNodes.Num() > 0u)
			{
				OutOctreeNodes.Empty();
			}
			OutOctreeNodes.Reserve(LayerCellNum);
			for (INT32 LayerIndex = UsedDimensionExp; LayerIndex >= 0; LayerIndex--)
			{
				INT32 CurrentLayerDimension[3]; FLOAT DimensionExtent[3];
				INT32 ParentLayerDimension[3] = { 1, 1, 1 }; FLOAT DimensionParentExtent[3] = { 0, 0, 0 };
				for (INT32 DimensionIndex = 0; DimensionIndex < 3; DimensionIndex++)
				{
					CurrentLayerDimension[DimensionIndex] = EMath::Max(1, DimensionXYZ[DimensionIndex] / ((INT32)(EMath::Pow(2.0f, (FLOAT)LayerIndex))));
					DimensionExtent[DimensionIndex] = TempExtent[DimensionIndex] / ((FLOAT)(CurrentLayerDimension[DimensionIndex]));
					if (LayerIndex != UsedDimensionExp)
					{
						ParentLayerDimension[DimensionIndex] = EMath::Max(1, DimensionXYZ[DimensionIndex] / ((INT32)(EMath::Pow(2.0f, (FLOAT)(LayerIndex + 1)))));
						DimensionParentExtent[DimensionIndex] = TempExtent[DimensionIndex] / ((FLOAT)(ParentLayerDimension[DimensionIndex]));
					}
				}
				Vector3 LayerExtentBase = Vector3(DimensionExtent[0], DimensionExtent[1], DimensionExtent[2]);
				Vector3 LayerOriginBase = TempAnchor + (LayerExtentBase / 2.f);
				Vector3 ParentLayerExtentBase = Vector3(DimensionParentExtent[0], DimensionParentExtent[1], DimensionParentExtent[2]);
				ROctreeLayerInfo& AddInfo = OutOctreeLayerInfos.AddDefaultGetRef();
				AddInfo.NodeNum = CurrentLayerDimension[0] * CurrentLayerDimension[1] * CurrentLayerDimension[2];
				AddInfo.Extent = LayerExtentBase / 2.f;
				AddInfo.StartIndex = 0;
				for (INT32 LayerInfoIndex = 0, LayerInfoNum = (OutOctreeLayerInfos.Num()) - 1; LayerInfoIndex < LayerInfoNum; LayerInfoIndex++)
				{
					AddInfo.StartIndex += OutOctreeLayerInfos[LayerInfoIndex].NodeNum;
				}
				for (INT32 DimensionYIndex = 0; DimensionYIndex < CurrentLayerDimension[1]; DimensionYIndex++)
				{
					for (INT32 DimensionXIndex = 0; DimensionXIndex < CurrentLayerDimension[0]; DimensionXIndex++)
					{
						for (INT32 DimensionZIndex = 0; DimensionZIndex < CurrentLayerDimension[2]; DimensionZIndex++)
						{
							INT32 ElementIndex = OutOctreeElements.Num();
							ROctreeElement& AddElement = OutOctreeElements.AddDefaultGetRef();
							INT32 NodeIndex = OutOctreeNodes.Num();
							ROctreeNode& AddNode = OutOctreeNodes.AddDefaultGetRef();
							{
								AddElement.NodeIndex = NodeIndex;
							}
							{
								AddNode.Origin = LayerExtentBase * Vector3(DimensionXIndex, DimensionYIndex, DimensionZIndex) + LayerOriginBase;
								AddNode.ElementIndex = ElementIndex;
								if (LayerIndex == UsedDimensionExp)
								{
									AddNode.ParentIndex = -1;
								}
								else
								{
									INT32 TempParentIndex = 0;
									{
										INT32 TempParentIndexX = (INT32)(EMath::Max(0.f, AddNode.Origin.x - TempAnchor.x) / ParentLayerExtentBase.x);
										INT32 TempParentIndexY = (INT32)(EMath::Max(0.f, AddNode.Origin.y - TempAnchor.y) / ParentLayerExtentBase.y);
										INT32 TempParentIndexZ = (INT32)(EMath::Max(0.f, AddNode.Origin.z - TempAnchor.z) / ParentLayerExtentBase.z);
										INT32 TempParentParentCellNum = 0;
										for (INT32 ParentParentLayerIndex = UsedDimensionExp, ParentParentLayerNum = LayerIndex + 1; ParentParentLayerIndex > ParentParentLayerNum; ParentParentLayerIndex--)
										{
											INT32 TempLayerCellNum = 1;
											for (INT32 DimensionIndex = 0; DimensionIndex < 3; DimensionIndex++)
											{
												TempLayerCellNum *= EMath::Max(1, DimensionXYZ[DimensionIndex] / ((INT32)(EMath::Pow(2.0f, (FLOAT)ParentParentLayerIndex))));
											}
											TempParentParentCellNum += TempLayerCellNum;
										}
										//TODO GetOctreeArrayIndex(OutOctreePerAxisCellNum, const FBezierGrassOctreeLayerInfo& InLayerInfo, INT32 InX, INT32 InY, INT32 InZ);
										TempParentIndex = TempParentParentCellNum + (ParentLayerDimension[0] * ParentLayerDimension[2]) * TempParentIndexY + ParentLayerDimension[2] * TempParentIndexX + TempParentIndexZ;
									}
									PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Render octree's parent node index is over node array."), (TempParentIndex < (OutOctreeNodes.Num())));
									AddNode.ParentIndex = TempParentIndex;
									OutOctreeNodes[TempParentIndex].ChildrenIndex.Add(NodeIndex);
								}
							}
						}
					}
				}
			}
		}
		return TRUE;
	}
	BOOL32 ROctree::AddPrimitive(const RPrimitiveSceneProxy* InSceneProxy)
	{
		BOOL32 Result = FALSE;
		if (Nodes.Num() < 2u)
		{
			return Result;
		}
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check render octree nodes and elements num failed."), (Nodes.Num() == Elements.Num()));
		if ((!InSceneProxy) || (!(InSceneProxy->IsRenderValid())))
		{
			return Result;
		}
		const INT32 OctreeElementNum = Elements.Num();
		const Vector3& TempLocation = InSceneProxy->GetWorldLocation();
		INT32 TempElementIndex = FindOctreeIndexByWorldLocation(TempLocation, BoundOrigin, LayerInfos, PerAxisCellNum[EOctreeAxisType::OCTREE_AXIS_X], PerAxisCellNum[EOctreeAxisType::OCTREE_AXIS_Y], PerAxisCellNum[EOctreeAxisType::OCTREE_AXIS_Z]);
		if ((TempElementIndex < 0) || (TempElementIndex >= OctreeElementNum))
		{
			return Result;
		}
		ROctreeElement& Element = Elements[TempElementIndex];
		if (Element.PrimitiveMapping.ContainsKey(InSceneProxy->GetUniqueID()))
		{
			return Result;
		}
		Element.PrimitiveMapping.Add(InSceneProxy->GetUniqueID(), Element.Primitives.Num());
		Element.Primitives.Add(InSceneProxy);
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check render octree element primitive container failed."), (Element.Primitives.Num() == Element.PrimitiveMapping.Num()));
		Result = TRUE;
		return Result;
	}
	BOOL32 ROctree::AddPrimitives(const TArray<RPrimitiveSceneProxy*>& InPrimitives, TArray<INT32>& OutErrorPrimitives)
	{
		BOOL32 Result = FALSE;
		if ((Nodes.Num() < 2) || (InPrimitives.Num() <= 0))
		{
			return Result;
		}
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check render octree nodes and elements num failed."), (Nodes.Num() == Elements.Num()));
		if (OutErrorPrimitives.Num() > 0)
		{
			OutErrorPrimitives.Empty();
		}
		for (INT32 PrimitiveIndex = 0, PrimitiveNum = InPrimitives.Num(); PrimitiveIndex < PrimitiveNum; PrimitiveIndex++)
		{
			const RPrimitiveSceneProxy* TempPrimitive = InPrimitives[PrimitiveIndex];
			if (!(AddPrimitive(TempPrimitive)))
			{
				OutErrorPrimitives.Add(PrimitiveIndex);
				continue;
			}
			Result = TRUE;
		}
		return Result;
	}
	BOOL32 ROctree::RemovePrimitive(const RPrimitiveSceneProxy* InSceneProxy)
	{
		BOOL32 Result = FALSE;
		if (Nodes.Num() < 2)
		{
			return Result;
		}
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check render octree nodes and elements num failed."), (Nodes.Num() == Elements.Num()));
		if (!InSceneProxy)
		{
			return Result;
		}
		const INT32 OctreeElementNum = Elements.Num();
		const Vector3& TempLocation = InSceneProxy->GetWorldLocation();
		INT32 TempElementIndex = FindOctreeIndexByWorldLocation(TempLocation, BoundOrigin, LayerInfos, PerAxisCellNum[EOctreeAxisType::OCTREE_AXIS_X], PerAxisCellNum[EOctreeAxisType::OCTREE_AXIS_Y], PerAxisCellNum[EOctreeAxisType::OCTREE_AXIS_Z]);
		if ((TempElementIndex < 0) || (TempElementIndex >= OctreeElementNum))
		{
			return Result;
		}
		ROctreeElement& Element = Elements[TempElementIndex];
		const INT32 PrimitiveNum = Element.Primitives.Num();
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check render octree element primitive container failed."), (Element.Primitives.Num() == Element.PrimitiveMapping.Num()));
		if (PrimitiveNum == 0u)
		{
			return Result;
		}
		if (INT32 PrimitiveMappingValue = -1; Element.PrimitiveMapping.FindValue(InSceneProxy->GetUniqueID(), PrimitiveMappingValue))
		{
			PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check primitive index in render octree element failed."), ((PrimitiveMappingValue >= 0) && (PrimitiveMappingValue < PrimitiveNum)));
			const INT32 PrimitiveIndex = PrimitiveMappingValue;
			const ObjectIdentityType& PrimitiveID = InSceneProxy->GetUniqueID();
			Element.PrimitiveMapping.Remove(PrimitiveID);
			if (PrimitiveIndex != (PrimitiveNum - 1))
			{
				const RPrimitiveSceneProxy* TempPrimitiveProxy = Element.Primitives[PrimitiveNum - 1u];
				Element.Primitives[PrimitiveNum - 1] = Element.Primitives[PrimitiveIndex];
				Element.Primitives[PrimitiveIndex] = TempPrimitiveProxy;
				Element.PrimitiveMapping[TempPrimitiveProxy->GetUniqueID()] = PrimitiveIndex;
			}
			Element.Primitives.Pop();
			Result = TRUE;
		}
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check render octree element primitive container failed."), (Element.Primitives.Num() == Element.PrimitiveMapping.Num()));
		return Result;
	}
	BOOL32 ROctree::RemovePrimitives(const TArray<RPrimitiveSceneProxy*>& InPrimitives, TArray<INT32>& OutErrorPrimitives)
	{
		BOOL32 Result = FALSE;
		if ((Nodes.Num() < 2) || (InPrimitives.Num() <= 0))
		{
			return Result;
		}
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check render octree nodes and elements num failed."), (Nodes.Num() == Elements.Num()));
		if (OutErrorPrimitives.Num() > 0)
		{
			OutErrorPrimitives.Empty();
		}
		for (INT32 PrimitiveIndex = 0, PrimitiveNum = InPrimitives.Num(); PrimitiveIndex < PrimitiveNum; PrimitiveIndex++)
		{
			const RPrimitiveSceneProxy* TempPrimitive = InPrimitives[PrimitiveIndex];
			if (!(RemovePrimitive(TempPrimitive)))
			{
				OutErrorPrimitives.Add(PrimitiveIndex);
				continue;
			}
			Result = TRUE;
		}
		return Result;
	}
	UINT32 ROctree::GetPrimitiveNum()const
	{
		UINT32 Result = 0u;
		for (INT32 i = 0, n = Elements.Num(); i < n; i++)
		{
			Result += Elements[i].Primitives.Num<UINT32>();
		}
		return Result;
	}
	void ROctree::ClearPrimitives()
	{
		for (INT32 i = 0, n = Elements.Num(); i < n; i++)
		{
			Elements[i].PrimitiveMapping.Empty();
			Elements[i].Primitives.Empty();
		}
	}
	BOOL32 ROctree::RebuildOctreeForWholeLevel(const EBoundAABB* InBounds)
	{
		if (TargetCellSize.x < 0.5f || TargetCellSize.y < 0.5f || TargetCellSize.z < 0.5f)
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Check render octree target cell size info error. Size values are invalid."));
			return FALSE;
		}
		const EBoundAABB* UsedBounds = InBounds;
		if ((!UsedBounds) || (((UsedBounds->AABBMax.x - UsedBounds->AABBMin.x) < 1.0f) || ((UsedBounds->AABBMax.y - UsedBounds->AABBMin.y) < 1.0f) || ((UsedBounds->AABBMax.z - UsedBounds->AABBMin.z) < 1.0f)))
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Check current level bounds info error. Can not find current level bounds."));
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Failed to rebuilding procedural bezier grass."));
			return FALSE;
		}
		Vector3 LevelBoundsOrigin, LevelBoundsExtent;
		{
			LevelBoundsOrigin = (UsedBounds->AABBMin + UsedBounds->AABBMax) / 2.f;
			LevelBoundsExtent = (UsedBounds->AABBMax - UsedBounds->AABBMin) / 2.f;
		}
		if (LayerInfos.Num() > 0)
		{
			LayerInfos.Empty();
		}
		if (Elements.Num() > 0)
		{
			Elements.Empty();
		}
		if (Nodes.Num() > 0)
		{
			Nodes.Empty();
		}
		TArray<UINT32> TempPerAxisCellNum, TempPerAxisDepth;
		if (SplitOctreeInternal(LevelBoundsOrigin, LevelBoundsExtent, (TargetCellSize / 2.f), BoundOrigin, BoundSize,
			UsedCellSize, TempPerAxisCellNum, TempPerAxisDepth, MaxDepth, LayerInfos, Elements, Nodes))
		{
			PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check render octree per axis cell num array failed."), (TempPerAxisCellNum.Num<UINT32>() == EOctreeAxisType::OCTREE_AXIS_COUNT));
			PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check render octree per axis depth array failed."), (TempPerAxisDepth.Num<UINT32>() == EOctreeAxisType::OCTREE_AXIS_COUNT));
			for (UINT32 i = 0u; i < EOctreeAxisType::OCTREE_AXIS_COUNT; i++)
			{
				PerAxisCellNum[i] = TempPerAxisCellNum[i];
				PerAxisDepth[i] = TempPerAxisDepth[i];
			}
			return TRUE;
		}
		PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Rebuilding procedural bezier grass octree for whole level failed."));
		return FALSE;
	}
	BOOL32 ROctree::FinalizeOctree()
	{
		if ((LayerInfos.Num() < 2) || (Nodes.Num() < 2))
		{
			return FALSE;
		}
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check render octree nodes and elements num failed."), (Nodes.Num() == Elements.Num()));
		BOOL32						Result			= FALSE;
		TArray<BOOL32>				VisibleNodeMaps;
		TArray<INT32>				OldNewMaps;
		TArray<ROctreeNode>			OldNodes;
		TArray<ROctreeElement>		OldElements;
		TArray<ROctreeLayerInfo>	OldLayerInfos;
		const INT32 LayerCount = LayerInfos.Num();
		{
			const INT32 NodeNum = Nodes.Num();
			VisibleNodeMaps.Reserve(NodeNum);
			OldNewMaps.Reserve(NodeNum);
			OldNodes.Reserve(NodeNum);
			OldElements.Reserve(NodeNum);
			OldLayerInfos.Reserve(LayerCount);
			for (INT32 NodeElementIndex = 0; NodeElementIndex < NodeNum; NodeElementIndex++)
			{
				VisibleNodeMaps.Add(FALSE);
				OldNewMaps.Add(-1);
				OldNodes.Add(Nodes[NodeElementIndex]);
				OldElements.Add(Elements[NodeElementIndex]);
			}
			for (INT32 LayerIndex = 0, LayerNum = LayerInfos.Num(); LayerIndex < LayerNum; LayerIndex++)
			{
				OldLayerInfos.Add(LayerInfos[LayerIndex]);
			}
		}
		for (INT32 LayerIndex = (LayerCount - 1); LayerIndex >= 0; LayerIndex--)
		{
			ROctreeLayerInfo& Layer = OldLayerInfos[LayerIndex];
			const INT32 TempStartIndex = Layer.StartIndex;
			const INT32 TempEndIndex = Layer.StartIndex + (Layer.NodeNum - 1);
			for (INT32 NodeElementIndex = TempEndIndex; NodeElementIndex >= TempStartIndex; NodeElementIndex--)
			{
				ROctreeElement& TempElement = OldElements[NodeElementIndex];
				ROctreeNode& TempNode = OldNodes[NodeElementIndex];
				BOOL32& TempVisible = VisibleNodeMaps[NodeElementIndex];
				if (LayerIndex == (LayerCount - 1))
				{
					BOOL32 ElementVisible = FALSE;
					for (INT32 PrimitiveIndex = 0, PrimitiveNum = TempElement.Primitives.Num(); PrimitiveIndex < PrimitiveNum; PrimitiveIndex++)
					{
						if (!!(TempElement.Primitives[PrimitiveIndex]) && (TempElement.Primitives[PrimitiveIndex]->IsRenderValid()))
						{
							ElementVisible = TRUE;
							break;
						}
					}
					TempVisible = ElementVisible;
					Result = Result || TempVisible;
				}
				else
				{
					for (INT32 ChildIndex = 0, ChildNum = TempNode.ChildrenIndex.Num(); ChildIndex < ChildNum; ChildIndex++)
					{
						TempVisible = VisibleNodeMaps[TempNode.ChildrenIndex[ChildIndex]] || TempVisible;
					}
				}
			}
		}
		if (Nodes.Num() > 0)
		{
			Nodes.Empty();
		}
		if (Elements.Num() > 0)
		{
			Elements.Empty();
		}
		if (LayerInfos.Num() > 0)
		{
			LayerInfos.Empty();
		}
		INT32 NodeElementCount = 0;
		for (INT32 LayerIndex = 0; LayerIndex < LayerCount; LayerIndex++)
		{
			ROctreeLayerInfo& OldLayer = OldLayerInfos[LayerIndex];
			const INT32 OldStartIndex = OldLayer.StartIndex;
			const INT32 OldEndIndex = OldLayer.StartIndex + (OldLayer.NodeNum - 1);
			INT32 NodeElementNum = 0;
			for (INT32 OldNodeElementIndex = OldStartIndex; OldNodeElementIndex <= OldEndIndex; OldNodeElementIndex++)
			{
				ROctreeElement& OldElement = OldElements[OldNodeElementIndex];
				ROctreeNode& OldNode = OldNodes[OldNodeElementIndex];
				BOOL32& OldVisible = VisibleNodeMaps[OldNodeElementIndex];
				if (OldVisible)
				{
					const INT32 AddElementIndex = Elements.Num();
					const INT32 AddNodeIndex = Nodes.Num();
					PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check render octree add element and node failed."), (AddElementIndex == AddNodeIndex));
					ROctreeElement& AddElement = Elements.AddDefaultGetRef();
					ROctreeNode& AddNode = Nodes.AddDefaultGetRef();
					OldNewMaps[OldNodeElementIndex] = AddNodeIndex;
					AddElement.NodeIndex = AddNodeIndex;
					for (INT32 PrimitiveIndex = 0, PrimitiveNum = OldElement.Primitives.Num(); PrimitiveIndex < PrimitiveNum; PrimitiveIndex++)
					{
						AddElement.Primitives.Add(OldElement.Primitives[PrimitiveIndex]);
					}
					AddNode.ElementIndex = AddElementIndex;
					AddNode.Origin = OldNode.Origin;
					if (OldNode.ParentIndex >= 0)
					{
						AddNode.ParentIndex = OldNewMaps[OldNode.ParentIndex];
						if (AddNode.ParentIndex >= 0)
						{
							Nodes[AddNode.ParentIndex].ChildrenIndex.Add(AddNodeIndex);
						}
					}
					NodeElementNum += 1;
				}
			}
			ROctreeLayerInfo& AddLayerInfo = LayerInfos.AddDefaultGetRef();
			AddLayerInfo.NodeNum	= NodeElementNum;
			AddLayerInfo.Extent		= OldLayer.Extent;
			AddLayerInfo.StartIndex = NodeElementCount;
			NodeElementCount += NodeElementNum;
		}
		return Result;
	}
	const Vector3& ROctree::GetTargetCellSize()const
	{
		return TargetCellSize;
	}
	const Vector3& ROctree::GetBoundOrigin()const
	{
		return BoundOrigin;
	}
	const Vector3& ROctree::GetBoundSize()const
	{
		return BoundSize;
	}
	const Vector3& ROctree::GetUsedCellSize()const
	{
		return UsedCellSize;
	}
	UINT32 ROctree::GetAxisCellNum(EOctreeAxisType InAxisType)const
	{
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check octree axis type failed."), (InAxisType < EOctreeAxisType::OCTREE_AXIS_COUNT));
#if _EDITOR_ONLY
		if ((InAxisType >= EOctreeAxisType::OCTREE_AXIS_X) && (InAxisType < EOctreeAxisType::OCTREE_AXIS_COUNT))
#endif
		{
			return (PerAxisCellNum[InAxisType]);
		}
#if _EDITOR_ONLY
		return 0u;
#endif
	}
	UINT32 ROctree::GetAxisDepth(EOctreeAxisType InAxisType)const
	{
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check octree axis type failed."), (InAxisType < EOctreeAxisType::OCTREE_AXIS_COUNT));
#if _EDITOR_ONLY
		if ((InAxisType >= EOctreeAxisType::OCTREE_AXIS_X) && (InAxisType < EOctreeAxisType::OCTREE_AXIS_COUNT))
#endif
		{
			return (PerAxisDepth[InAxisType]);
		}
#if _EDITOR_ONLY
		return 0u;
#endif
	}
	UINT32 ROctree::GetMaxDepth()const
	{
		return MaxDepth;
	}
	const TArray<ROctreeNode>& ROctree::GetNodes()const
	{
		return Nodes;
	}
	const TArray<ROctreeElement>& ROctree::GetElements()const
	{
		return Elements;
	}
	const TArray<ROctreeLayerInfo>& ROctree::GetLayerInfos()const
	{
		return LayerInfos;
	}
	void ROctree::SetTargetCellSize(const Vector3& InTargetCellSize)
	{
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check render octree target cell size failed."), ((InTargetCellSize.x > 0.5f) && (InTargetCellSize.y > 0.5f) && (InTargetCellSize.z > 0.5f)));
		TargetCellSize = InTargetCellSize;
	}

};