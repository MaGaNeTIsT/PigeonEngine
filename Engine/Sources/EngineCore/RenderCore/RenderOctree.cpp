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

	FVector LevelBoundsOrigin, LevelBoundsExtent;
	{
		FVector TempVector[3] =
		{
			UsedBounds->BoxComponent->GetForwardVector(),
			UsedBounds->BoxComponent->GetRightVector(),
			UsedBounds->BoxComponent->GetUpVector()
		};
		FVector TempOrigin = UsedBounds->BoxComponent->GetComponentLocation();
		FVector TempExtent = UsedBounds->BoxComponent->GetScaledBoxExtent();
		FVector TempLocation[8] =
		{
			FVector(-1, -1, -1) * TempExtent,
			FVector(-1,  1, -1) * TempExtent,
			FVector( 1, -1, -1) * TempExtent,
			FVector( 1,  1, -1) * TempExtent,
			FVector(-1, -1,  1) * TempExtent,
			FVector(-1,  1,  1) * TempExtent,
			FVector( 1, -1,  1) * TempExtent,
			FVector( 1,  1,  1) * TempExtent
		};
		FVector TempMin(UE_MAX_FLT, UE_MAX_FLT, UE_MAX_FLT), TempMax(-UE_MAX_FLT, -UE_MAX_FLT, -UE_MAX_FLT);
		for (int32 Index = 0; Index < 8; Index++)
		{
			TempLocation[Index] = TempVector[0] * TempLocation[Index].X + TempVector[1] * TempLocation[Index].Y + TempVector[2] * TempLocation[Index].Z + TempOrigin;
			TempMin.X = FMath::Min(TempLocation[Index].X, TempMin.X);
			TempMin.Y = FMath::Min(TempLocation[Index].Y, TempMin.Y);
			TempMin.Z = FMath::Min(TempLocation[Index].Z, TempMin.Z);
			TempMax.X = FMath::Max(TempLocation[Index].X, TempMax.X);
			TempMax.Y = FMath::Max(TempLocation[Index].Y, TempMax.Y);
			TempMax.Z = FMath::Max(TempLocation[Index].Z, TempMax.Z);
		}
		LevelBoundsOrigin = (TempMin + TempMax) / ((double)2);
		LevelBoundsExtent = (TempMax - TempMin) / ((double)2);
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

	if (SplitOctreeInternal(LevelBoundsOrigin, LevelBoundsExtent, (OctreeTargetCellSize / ((double)2)), OctreeOrigin, OctreeSize, OctreeUsedCellSize, OctreePerAxisCellNum, OctreeAxisDepth, OctreeMaxDepth, OctreeLayerInfos, OctreeElements, OctreeNodes))
	{
		UE_LOG(LogTemp, Display, TEXT("Rebuilding procedural bezier grass octree for whole level success."));
		return true;
	}

	UE_LOG(LogTemp, Display, TEXT("Rebuilding procedural bezier grass octree for whole level failed."));
	return false;
}
int32 UProceduralBezierGrassComponent::AddInstanceIntoElement(UBezierGrassAssetData* InAssetData, int32 InIndex, const FTransform& InTransform)
{
	if ((!InAssetData) || (InIndex < 0) || (InIndex >= OctreeElements.Num()))
	{
		return -1;
	}
	FBezierGrassOctreeElement& Element = OctreeElements[InIndex];
	int32 AddIntoGroupIndex = -1;
	for (int32 GroupIndex = 0; GroupIndex < Element.BezierGrassGroups.Num(); GroupIndex++)
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
		FVector TempScale3D(TempInstanceTransform.GetScale3D());
		FBezierGrassInstanceData TempInstance(
			TempInstanceTransform.GetLocation() - Group.GroupTransform.GetLocation(),
			InAssetData->GrassSize * FMath::Max(TempScale3D.X, TempScale3D.Y),
			TempInstanceTransform.GetRotation().RotateVector(FVector::XAxisVector));
		Group.InstanceData.Add(MoveTemp(TempInstance));
	}
	return AddIntoGroupIndex;
}
void UProceduralBezierGrassComponent::AddInstancesIntoOctreeElement(UBezierGrassAssetData* InAssetData, const TArray<FTransform>& InInstanceWorldTransforms, TArray<int32>& OutErrorTransforms)
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

	for (int32 InstanceIndex = 0; InstanceIndex < InInstanceWorldTransforms.Num(); InstanceIndex++)
	{
		const FTransform& TempTransform = InInstanceWorldTransforms[InstanceIndex];
		int32 TempElementIndex = FindOctreeIndexByTransform(TempTransform, OctreeOrigin, OctreeLayerInfos, OctreePerAxisCellNum);
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
	TArray<int32> OldNewMaps;
	TArray<FBezierGrassOctreeNode> OldNodes;
	TArray<FBezierGrassOctreeElement> OldElements;
	TArray<FBezierGrassOctreeLayerInfo> OldLayerInfos;
	const int32 LayerCount = OctreeLayerInfos.Num();
	{
		VisibleNodeMaps.Reserve(OctreeNodes.Num());
		OldNewMaps.Reserve(OctreeNodes.Num());
		OldNodes.Reserve(OctreeNodes.Num());
		OldElements.Reserve(OctreeElements.Num());
		OldLayerInfos.Reserve(OctreeLayerInfos.Num());
		for (int32 NodeElementIndex = 0; NodeElementIndex < OctreeNodes.Num(); NodeElementIndex++)
		{
			VisibleNodeMaps.Add(false);
			OldNewMaps.Add(-1);
			OldNodes.Add(MoveTemp(OctreeNodes[NodeElementIndex]));
			OldElements.Add(MoveTemp(OctreeElements[NodeElementIndex]));
		}
		for (int32 LayerIndex = 0; LayerIndex < OctreeLayerInfos.Num(); LayerIndex++)
		{
			OldLayerInfos.Add(MoveTemp(OctreeLayerInfos[LayerIndex]));
		}
	}
	for (int32 LayerIndex = (LayerCount - 1); LayerIndex >= 0; LayerIndex--)
	{
		FBezierGrassOctreeLayerInfo& Layer = OldLayerInfos[LayerIndex];
		const int32 TempStartIndex = Layer.StartIndex;
		const int32 TempEndIndex = Layer.StartIndex + (Layer.NodeNum - 1);
		for (int32 NodeElementIndex = TempEndIndex; NodeElementIndex >= TempStartIndex; NodeElementIndex--)
		{
			FBezierGrassOctreeElement& TempElement = OldElements[NodeElementIndex];
			FBezierGrassOctreeNode& TempNode = OldNodes[NodeElementIndex];
			bool& TempVisible = VisibleNodeMaps[NodeElementIndex];
			if (LayerIndex == (LayerCount - 1))
			{
				int32 InstanceNum = 0;
				for (int32 GroupIndex = 0; GroupIndex < TempElement.BezierGrassGroups.Num(); GroupIndex++)
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
				for (int32 ChildIndex = 0; ChildIndex < TempNode.ChildrenIndex.Num(); ChildIndex++)
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
	int32 NodeElementCount = 0;
	for (int32 LayerIndex = 0; LayerIndex < LayerCount; LayerIndex++)
	{
		FBezierGrassOctreeLayerInfo& OldLayer = OldLayerInfos[LayerIndex];
		const int32 OldStartIndex = OldLayer.StartIndex;
		const int32 OldEndIndex = OldLayer.StartIndex + (OldLayer.NodeNum - 1);
		int32 NodeElementNum = 0;
		for (int32 OldNodeElementIndex = OldStartIndex; OldNodeElementIndex <= OldEndIndex; OldNodeElementIndex++)
		{
			FBezierGrassOctreeElement& OldElement = OldElements[OldNodeElementIndex];
			FBezierGrassOctreeNode& OldNode = OldNodes[OldNodeElementIndex];
			bool& OldVisible = VisibleNodeMaps[OldNodeElementIndex];
			if (OldVisible)
			{
				const int32 AddElementIndex = OctreeElements.Num();
				const int32 AddNodeIndex = OctreeNodes.Num();
				check(AddElementIndex == AddNodeIndex);
				FBezierGrassOctreeElement& AddElement = OctreeElements.AddDefaulted_GetRef();
				FBezierGrassOctreeNode& AddNode = OctreeNodes.AddDefaulted_GetRef();
				OldNewMaps[OldNodeElementIndex] = AddNodeIndex;
				AddElement.OctreeNodeIndex = AddNodeIndex;
				for (int32 GroupIndex = 0; GroupIndex < OldElement.BezierGrassGroups.Num(); GroupIndex++)
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
		auto GenerateDimension = [](const UINT32 InExp, const FLOAT InExtent, const FLOAT InMinExtent, UINT32& DimensionExp, UINT32& DimensionCellNum, FLOAT& UsedMinExtent)->void
		{
			FLOAT SplitCellNum = EMath::LogX(InExp, InExtent / InMinExtent);
			DimensionExp = EMath::CeilToInt(SplitCellNum);
			DimensionExp = EMath::Max(1, DimensionExp);
			DimensionCellNum = (int32)(EMath::Pow((float)InExp, DimensionExp));
			UsedMinExtent = (InExtent / ((double)(DimensionCellNum))) / ((double)2);
		};

		const FVector TempAnchor = InOrigin - InExtent;
		const double TempExtent[3] = { InExtent.X * ((double)2), InExtent.Y * ((double)2), InExtent.Z * ((double)2) };
		const double TempMinExtent[3] = { InMinExtent.X * ((double)2), InMinExtent.Y * ((double)2), InMinExtent.Z * ((double)2) };

		UE_LOG(LogTemp, Display, TEXT("Used scene length is x : %f, y : %f, z : %f."), TempExtent[0], TempExtent[1], TempExtent[2]);
		UE_LOG(LogTemp, Display, TEXT("Target cell length is x : %f, y : %f, z : %f."), TempMinExtent[0], TempMinExtent[1], TempMinExtent[2]);

		int32 DimensionXYZExp[3]; int32 DimensionXYZ[3]; double UsedMinExtent[3];
		int32 UsedDimensionExp = 1;
		for (int32 DimensionIndex = 0; DimensionIndex < 3; DimensionIndex++)
		{
			GenerateDimension(2, TempExtent[DimensionIndex], TempMinExtent[DimensionIndex], DimensionXYZExp[DimensionIndex], DimensionXYZ[DimensionIndex], UsedMinExtent[DimensionIndex]);
			UsedDimensionExp = FMath::Max(DimensionXYZExp[DimensionIndex], UsedDimensionExp);
		}

		UE_LOG(LogTemp, Display, TEXT("Layers with axis is x : %d, y : %d, z : %d."), DimensionXYZExp[0], DimensionXYZExp[1], DimensionXYZExp[2]);
		UE_LOG(LogTemp, Display, TEXT("Cell number is x : %d, y : %d, z : %d."), DimensionXYZ[0], DimensionXYZ[1], DimensionXYZ[2]);
		UE_LOG(LogTemp, Display, TEXT("Cell length is x : %f, y : %f, z : %f."), (UsedMinExtent[0] * ((double)2)), (UsedMinExtent[1] * ((double)2)), (UsedMinExtent[2] * ((double)2)));

		{
			OutOctreeOrigin = InOrigin;
			OutOctreeSize = FVector(TempExtent[0], TempExtent[1], TempExtent[2]);
			OutOctreeCellSize = FVector(UsedMinExtent[0] * ((double)2), UsedMinExtent[1] * ((double)2), UsedMinExtent[2] * ((double)2));
			OutOctreeMaxDepth = UsedDimensionExp;
			for (int32 DimensionIndex = 0; DimensionIndex < 3; DimensionIndex++)
			{
				OutOctreePerAxisCellNum[DimensionIndex] = DimensionXYZ[DimensionIndex];
				OutOctreeAxisDepth[DimensionIndex] = DimensionXYZExp[DimensionIndex] + 1;
			}
			if (OutOctreeLayerInfos.Num() > 0)
			{
				OutOctreeLayerInfos.Empty();
			}
			OutOctreeLayerInfos.Reserve((UsedDimensionExp + 1));
		}

		{
			int32 LayerCellNum = 0;
			for (int32 LayerIndex = UsedDimensionExp; LayerIndex >= 0; LayerIndex--)
			{
				int32 CurrentLayerCellNum = 1;
				for (int32 DimensionIndex = 0; DimensionIndex < 3; DimensionIndex++)
				{
					CurrentLayerCellNum *= FMath::Max(1, DimensionXYZ[DimensionIndex] / ((int32)(FMath::Pow(2.0f, (float)LayerIndex))));
				}
				LayerCellNum += CurrentLayerCellNum;
			}
			UE_LOG(LogTemp, Display, TEXT("Nodes number is %d."), LayerCellNum);
			if (OutOctreeElements.Num() > 0)
			{
				OutOctreeElements.Empty();
			}
			OutOctreeElements.Reserve(LayerCellNum);
			if (OutOctreeNodes.Num() > 0)
			{
				OutOctreeNodes.Empty();
			}
			OutOctreeNodes.Reserve(LayerCellNum);
			for (int32 LayerIndex = UsedDimensionExp; LayerIndex >= 0; LayerIndex--)
			{
				int32 CurrentLayerDimension[3]; double DimensionExtent[3];
				int32 ParentLayerDimension[3] = { 1, 1, 1 }; double DimensionParentExtent[3] = { 0, 0, 0 };
				for (int32 DimensionIndex = 0; DimensionIndex < 3; DimensionIndex++)
				{
					CurrentLayerDimension[DimensionIndex] = FMath::Max(1, DimensionXYZ[DimensionIndex] / ((int32)(FMath::Pow(2.0f, (float)LayerIndex))));
					DimensionExtent[DimensionIndex] = TempExtent[DimensionIndex] / ((double)(CurrentLayerDimension[DimensionIndex]));
					if (LayerIndex != UsedDimensionExp)
					{
						ParentLayerDimension[DimensionIndex] = FMath::Max(1, DimensionXYZ[DimensionIndex] / ((int32)(FMath::Pow(2.0f, (float)(LayerIndex + 1)))));
						DimensionParentExtent[DimensionIndex] = TempExtent[DimensionIndex] / ((double)(ParentLayerDimension[DimensionIndex]));
					}
				}
				FVector LayerExtentBase = FVector(DimensionExtent[0], DimensionExtent[1], DimensionExtent[2]);
				FVector LayerOriginBase = TempAnchor + (LayerExtentBase / ((double)2));
				FVector ParentLayerExtentBase = FVector(DimensionParentExtent[0], DimensionParentExtent[1], DimensionParentExtent[2]);
				FBezierGrassOctreeLayerInfo& AddInfo = OutOctreeLayerInfos.AddDefaulted_GetRef();
				AddInfo.NodeNum = CurrentLayerDimension[0] * CurrentLayerDimension[1] * CurrentLayerDimension[2];
				AddInfo.Extent = LayerExtentBase / ((double)2);
				AddInfo.StartIndex = 0;
				for (int32 LayerInfoIndex = 0; LayerInfoIndex < (OutOctreeLayerInfos.Num() - 1); LayerInfoIndex++)
				{
					AddInfo.StartIndex += OutOctreeLayerInfos[LayerInfoIndex].NodeNum;
				}
				for (int32 DimensionYIndex = 0; DimensionYIndex < CurrentLayerDimension[1]; DimensionYIndex++)
				{
					for (int32 DimensionXIndex = 0; DimensionXIndex < CurrentLayerDimension[0]; DimensionXIndex++)
					{
						for (int32 DimensionZIndex = 0; DimensionZIndex < CurrentLayerDimension[2]; DimensionZIndex++)
						{
							int32 ElementIndex = OutOctreeElements.Num();
							FBezierGrassOctreeElement& AddElement = OutOctreeElements.AddDefaulted_GetRef();
							int32 NodeIndex = OutOctreeNodes.Num();
							FBezierGrassOctreeNode& AddNode = OutOctreeNodes.AddDefaulted_GetRef();
							{
								AddElement.OctreeNodeIndex = NodeIndex;
							}
							{
								AddNode.Origin = LayerExtentBase * FVector(DimensionXIndex, DimensionYIndex, DimensionZIndex) + LayerOriginBase;
								AddNode.OctreeElementIndex = ElementIndex;
								if (LayerIndex == UsedDimensionExp)
								{
									AddNode.ParentIndex = -1;
								}
								else
								{
									int32 TempParentIndex = 0;
									{
										int32 TempParentIndexX = FMath::Max(((double)0), AddNode.Origin.X - TempAnchor.X) / ParentLayerExtentBase.X;
										int32 TempParentIndexY = FMath::Max(((double)0), AddNode.Origin.Y - TempAnchor.Y) / ParentLayerExtentBase.Y;
										int32 TempParentIndexZ = FMath::Max(((double)0), AddNode.Origin.Z - TempAnchor.Z) / ParentLayerExtentBase.Z;
										int32 TempParentParentCellNum = 0;
										for (int32 ParentParentLayerIndex = UsedDimensionExp; ParentParentLayerIndex > (LayerIndex + 1); ParentParentLayerIndex--)
										{
											int32 TempLayerCellNum = 1;
											for (int32 DimensionIndex = 0; DimensionIndex < 3; DimensionIndex++)
											{
												TempLayerCellNum *= FMath::Max(1, DimensionXYZ[DimensionIndex] / ((int32)(FMath::Pow(2.0f, (float)ParentParentLayerIndex))));
											}
											TempParentParentCellNum += TempLayerCellNum;
										}
										//TODO GetOctreeArrayIndex(OutOctreePerAxisCellNum, const FBezierGrassOctreeLayerInfo& InLayerInfo, int32 InX, int32 InY, int32 InZ);
										TempParentIndex = TempParentParentCellNum + (ParentLayerDimension[0] * ParentLayerDimension[2]) * TempParentIndexY + ParentLayerDimension[2] * TempParentIndexX + TempParentIndexZ;
									}
									check(TempParentIndex < OutOctreeNodes.Num());
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
	*/

};