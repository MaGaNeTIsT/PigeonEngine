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
		for (UINT32 i = 0u; i < EOctreeAxisType::OCTREE_AXIS_COUNT; i++)
		{
			OctreePerAxisCellNum[i]	= 0u;
			OctreeAxisDepth[i]		= 0u;
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
		for (UINT32 i = 0u; i < EOctreeAxisType::OCTREE_AXIS_COUNT; i++)
		{
			OctreePerAxisCellNum[i]	= Other.OctreePerAxisCellNum[i];
			OctreeAxisDepth[i]		= Other.OctreeAxisDepth[i];
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
		for (UINT32 i = 0u; i < EOctreeAxisType::OCTREE_AXIS_COUNT; i++)
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
	UINT32 ROctree::GetOctreeAxisCellNum(EOctreeAxisType InAxisType)const
	{
		Check((ENGINE_RENDER_CORE_ERROR), ("Check octree axis type failed."), (InAxisType < EOctreeAxisType::OCTREE_AXIS_COUNT));
#if _EDITOR_ONLY
		if ((InAxisType >= EOctreeAxisType::OCTREE_AXIS_X) && (InAxisType < EOctreeAxisType::OCTREE_AXIS_COUNT))
#endif
		{
			return (OctreePerAxisCellNum[InAxisType]);
		}
#if _EDITOR_ONLY
		return 0u;
#endif
	}
	UINT32 ROctree::GetOctreeAxisDepth(EOctreeAxisType InAxisType)const
	{
		Check((ENGINE_RENDER_CORE_ERROR), ("Check octree axis type failed."), (InAxisType < EOctreeAxisType::OCTREE_AXIS_COUNT));
#if _EDITOR_ONLY
		if ((InAxisType >= EOctreeAxisType::OCTREE_AXIS_X) && (InAxisType < EOctreeAxisType::OCTREE_AXIS_COUNT))
#endif
		{
			return (OctreeAxisDepth[InAxisType]);
		}
#if _EDITOR_ONLY
		return 0u;
#endif
	}
	UINT32 ROctree::GetOctreeMaxDepth()const
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
	/*
bool UProceduralBezierGrassComponent::RebuildOctreeForWholeLevel(const ALevelBounds* InBounds)
{
	UE_LOG(LogTemp, Display, TEXT("Try to rebuilding procedural bezier grass octree for whole level."));
	if (OctreeTargetCellSize.X < 1.0f || OctreeTargetCellSize.Y < 1.0f || OctreeTargetCellSize.Z < 1.0f)
	{
		UE_LOG(LogTemp, Display, TEXT("Check target cell size info error. Size values are invalid."));
		return false;
	}
	const ALevelBounds* UsedBounds = InBounds;
	if ((!UsedBounds) || (!UsedBounds->BoxComponent) || UsedBounds->BoxComponent->IsZeroExtent())
	{
		UE_LOG(LogTemp, Display, TEXT("Check current level bounds info error. Can not find current level bounds."));
		UE_LOG(LogTemp, Display, TEXT("Failed to rebuilding procedural bezier grass."));
		return false;
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

	if (OctreeLayerInfos.Num() > 0)
	{
		OctreeLayerInfos.Empty();
	}
	if (OctreeElements.Num() > 0)
	{
		OctreeElements.Empty();
	}
	if (OctreeNodes.Num() > 0)
	{
		OctreeNodes.Empty();
	}

	UE_LOG(LogTemp, Display, TEXT("Check ready. Prepare to spliting octree."));

	if (SplitOctreeInternal(LevelBoundsOrigin, LevelBoundsExtent, (OctreeTargetCellSize / 2.f), OctreeOrigin, OctreeSize, OctreeUsedCellSize, OctreePerAxisCellNum, OctreeAxisDepth, OctreeMaxDepth, OctreeLayerInfos, OctreeElements, OctreeNodes))
	{
		UE_LOG(LogTemp, Display, TEXT("Rebuilding procedural bezier grass octree for whole level success."));
		return true;
	}

	UE_LOG(LogTemp, Display, TEXT("Rebuilding procedural bezier grass octree for whole level failed."));
	return false;
}
INT32 UProceduralBezierGrassComponent::AddInstanceIntoElement(UBezierGrassAssetData* InAssetData, INT32 InIndex, const FTransform& InTransform)
{
	if ((!InAssetData) || (InIndex < 0) || (InIndex >= OctreeElements.Num()))
	{
		return -1;
	}
	FBezierGrassOctreeElement& Element = OctreeElements[InIndex];
	INT32 AddIntoGroupIndex = -1;
	for (INT32 GroupIndex = 0; GroupIndex < Element.BezierGrassGroups.Num(); GroupIndex++)
	{
		FBezierGrassGroupData& TempGroup = Element.BezierGrassGroups[GroupIndex];
		if (TempGroup.AssetData == InAssetData)
		{
			AddIntoGroupIndex = GroupIndex;
			break;
		}
	}
	if (AddIntoGroupIndex < 0)
	{
		AddIntoGroupIndex = Element.BezierGrassGroups.Num();
		FBezierGrassGroupData& TempGroup = Element.BezierGrassGroups.AddDefaulted_GetRef();
		FBezierGrassOctreeNode& Node = OctreeNodes[InIndex];
		TempGroup.AssetData = InAssetData;
		TempGroup.GroupTransform = FTransform(Node.Origin);
	}
	FBezierGrassGroupData& Group = Element.BezierGrassGroups[AddIntoGroupIndex];
	{
		FTransform TempInstanceTransform = InTransform;
		Vector3 TempScale3D(TempInstanceTransform.GetScale3D());
		FBezierGrassInstanceData TempInstance(
			TempInstanceTransform.GetLocation() - Group.GroupTransform.GetLocation(),
			InAssetData->GrassSize * EMath::Max(TempScale3D.X, TempScale3D.Y),
			TempInstanceTransform.GetRotation().RotateVector(Vector3::XAxisVector));
		Group.InstanceData.Add(MoveTemp(TempInstance));
	}
	return AddIntoGroupIndex;
}
void UProceduralBezierGrassComponent::AddInstancesIntoOctreeElement(UBezierGrassAssetData* InAssetData, const TArray<FTransform>& InInstanceWorldTransforms, TArray<INT32>& OutErrorTransforms)
{
	if ((!InAssetData) || (InInstanceWorldTransforms.Num() < 1))
	{
		return;
	}
	if (OctreeNodes.Num() < 2)
	{
		return;
	}
	check(OctreeNodes.Num() == OctreeElements.Num());

	if (OutErrorTransforms.Num() > 0)
	{
		OutErrorTransforms.Empty();
	}

	for (INT32 InstanceIndex = 0; InstanceIndex < InInstanceWorldTransforms.Num(); InstanceIndex++)
	{
		const FTransform& TempTransform = InInstanceWorldTransforms[InstanceIndex];
		INT32 TempElementIndex = FindOctreeIndexByTransform(TempTransform, OctreeOrigin, OctreeLayerInfos, OctreePerAxisCellNum);
		if (TempElementIndex < 0)
		{
			OutErrorTransforms.Add(InstanceIndex);
			continue;
		}
		if (AddInstanceIntoElement(InAssetData, TempElementIndex, TempTransform) < 0)
		{
			OutErrorTransforms.Add(InstanceIndex);
			continue;
		}
	}
}
bool UProceduralBezierGrassComponent::FinalizeOctree()
{
	if ((OctreeLayerInfos.Num() < 2) || (OctreeNodes.Num() < 2))
	{
		return false;
	}
	check(OctreeNodes.Num() == OctreeElements.Num());
	bool Result = false;
	TArray<bool> VisibleNodeMaps;
	TArray<INT32> OldNewMaps;
	TArray<FBezierGrassOctreeNode> OldNodes;
	TArray<FBezierGrassOctreeElement> OldElements;
	TArray<FBezierGrassOctreeLayerInfo> OldLayerInfos;
	const INT32 LayerCount = OctreeLayerInfos.Num();
	{
		VisibleNodeMaps.Reserve(OctreeNodes.Num());
		OldNewMaps.Reserve(OctreeNodes.Num());
		OldNodes.Reserve(OctreeNodes.Num());
		OldElements.Reserve(OctreeElements.Num());
		OldLayerInfos.Reserve(OctreeLayerInfos.Num());
		for (INT32 NodeElementIndex = 0; NodeElementIndex < OctreeNodes.Num(); NodeElementIndex++)
		{
			VisibleNodeMaps.Add(false);
			OldNewMaps.Add(-1);
			OldNodes.Add(MoveTemp(OctreeNodes[NodeElementIndex]));
			OldElements.Add(MoveTemp(OctreeElements[NodeElementIndex]));
		}
		for (INT32 LayerIndex = 0; LayerIndex < OctreeLayerInfos.Num(); LayerIndex++)
		{
			OldLayerInfos.Add(MoveTemp(OctreeLayerInfos[LayerIndex]));
		}
	}
	for (INT32 LayerIndex = (LayerCount - 1); LayerIndex >= 0; LayerIndex--)
	{
		FBezierGrassOctreeLayerInfo& Layer = OldLayerInfos[LayerIndex];
		const INT32 TempStartIndex = Layer.StartIndex;
		const INT32 TempEndIndex = Layer.StartIndex + (Layer.NodeNum - 1);
		for (INT32 NodeElementIndex = TempEndIndex; NodeElementIndex >= TempStartIndex; NodeElementIndex--)
		{
			FBezierGrassOctreeElement& TempElement = OldElements[NodeElementIndex];
			FBezierGrassOctreeNode& TempNode = OldNodes[NodeElementIndex];
			bool& TempVisible = VisibleNodeMaps[NodeElementIndex];
			if (LayerIndex == (LayerCount - 1))
			{
				INT32 InstanceNum = 0;
				for (INT32 GroupIndex = 0; GroupIndex < TempElement.BezierGrassGroups.Num(); GroupIndex++)
				{
					if (CheckBezierGrassAssetDataValid(TempElement.BezierGrassGroups[GroupIndex].AssetData))
					{
						InstanceNum += TempElement.BezierGrassGroups[GroupIndex].InstanceData.Num();
					}
				}
				TempVisible = InstanceNum > 0;
				Result = Result || TempVisible;
			}
			else
			{
				for (INT32 ChildIndex = 0; ChildIndex < TempNode.ChildrenIndex.Num(); ChildIndex++)
				{
					TempVisible = VisibleNodeMaps[TempNode.ChildrenIndex[ChildIndex]] || TempVisible;
				}
			}
		}
	}
	if (OctreeNodes.Num() > 0)
	{
		OctreeNodes.Empty();
	}
	if (OctreeElements.Num() > 0)
	{
		OctreeElements.Empty();
	}
	if (OctreeLayerInfos.Num() > 0)
	{
		OctreeLayerInfos.Empty();
	}
	INT32 NodeElementCount = 0;
	for (INT32 LayerIndex = 0; LayerIndex < LayerCount; LayerIndex++)
	{
		FBezierGrassOctreeLayerInfo& OldLayer = OldLayerInfos[LayerIndex];
		const INT32 OldStartIndex = OldLayer.StartIndex;
		const INT32 OldEndIndex = OldLayer.StartIndex + (OldLayer.NodeNum - 1);
		INT32 NodeElementNum = 0;
		for (INT32 OldNodeElementIndex = OldStartIndex; OldNodeElementIndex <= OldEndIndex; OldNodeElementIndex++)
		{
			FBezierGrassOctreeElement& OldElement = OldElements[OldNodeElementIndex];
			FBezierGrassOctreeNode& OldNode = OldNodes[OldNodeElementIndex];
			bool& OldVisible = VisibleNodeMaps[OldNodeElementIndex];
			if (OldVisible)
			{
				const INT32 AddElementIndex = OctreeElements.Num();
				const INT32 AddNodeIndex = OctreeNodes.Num();
				check(AddElementIndex == AddNodeIndex);
				FBezierGrassOctreeElement& AddElement = OctreeElements.AddDefaulted_GetRef();
				FBezierGrassOctreeNode& AddNode = OctreeNodes.AddDefaulted_GetRef();
				OldNewMaps[OldNodeElementIndex] = AddNodeIndex;
				AddElement.OctreeNodeIndex = AddNodeIndex;
				for (INT32 GroupIndex = 0; GroupIndex < OldElement.BezierGrassGroups.Num(); GroupIndex++)
				{
					AddElement.BezierGrassGroups.Add(CopyTemp(OldElement.BezierGrassGroups[GroupIndex]));
				}
				AddNode.OctreeElementIndex = AddElementIndex;
				AddNode.Origin = OldNode.Origin;
				if (OldNode.ParentIndex >= 0)
				{
					AddNode.ParentIndex = OldNewMaps[OldNode.ParentIndex];
					if (AddNode.ParentIndex >= 0)
					{
						OctreeNodes[AddNode.ParentIndex].ChildrenIndex.Add(AddNodeIndex);
					}
				}
				NodeElementNum += 1;
			}
		}
		FBezierGrassOctreeLayerInfo& AddLayerInfo = OctreeLayerInfos.AddDefaulted_GetRef();
		AddLayerInfo.NodeNum = NodeElementNum;
		AddLayerInfo.Extent = OldLayer.Extent;
		AddLayerInfo.StartIndex = NodeElementCount;
		NodeElementCount += NodeElementNum;
	}
	return Result;
}
*/
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

};