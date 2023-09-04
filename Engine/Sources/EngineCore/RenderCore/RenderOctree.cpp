#include "RenderOctree.h"
#include <Base/DataStructure/Transform.h>
#include "RenderProxy/PrimitiveProxy.h"

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
		if (InOctreeLayerInfos.Length() < 2u)
		{
			return Result;
		}
		const ROctreeLayerInfo& RootLayerInfo = InOctreeLayerInfos[0];
		const ROctreeLayerInfo& CellLayerInfo = InOctreeLayerInfos[InOctreeLayerInfos.Length() - 1u];
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
		Nodes.Clear();
		Elements.Clear();
		LayerInfos.Clear();
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
			FLOAT SplitCellNum = EMath::LogX(InExp, InExtent / InMinExtent);
			DimensionExp = EMath::CeilToInt32(SplitCellNum);
			DimensionExp = EMath::Max(1, DimensionExp);
			DimensionCellNum = (INT32)(EMath::Pow((FLOAT)InExp, DimensionExp));
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
			if (OutOctreeLayerInfos.Length() > 0u)
			{
				OutOctreeLayerInfos.Clear();
			}
			OutOctreeLayerInfos.Recapacity((UsedDimensionExp + 1));
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
			if (OutOctreeElements.Length() > 0u)
			{
				OutOctreeElements.Clear();
			}
			OutOctreeElements.Recapacity(LayerCellNum);
			if (OutOctreeNodes.Length() > 0u)
			{
				OutOctreeNodes.Clear();
			}
			OutOctreeNodes.Recapacity(LayerCellNum);
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
				ROctreeLayerInfo& AddInfo = OutOctreeLayerInfos.Add_Default_GetRef();
				AddInfo.NodeNum = CurrentLayerDimension[0] * CurrentLayerDimension[1] * CurrentLayerDimension[2];
				AddInfo.Extent = LayerExtentBase / 2.f;
				AddInfo.StartIndex = 0;
				for (INT32 LayerInfoIndex = 0, LayerInfoNum = ((INT32)(OutOctreeLayerInfos.Length())) - 1; LayerInfoIndex < LayerInfoNum; LayerInfoIndex++)
				{
					AddInfo.StartIndex += OutOctreeLayerInfos[LayerInfoIndex].NodeNum;
				}
				for (INT32 DimensionYIndex = 0; DimensionYIndex < CurrentLayerDimension[1]; DimensionYIndex++)
				{
					for (INT32 DimensionXIndex = 0; DimensionXIndex < CurrentLayerDimension[0]; DimensionXIndex++)
					{
						for (INT32 DimensionZIndex = 0; DimensionZIndex < CurrentLayerDimension[2]; DimensionZIndex++)
						{
							INT32 ElementIndex = (INT32)(OutOctreeElements.Length());
							ROctreeElement& AddElement = OutOctreeElements.Add_Default_GetRef();
							INT32 NodeIndex = (INT32)(OutOctreeNodes.Length());
							ROctreeNode& AddNode = OutOctreeNodes.Add_Default_GetRef();
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
									Check((ENGINE_RENDER_CORE_ERROR), ("Render octree's parent node index is over node array."), (TempParentIndex < ((INT32)(OutOctreeNodes.Length()))));
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
	/*
BOOL32 UProceduralBezierGrassComponent::RebuildOctreeForWholeLevel(const ALevelBounds* InBounds)
{
	UE_LOG(LogTemp, Display, TEXT("Try to rebuilding procedural bezier grass octree for whole level."));
	if (OctreeTargetCellSize.X < 1.0f || OctreeTargetCellSize.Y < 1.0f || OctreeTargetCellSize.Z < 1.0f)
	{
		UE_LOG(LogTemp, Display, TEXT("Check target cell size info error. Size values are invalid."));
		return FALSE;
	}
	const ALevelBounds* UsedBounds = InBounds;
	if ((!UsedBounds) || (!UsedBounds->BoxComponent) || UsedBounds->BoxComponent->IsZeroExtent())
	{
		UE_LOG(LogTemp, Display, TEXT("Check current level bounds info error. Can not find current level bounds."));
		UE_LOG(LogTemp, Display, TEXT("Failed to rebuilding procedural bezier grass."));
		return FALSE;
	}

	Vector3 LevelBoundsOrigin, LevelBoundsExtent;
	{
		Vector3 TempVector[3] =
		{
			UsedBounds->BoxComponent->GetForwardVector(),
			UsedBounds->BoxComponent->GetRightVector(),
			UsedBounds->BoxComponent->GetUpVector()
		};
		Vector3 TempOrigin = UsedBounds->BoxComponent->GetComponentLocation();
		Vector3 TempExtent = UsedBounds->BoxComponent->GetScaledBoxExtent();
		Vector3 TempLocation[8] =
		{
			Vector3(-1, -1, -1) * TempExtent,
			Vector3(-1,  1, -1) * TempExtent,
			Vector3( 1, -1, -1) * TempExtent,
			Vector3( 1,  1, -1) * TempExtent,
			Vector3(-1, -1,  1) * TempExtent,
			Vector3(-1,  1,  1) * TempExtent,
			Vector3( 1, -1,  1) * TempExtent,
			Vector3( 1,  1,  1) * TempExtent
		};
		Vector3 TempMin(UE_MAX_FLT, UE_MAX_FLT, UE_MAX_FLT), TempMax(-UE_MAX_FLT, -UE_MAX_FLT, -UE_MAX_FLT);
		for (INT32 Index = 0; Index < 8; Index++)
		{
			TempLocation[Index] = TempVector[0] * TempLocation[Index].X + TempVector[1] * TempLocation[Index].Y + TempVector[2] * TempLocation[Index].Z + TempOrigin;
			TempMin.X = EMath::Min(TempLocation[Index].X, TempMin.X);
			TempMin.Y = EMath::Min(TempLocation[Index].Y, TempMin.Y);
			TempMin.Z = EMath::Min(TempLocation[Index].Z, TempMin.Z);
			TempMax.X = EMath::Max(TempLocation[Index].X, TempMax.X);
			TempMax.Y = EMath::Max(TempLocation[Index].Y, TempMax.Y);
			TempMax.Z = EMath::Max(TempLocation[Index].Z, TempMax.Z);
		}
		LevelBoundsOrigin = (TempMin + TempMax) / 2.f;
		LevelBoundsExtent = (TempMax - TempMin) / 2.f;
	}

	if (OctreeLayerInfos.Length() > 0)
	{
		OctreeLayerInfos.Clear();
	}
	if (OctreeElements.Length() > 0)
	{
		OctreeElements.Clear();
	}
	if (OctreeNodes.Length() > 0)
	{
		OctreeNodes.Clear();
	}

	UE_LOG(LogTemp, Display, TEXT("Check ready. Prepare to spliting octree."));

	if (SplitOctreeInternal(LevelBoundsOrigin, LevelBoundsExtent, (OctreeTargetCellSize / 2.f), OctreeOrigin, OctreeSize, OctreeUsedCellSize, OctreePerAxisCellNum, OctreeAxisDepth, OctreeMaxDepth, OctreeLayerInfos, OctreeElements, OctreeNodes))
	{
		UE_LOG(LogTemp, Display, TEXT("Rebuilding procedural bezier grass octree for whole level success."));
		return TRUE;
	}

	UE_LOG(LogTemp, Display, TEXT("Rebuilding procedural bezier grass octree for whole level failed."));
	return FALSE;
}
*/
	BOOL32 ROctree::AddPrimitiveIntoOctreeElement(const RPrimitiveProxy* InPrimitiveProxy)
	{
		BOOL32 Result = FALSE;
		if ((!InPrimitiveProxy) || (!(InPrimitiveProxy->IsRenderValid())))
		{
			return Result;
		}
		const INT32 OctreeElementNum = (INT32)(Elements.Length());
		const Vector3& TempLocation = InPrimitiveProxy->GetWorldLocation();
		INT32 TempElementIndex = FindOctreeIndexByWorldLocation(TempLocation, BoundOrigin, LayerInfos, PerAxisCellNum[EOctreeAxisType::OCTREE_AXIS_X], PerAxisCellNum[EOctreeAxisType::OCTREE_AXIS_Y], PerAxisCellNum[EOctreeAxisType::OCTREE_AXIS_Z]);
		if ((TempElementIndex < 0) || (TempElementIndex >= OctreeElementNum))
		{
			return Result;
		}
		ROctreeElement& Element = Elements[TempElementIndex];
		if (Element.PrimitiveMapping.ContainsKey(InPrimitiveProxy->GetUniqueID()))
		{
			return Result;
		}
		Element.PrimitiveMapping.Add(InPrimitiveProxy->GetUniqueID(), (INT32)(Element.Primitives.Length()));
		Element.Primitives.Add(InPrimitiveProxy);
		Result = TRUE;
		return Result;
	}
	void ROctree::AddPrimitivesIntoOctreeElement(const TArray<RPrimitiveProxy*>& InPrimitives, TArray<INT32>& OutErrorPrimitives)
	{
		if ((Nodes.Length() < 2u) || (InPrimitives.Length() == 0u))
		{
			return;
		}
		Check((ENGINE_RENDER_CORE_ERROR), ("Check render octree nodes and elements num failed."), (Nodes.Length() == Elements.Length()));
		if (OutErrorPrimitives.Length() > 0u)
		{
			OutErrorPrimitives.Clear();
		}
		for (INT32 PrimitiveIndex = 0, PrimitiveNum = (INT32)(InPrimitives.Length()); PrimitiveIndex < PrimitiveNum; PrimitiveIndex++)
		{
			const RPrimitiveProxy* TempPrimitive = InPrimitives[PrimitiveIndex];
			if ((!TempPrimitive) || (!(TempPrimitive->IsRenderValid())))
			{
				continue;
			}
			const Vector3& TempLocation = TempPrimitive->GetWorldLocation();
			INT32 TempElementIndex = FindOctreeIndexByWorldLocation(TempLocation, BoundOrigin, LayerInfos, PerAxisCellNum[EOctreeAxisType::OCTREE_AXIS_X], PerAxisCellNum[EOctreeAxisType::OCTREE_AXIS_Y], PerAxisCellNum[EOctreeAxisType::OCTREE_AXIS_Z]);
			if (TempElementIndex < 0)
			{
				OutErrorPrimitives.Add(PrimitiveIndex);
				continue;
			}
			if (AddPrimitiveIntoOctreeElementInternal(TempElementIndex, TempPrimitive) < 0)
			{
				OutErrorPrimitives.Add(PrimitiveIndex);
				continue;
			}
		}
	}
	BOOL32 ROctree::FinalizeOctree()
	{
		if ((LayerInfos.Length() < 2u) || (Nodes.Length() < 2u))
		{
			return FALSE;
		}
		Check((ENGINE_RENDER_CORE_ERROR), ("Check render octree nodes and elements num failed."), (Nodes.Length() == Elements.Length()));
		BOOL32						Result			= FALSE;
		TArray<BOOL32>				VisibleNodeMaps;
		TArray<INT32>				OldNewMaps;
		TArray<ROctreeNode>			OldNodes;
		TArray<ROctreeElement>		OldElements;
		TArray<ROctreeLayerInfo>	OldLayerInfos;
		const INT32 LayerCount = (INT32)(LayerInfos.Length());
		{
			const UINT32 NodeNum = Nodes.Length();
			VisibleNodeMaps.Recapacity(NodeNum);
			OldNewMaps.Recapacity(NodeNum);
			OldNodes.Recapacity(NodeNum);
			OldElements.Recapacity(NodeNum);
			OldLayerInfos.Recapacity(LayerCount);
			for (UINT32 NodeElementIndex = 0; NodeElementIndex < NodeNum; NodeElementIndex++)
			{
				VisibleNodeMaps.Add(FALSE);
				OldNewMaps.Add(-1);
				OldNodes.Add(Nodes[NodeElementIndex]);
				OldElements.Add(Elements[NodeElementIndex]);
			}
			for (UINT32 LayerIndex = 0, LayerNum = LayerInfos.Length(); LayerIndex < LayerNum; LayerIndex++)
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
					for (INT32 PrimitiveIndex = 0, PrimitiveNum = TempElement.Primitives.Length(); PrimitiveIndex < PrimitiveNum; PrimitiveIndex++)
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
					for (INT32 ChildIndex = 0; ChildIndex < TempNode.ChildrenIndex.Length(); ChildIndex++)
					{
						TempVisible = VisibleNodeMaps[TempNode.ChildrenIndex[ChildIndex]] || TempVisible;
					}
				}
			}
		}
		if (Nodes.Length() > 0)
		{
			Nodes.Clear();
		}
		if (Elements.Length() > 0)
		{
			Elements.Clear();
		}
		if (LayerInfos.Length() > 0)
		{
			LayerInfos.Clear();
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
					const INT32 AddElementIndex = (INT32)(Elements.Length());
					const INT32 AddNodeIndex = (INT32)(Nodes.Length());
					Check((ENGINE_RENDER_CORE_ERROR), ("Check render octree add element and node failed."), (AddElementIndex == AddNodeIndex));
					ROctreeElement& AddElement = Elements.Add_Default_GetRef();
					ROctreeNode& AddNode = Nodes.Add_Default_GetRef();
					OldNewMaps[OldNodeElementIndex] = AddNodeIndex;
					AddElement.NodeIndex = AddNodeIndex;
					for (INT32 PrimitiveIndex = 0, PrimitiveNum = OldElement.Primitives.Length(); PrimitiveIndex < PrimitiveNum; PrimitiveIndex++)
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
			ROctreeLayerInfo& AddLayerInfo = LayerInfos.Add_Default_GetRef();
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
		Check((ENGINE_RENDER_CORE_ERROR), ("Check octree axis type failed."), (InAxisType < EOctreeAxisType::OCTREE_AXIS_COUNT));
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
		Check((ENGINE_RENDER_CORE_ERROR), ("Check octree axis type failed."), (InAxisType < EOctreeAxisType::OCTREE_AXIS_COUNT));
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
		Check((ENGINE_RENDER_CORE_ERROR), ("Check render octree target cell size failed."), ((InTargetCellSize.x > 0.5f) && (InTargetCellSize.y > 0.5f) && (InTargetCellSize.z > 0.5f)));
		TargetCellSize = InTargetCellSize;
	}

};