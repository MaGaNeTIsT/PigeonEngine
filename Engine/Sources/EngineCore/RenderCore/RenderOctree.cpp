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