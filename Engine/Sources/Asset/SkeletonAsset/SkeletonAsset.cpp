#include "SkeletonAsset.h"
#include <RenderDevice/DeviceD3D11.h>
#include <Config/EngineConfig.h>
#include <RenderConfig/RenderConfig.h>
#if _EDITOR_ONLY
#include "../../../EngineThirdParty/assimp/Headers/assimpManager.h"
#endif
#include <Shader/ShaderParameter.h>

namespace PigeonEngine
{

	template <typename _TAssetResourceType, typename _AssetType>
	static void RegisterSkeletonClassTypes()
	{
		RegisterBaseAssetClassTypes<_TAssetResourceType>();
		RegisterClassType<_AssetType, TBaseAsset<_TAssetResourceType>>();
	}

	static void RegisterClassTypes()
	{
		RegisterClassType<ESkeleton, EObjectBase, EResourceInterface>();
		RegisterClassType<ESkeletonRenderResource, EObjectBase, RRenderResourceInterface>();
		RegisterClassType<ESkeletonAssetManager, EManagerBase>();

		RegisterSkeletonClassTypes<ESkeleton, ESkeletonAsset>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	PE_INLINE Matrix4x4 MakeMatrix4x4(const EBoneTransform& InBoneTransform)
	{
		return MakeMatrix4x4(InBoneTransform.Position, InBoneTransform.Rotation, InBoneTransform.Scaling);
	}

	static void BreakBoneRelation(ESkeleton::EBonePart& InOutDatas, USHORT InBoneIndex)
	{
		SHORT BoneNum = static_cast<SHORT>(InOutDatas.Length());
		EBoneData& BoneData = InOutDatas[InBoneIndex];
		if (BoneData.Parent >= 0 && BoneData.Parent < BoneNum)
		{
			InOutDatas[BoneData.Parent].Children.Remove(static_cast<SHORT>(InBoneIndex));
		}
		BoneData.Parent = -2;
		for (UINT32 ChildIndex = 0u, ChildNum = BoneData.Children.Length(); ChildIndex < ChildNum; ChildIndex++)
		{
			if (BoneData.Children[ChildIndex] >= 0 && BoneData.Children[ChildIndex] < BoneNum)
			{
				InOutDatas[BoneData.Children[ChildIndex]].Parent = -2;
			}
		}
		BoneData.Children.Clear();
	}
	static void ExchangeBoneData(ESkeleton::EBonePart& InOutDatas, USHORT InBoneIndex0, USHORT InBoneIndex1)
	{
		SHORT BoneNum = static_cast<SHORT>(InOutDatas.Length());
		EBoneData& BoneData0 = InOutDatas[InBoneIndex0];
		EBoneData& BoneData1 = InOutDatas[InBoneIndex1];
		if (BoneData0.Parent >= 0 && BoneData0.Parent < BoneNum)
		{
			InOutDatas[BoneData0.Parent].Children.Remove(static_cast<SHORT>(InBoneIndex0));
			InOutDatas[BoneData0.Parent].Children.Add(static_cast<SHORT>(InBoneIndex1));
		}
		if (BoneData1.Parent >= 0 && BoneData1.Parent < BoneNum)
		{
			InOutDatas[BoneData1.Parent].Children.Remove(static_cast<SHORT>(InBoneIndex1));
			InOutDatas[BoneData1.Parent].Children.Add(static_cast<SHORT>(InBoneIndex0));
		}
		for (UINT32 ChildIndex = 0u, ChildNum = BoneData0.Children.Length(); ChildIndex < ChildNum; ChildIndex++)
		{
			if (BoneData0.Children[ChildIndex] >= 0 && BoneData0.Children[ChildIndex] < BoneNum)
			{
				InOutDatas[BoneData0.Children[ChildIndex]].Parent = static_cast<SHORT>(InBoneIndex1);
			}
		}
		for (UINT32 ChildIndex = 0u, ChildNum = BoneData1.Children.Length(); ChildIndex < ChildNum; ChildIndex++)
		{
			if (BoneData1.Children[ChildIndex] >= 0 && BoneData1.Children[ChildIndex] < BoneNum)
			{
				InOutDatas[BoneData1.Children[ChildIndex]].Parent = static_cast<SHORT>(InBoneIndex0);
			}
		}
		EBoneData TempBoneData;
		TempBoneData.Index = InBoneIndex0;
		TempBoneData.Name = BoneData0.Name;
		TempBoneData.DefaultPosition = BoneData0.DefaultPosition;
		TempBoneData.DefaultRotation = BoneData0.DefaultRotation;
		TempBoneData.DefaultScaling = BoneData0.DefaultScaling;
		TempBoneData.Parent = BoneData0.Parent;
		if (BoneData0.Children.Length() > 0u)
		{
			TempBoneData.Children.Recapacity(BoneData0.Children.Length());
			for (UINT32 ChildIndex = 0u, ChildNum = BoneData0.Children.Length(); ChildIndex < ChildNum; ChildIndex++)
			{
				if (BoneData0.Children[ChildIndex] >= 0 && BoneData0.Children[ChildIndex] < BoneNum)
				{
					TempBoneData.Children.Add(BoneData0.Children[ChildIndex]);
				}
			}
		}
		BoneData0.Index = InBoneIndex1;
		BoneData0.Name = BoneData1.Name;
		BoneData0.DefaultPosition = BoneData1.DefaultPosition;
		BoneData0.DefaultRotation = BoneData1.DefaultRotation;
		BoneData0.DefaultScaling = BoneData1.DefaultScaling;
		BoneData0.Parent = BoneData1.Parent;
		BoneData0.Children.Clear();
		if (BoneData1.Children.Length() > 0)
		{
			BoneData0.Children.Recapacity(BoneData1.Children.Length());
			for (UINT32 ChildIndex = 0u, ChildNum = BoneData1.Children.Length(); ChildIndex < ChildNum; ChildIndex++)
			{
				if (BoneData1.Children[ChildIndex] >= 0 && BoneData1.Children[ChildIndex] < BoneNum)
				{
					BoneData0.Children.Add(BoneData1.Children[ChildIndex]);
				}
			}
		}
		BoneData1.Index = InBoneIndex0;
		BoneData1.Name = TempBoneData.Name;
		BoneData1.DefaultPosition = TempBoneData.DefaultPosition;
		BoneData1.DefaultRotation = TempBoneData.DefaultRotation;
		BoneData1.DefaultScaling = TempBoneData.DefaultScaling;
		BoneData1.Parent = TempBoneData.Parent;
		BoneData1.Children.Clear();
		if (TempBoneData.Children.Length() > 0u)
		{
			BoneData1.Children.Recapacity(TempBoneData.Children.Length());
			for (UINT32 ChildIndex = 0u, ChildNum = TempBoneData.Children.Length(); ChildIndex < ChildNum; ChildIndex++)
			{
				BoneData1.Children.Add(TempBoneData.Children[ChildIndex]);
			}
		}
	}
	static void RecursionRemoveBone(ESkeleton::EBonePart& InOutDatas, USHORT InBoneIndex)
	{
		USHORT CurrentBoneNum = static_cast<USHORT>(InOutDatas.Length());
		if (CurrentBoneNum > 0u)
		{
			if (InBoneIndex >= CurrentBoneNum)
			{
				return;
			}

			USHORT LastBoneIndex = CurrentBoneNum - 1u;
			if (InBoneIndex != (CurrentBoneNum - 1u))
			{
				ExchangeBoneData(InOutDatas, InBoneIndex, LastBoneIndex);
			}

			TArray<SHORT> TempChildren;
			if (InOutDatas[InBoneIndex].Children.Length() > 0u)
			{
				SHORT TempBoneNum = static_cast<SHORT>(CurrentBoneNum);
				TempChildren.Resize(InOutDatas[InBoneIndex].Children.Length());
				for (UINT32 ChildIndex = 0u, ChildNum = InOutDatas[InBoneIndex].Children.Length(); ChildIndex < ChildNum; ChildIndex++)
				{
					SHORT TempChildIndex = InOutDatas[InBoneIndex].Children[ChildIndex];
					if (TempChildIndex >= 0 && TempChildIndex < TempBoneNum)
					{
						TempChildren[ChildIndex] = TempChildIndex;
					}
					else
					{
						TempChildren[ChildIndex] = -2;
					}
				}
			}

			{
				BreakBoneRelation(InOutDatas, InBoneIndex);
				InOutDatas.RemoveAt(LastBoneIndex);
			}

			if (TempChildren.Length() > 0u)
			{
				for (UINT32 ChildIndex = 0u, ChildNum = TempChildren.Length(); ChildIndex < ChildNum; ChildIndex++)
				{
					if (TempChildren[ChildIndex] < 0)
					{
						continue;
					}
					RecursionRemoveBone(InOutDatas, static_cast<USHORT>(TempChildren[ChildIndex]));
				}
			}
		}
	}

	ESkeleton::ESkeleton(const EString& InSkeletonName)
		: SkeletonType(ESkeletonType::SKELETON_TYPE_NORMAL), SkeletonName(InSkeletonName)
	{
#if _EDITOR_ONLY
		DebugName = InSkeletonName;
#endif
	}
	ESkeleton::~ESkeleton()
	{
		ReleaseResource();
	}
	BOOL32 ESkeleton::IsResourceValid()const
	{
		return ((Bones.Length() > 0u) && (Bones.Length() == BoneMapping.Length()));
	}
	BOOL32 ESkeleton::InitResource()
	{
		// Skeleton resource must init by skeleton manager.
		return TRUE;
	}
	void ESkeleton::ReleaseResource()
	{
		SkeletonName = ESettings::ENGINE_DEFAULT_NAME;
		Bones.Clear();
		BoneMapping.Clear();
	}
	ESkeletonType ESkeleton::GetSkeletonType()const
	{
		return SkeletonType;
	}
	const EString& ESkeleton::GetSkeletonName()const
	{
		return SkeletonName;
	}
	ESkeleton::EBonePart& ESkeleton::GetBonePart()
	{
		return Bones;
	}
	const ESkeleton::EBonePart& ESkeleton::GetBonePart()const
	{
		return Bones;
	}
	TMap<EString, USHORT>& ESkeleton::GetBoneMapping()
	{
		return BoneMapping;
	}
	const TMap<EString, USHORT>& ESkeleton::GetBoneMapping()const
	{
		return BoneMapping;
	}
	UINT32 ESkeleton::GetBoneCount()const
	{
		Check((Bones.Length() == BoneMapping.Length()));
		return (Bones.Length());
	}
	USHORT ESkeleton::GetBoneIndex(const EString& InBoneName)const
	{
#if _EDITOR_ONLY
		if (const USHORT* BoneIndex = BoneMapping.FindValueAsPtr(InBoneName); !!BoneIndex)
#endif
		{
			return (*BoneIndex);
		}
#if _EDITOR_ONLY
		else
		{
			EString ErrorInfo("Can not find [");
			ErrorInfo = ErrorInfo + InBoneName + "] bone in [" + SkeletonName + "] skeleton.";
			PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorInfo));
		}
		return (static_cast<USHORT>(-1));
#endif
	}
	BOOL32 ESkeleton::AddBoneElement(EBoneData* InIndexData)
	{
		if (!InIndexData)
		{
			return FALSE;
		}
		USHORT TempIndex = 0u;
		if (BoneMapping.FindValue(InIndexData->Name, TempIndex))
		{
			return FALSE;
		}
		const UINT32 NewIndex = Bones.Length();
		Bones.Add(EBoneData());
		BoneMapping.Add(InIndexData->Name, static_cast<USHORT>(NewIndex));
		Bones[NewIndex].Index			= static_cast<SHORT>(NewIndex);
		Bones[NewIndex].Name			= InIndexData->Name;
		Bones[NewIndex].DefaultPosition	= InIndexData->DefaultPosition;
		Bones[NewIndex].DefaultRotation	= InIndexData->DefaultRotation;
		Bones[NewIndex].DefaultScaling	= InIndexData->DefaultScaling;
		Bones[NewIndex].Parent			= static_cast<SHORT>(InIndexData->Parent);
		for (UINT32 ChildIndex = 0u, ChildrenNum = InIndexData->Children.Length(); ChildIndex < ChildrenNum; ChildIndex++)
		{
			Bones[NewIndex].Children.Add(InIndexData->Children[ChildIndex]);
		}
		return TRUE;
	}
	BOOL32 ESkeleton::RemoveBoneElement(const EString& InBoneName)
	{
		if (Bones.Length() == 0u)
		{
			PE_FAILED((ENGINE_ASSET_ERROR), ("Try to removing bone from empty skeleton."));
			return FALSE;
		}
		USHORT TempIndex = 0u;
		if (!BoneMapping.FindValue(InBoneName, TempIndex))
		{
#if (defined(_DEBUG_MODE) || defined(_DEVELOP_MODE))
			EString ErrorInfo("Try to removing not exist bone [");
			ErrorInfo = ErrorInfo + InBoneName + "] in skeleton [";
			ErrorInfo = ErrorInfo + SkeletonName + "].";
			PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorInfo));
#endif
			return FALSE;
		}
#if (defined(_DEBUG_MODE) || defined(_DEVELOP_MODE))
		if (TempIndex >= Bones.Length())
		{
			PE_FAILED((ENGINE_ASSET_ERROR), ("Check error that index out of bones list."));
			return FALSE;
		}
#endif
		RemoveBoneInternal(TempIndex);
		return TRUE;
	}
	BOOL32 ESkeleton::RemoveBoneElement(USHORT InBoneIndex, EString* OutBoneName)
	{
		if (Bones.Length() == 0u)
		{
			PE_FAILED((ENGINE_ASSET_ERROR), ("Try to removing bone from empty skeleton."));
			return FALSE;
		}
#if (defined(_DEBUG_MODE) || defined(_DEVELOP_MODE))
		if (InBoneIndex >= Bones.Length())
		{
			PE_FAILED((ENGINE_ASSET_ERROR), ("Check error that index out of bones list."));
			return FALSE;
		}
#endif
		if (OutBoneName)
		{
			(*OutBoneName) = Bones[InBoneIndex].Name;
		}
		RemoveBoneInternal(InBoneIndex);
		return TRUE;
	}
	BOOL32 ESkeleton::GetBoneElement(const EString& InBoneName, const EBoneData*& OutBoneData)const
	{
		USHORT TempIndex = 0u;
		if (!BoneMapping.FindValue(InBoneName, TempIndex))
		{
#if (defined(_DEBUG_MODE) || defined(_DEVELOP_MODE))
			EString ErrorInfo("Try to getting not exist bone [");
			ErrorInfo = ErrorInfo + InBoneName + "] in skeleton [";
			ErrorInfo = ErrorInfo + SkeletonName + "].";
			PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorInfo));
#endif
			return FALSE;
		}
		return (GetBoneElement(TempIndex, OutBoneData));
	}
	BOOL32 ESkeleton::GetBoneElement(USHORT InBoneIndex, const EBoneData*& OutBoneData)const
	{
		if (Bones.Length() == 0u)
		{
			PE_FAILED((ENGINE_ASSET_ERROR), ("Try to removing bone from empty skeleton."));
			return FALSE;
		}
#if (defined(_DEBUG_MODE) || defined(_DEVELOP_MODE))
		if (InBoneIndex >= Bones.Length())
		{
			PE_FAILED((ENGINE_ASSET_ERROR), ("Check error that index out of bones list."));
			return FALSE;
		}
#endif
		OutBoneData = static_cast<const EBoneData*>(&(Bones[InBoneIndex]));
		return TRUE;
	}
	void ESkeleton::RemoveBoneInternal(USHORT InBoneIndex)
	{
		RecursionRemoveBone(this->Bones, InBoneIndex);
	}

	ESkeletonRenderResource::ESkeletonRenderResource()
		: BoneNum(0u)
	{
	}
	ESkeletonRenderResource::~ESkeletonRenderResource()
	{
		ReleaseRenderResource();
	}
	void ESkeletonRenderResource::SetBoneNum(const UINT32 InBoneNum)
	{
		if (BoneNum != InBoneNum)
		{
			BoneNum = InBoneNum;
			{
#if _EDITOR_ONLY
				BOOL32 Result =
#endif
					InitRenderResource();
#if _EDITOR_ONLY
				PE_CHECK((ENGINE_ASSET_ERROR), ("New skeleton render resource create failed."), (Result));
#endif
			}
		}
	}
	UINT32 ESkeletonRenderResource::GetBoneNum()const
	{
		return BoneNum;
	}
	const RStructuredBuffer& ESkeletonRenderResource::GetRenderResource(ESkeletonRenderResourceType InType)const
	{
		Check(((InType >= ESkeletonRenderResourceType::SKELETON_RENDER_RESOURCE_MATRIX) && (InType < ESkeletonRenderResourceType::SKELETON_RENDER_RESOURCE_COUNT)), (ENGINE_ASSET_ERROR));
		return (RenderResource[InType]);
	}
	void ESkeletonRenderResource::UpdateRenderResource(const TArray<Matrix4x4>& InMatrices)
	{
		const UINT32 InMatrixNum = InMatrices.Length();
#if _EDITOR_ONLY
		if ((InMatrixNum > 0u) && (InMatrixNum <= RCommonSettings::RENDER_MESH_BONE_NUM_MAX))
#endif
		{
			Check((IsRenderResourceValid()), (ENGINE_ASSET_ERROR));
			Check((BoneNum == InMatrices.Length()), (ENGINE_ASSET_ERROR));
			Matrix4x4* NewMatrices[ESkeletonRenderResourceType::SKELETON_RENDER_RESOURCE_COUNT];
			NewMatrices[ESkeletonRenderResourceType::SKELETON_RENDER_RESOURCE_MATRIX] = new Matrix4x4[InMatrixNum];
			const UINT32 MatrixSize = sizeof(Matrix4x4) * InMatrixNum;
			::memcpy_s(NewMatrices[ESkeletonRenderResourceType::SKELETON_RENDER_RESOURCE_MATRIX], MatrixSize, InMatrices.RawData(), MatrixSize);
			NewMatrices[ESkeletonRenderResourceType::SKELETON_RENDER_RESOURCE_INVERSE_TRANSPOSE_MATRIX] = new Matrix4x4[InMatrixNum];
			for (UINT32 i = 0u; i < InMatrixNum; i++)
			{
				(NewMatrices[ESkeletonRenderResourceType::SKELETON_RENDER_RESOURCE_INVERSE_TRANSPOSE_MATRIX])[i] = TranslateUploadTransposeMatrixType((NewMatrices[ESkeletonRenderResourceType::SKELETON_RENDER_RESOURCE_MATRIX])[i].Inverse());
				(NewMatrices[ESkeletonRenderResourceType::SKELETON_RENDER_RESOURCE_MATRIX])[i] = TranslateUploadMatrixType((NewMatrices[ESkeletonRenderResourceType::SKELETON_RENDER_RESOURCE_MATRIX])[i]);
			}
			RDeviceD3D11* RenderDevice = RDeviceD3D11::GetDeviceSingleton();
			for (UINT32 i = 0u, n = ESkeletonRenderResourceType::SKELETON_RENDER_RESOURCE_COUNT; i < n; i++)
			{
				RenderDevice->UploadBuffer(RenderResource[i].Buffer, NewMatrices[i], MatrixSize, MatrixSize);
			}
			delete[](NewMatrices[ESkeletonRenderResourceType::SKELETON_RENDER_RESOURCE_MATRIX]);
			delete[](NewMatrices[ESkeletonRenderResourceType::SKELETON_RENDER_RESOURCE_INVERSE_TRANSPOSE_MATRIX]);
		}
	}
	BOOL32 ESkeletonRenderResource::IsRenderResourceValid()const
	{
		BOOL32 Result = (BoneNum > 0u) && (BoneNum <= RCommonSettings::RENDER_MESH_BONE_NUM_MAX);
		if (Result)
		{
			for (UINT32 i = 0u, n = ESkeletonRenderResourceType::SKELETON_RENDER_RESOURCE_COUNT; i < n; i++)
			{
				Result = Result && (RenderResource[i].IsRenderResourceValid());
			}
		}
		return Result;
	}
	BOOL32 ESkeletonRenderResource::InitRenderResource()
	{
		for (UINT32 i = 0u, n = ESkeletonRenderResourceType::SKELETON_RENDER_RESOURCE_COUNT; i < n; i++)
		{
			if (RenderResource[i].IsRenderResourceValid())
			{
				RenderResource[i].ReleaseRenderResource();
			}
		}
		if ((BoneNum == 0u) || (BoneNum > RCommonSettings::RENDER_MESH_BONE_NUM_MAX))
		{
			PE_FAILED((ENGINE_ASSET_ERROR), ("Skeleton asset init render resource failed(bone num error)."));
			return FALSE;
		}
		const UINT32 SkeletonBoneNum = BoneNum;
		for (UINT32 i = 0u, n = ESkeletonRenderResourceType::SKELETON_RENDER_RESOURCE_COUNT; i < n; i++)
		{
			Matrix4x4* SkeletonMatrices = new Matrix4x4[SkeletonBoneNum];
			{
				for (UINT32 i = 0u; i < SkeletonBoneNum; i++)
				{
					SkeletonMatrices[i] = Matrix4x4::Identity();
				}
			}
			RSubresourceDataDesc SubresourceDataDesc;
			SubresourceDataDesc.pSysMem = SkeletonMatrices;
			if (!(RDeviceD3D11::GetDeviceSingleton()->CreateStructuredBuffer(
				(RenderResource[i]),
				RStructuredBufferDesc(sizeof(Matrix4x4), SkeletonBoneNum, FALSE),
				(&SubresourceDataDesc))))
			{
#if _EDITOR_ONLY
				EString ErrorInfo("Create skeleton resource bones buffer [num = ");
				ErrorInfo += ToString(SkeletonBoneNum);
				ErrorInfo += "] failed.";
				PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorInfo));
#endif
				delete[]SkeletonMatrices;
				return FALSE;
			}
			delete[]SkeletonMatrices;
		}
		return TRUE;
	}
	void ESkeletonRenderResource::ReleaseRenderResource()
	{
		BoneNum = 0u;
		for (UINT32 i = 0u, n = ESkeletonRenderResourceType::SKELETON_RENDER_RESOURCE_COUNT; i < n; i++)
		{
			RenderResource[i].ReleaseRenderResource();
		}
	}

	ESkeletonBoneMemoryPool::ESkeletonBoneMemoryPool(const ESkeleton* InRawSkeletonPtr)
		: RawSkeletonPtr(InRawSkeletonPtr)
	{
		GenerateFromSkeleton(InRawSkeletonPtr);
	}
	ESkeletonBoneMemoryPool::ESkeletonBoneMemoryPool()
		: RawSkeletonPtr(nullptr)
	{
	}
	ESkeletonBoneMemoryPool::ESkeletonBoneMemoryPool(const ESkeletonBoneMemoryPool& Other)
		: RawSkeletonPtr(Other.RawSkeletonPtr)
		, BoneRelativeTransforms(Other.BoneRelativeTransforms)
		, BoneToRootTransforms(Other.BoneToRootTransforms)
	{
	}
	ESkeletonBoneMemoryPool::~ESkeletonBoneMemoryPool()
	{
	}
	ESkeletonBoneMemoryPool& ESkeletonBoneMemoryPool::operator=(const ESkeletonBoneMemoryPool& Other)
	{
		if (BoneRelativeTransforms.Length() > 0u)
		{
			BoneRelativeTransforms.Clear();
		}
		if (BoneToRootTransforms.Length() > 0u)
		{
			BoneToRootTransforms.Clear();
		}
		RawSkeletonPtr			= Other.RawSkeletonPtr;
		BoneRelativeTransforms	= Other.BoneRelativeTransforms;
		BoneToRootTransforms	= Other.BoneToRootTransforms;
		return (*this);
	}
	BOOL32 ESkeletonBoneMemoryPool::IsValid()const
	{
		return ((!!RawSkeletonPtr) && (BoneRelativeTransforms.Length() > 0u) && (BoneRelativeTransforms.Length() == BoneToRootTransforms.Length()));
	}
	void ESkeletonBoneMemoryPool::GenerateFromSkeleton(const ESkeleton* InRawSkeletonPtr)
	{
		if (BoneRelativeTransforms.Length() > 0u)
		{
			BoneRelativeTransforms.Clear();
		}
		if (BoneToRootTransforms.Length() > 0u)
		{
			BoneToRootTransforms.Clear();
		}
		RawSkeletonPtr = InRawSkeletonPtr;
		if (!!RawSkeletonPtr)
		{
			const TMap<EString, USHORT>& BoneMappings = RawSkeletonPtr->GetBoneMapping();
			const ESkeleton::EBonePart& BonePart = RawSkeletonPtr->GetBonePart();
			Check(((BoneMappings.Length() > 0u) && (BoneMappings.Length() == BonePart.Length())));
			const UINT32 BoneNum = BonePart.Length();
			BoneRelativeTransforms.Resize(BoneNum);
			BoneToRootTransforms.Resize(BoneNum);
			for (UINT32 BoneIndex = 0u; BoneIndex < BoneNum; BoneIndex++)
			{
				BoneRelativeTransforms[BoneIndex].Position	= BonePart[BoneIndex].DefaultPosition;
				BoneRelativeTransforms[BoneIndex].Rotation	= BonePart[BoneIndex].DefaultRotation;
				BoneRelativeTransforms[BoneIndex].Scaling	= BonePart[BoneIndex].DefaultScaling;
			}
			RecursionToRootTransforms();
		}
	}
	void ESkeletonBoneMemoryPool::RecursionToRootTransforms()
	{
#if _EDITOR_ONLY
		if (IsValid())
#endif
		{
			TArray<EBoneTransform>& UsedRelativeTransforms = BoneRelativeTransforms;
			TArray<EBoneTransform>& UsedToRootTransforms = BoneToRootTransforms;
#if _EDITOR_ONLY
			const UINT32 UsedRelativeTransformNum = UsedRelativeTransforms.Length();
#endif
			BackwardRecursionBone(
				0,
				EBoneTransform::Identity(),
				[&UsedRelativeTransforms, &UsedToRootTransforms
#if _EDITOR_ONLY
				, &UsedRelativeTransformNum
#endif
				](const EBoneData& InBoneData, EBoneTransform& InBoneTransform)->void
				{
					Check((InBoneData.Index >= 0));
					Check((((UINT32)(InBoneData.Index)) < UsedRelativeTransformNum));
					//Matrix4x4 TempGlobalTransform = UsedRelativeTransforms[InBoneData.Index].ToMatrix4x4().Transpose();
					//TempGlobalTransform = InMatrix * TempGlobalTransform;
					//UsedToRootTransforms[InBoneData.Index] = TempGlobalTransform;
					//InMatrix = TempGlobalTransform;
					const EBoneTransform& TempRelativeBoneTransform = UsedRelativeTransforms[InBoneData.Index];
					InBoneTransform.Position += TempRelativeBoneTransform.Position;
					InBoneTransform.Rotation = TempRelativeBoneTransform.Rotation * InBoneTransform.Rotation;
					InBoneTransform.Scaling *= TempRelativeBoneTransform.Scaling;
					UsedToRootTransforms[InBoneData.Index] = InBoneTransform;
				},
				[](const EBoneData& InBoneData, const EBoneTransform& InMatrix)->BOOL32
				{
					return TRUE;
				});
		}
#if _EDITOR_ONLY
		else
		{
			EString ErrorInfo("Skeleton [");
			ErrorInfo = ErrorInfo + (RawSkeletonPtr ? RawSkeletonPtr->GetSkeletonName() : "null") + "] could not do [RecursionToRootTransforms()].";
			PE_FAILED((ENGINE_UNKNOWN_ERROR), (*ErrorInfo));
		}
#endif
	}
	const ESkeleton* ESkeletonBoneMemoryPool::GetRawSkeleton()const
	{
		return RawSkeletonPtr;
	}
	const TArray<EBoneTransform>& ESkeletonBoneMemoryPool::GetBoneRelativeTransforms()const
	{
		return BoneRelativeTransforms;
	}
	TArray<EBoneTransform>& ESkeletonBoneMemoryPool::GetBoneRelativeTransforms()
	{
		return BoneRelativeTransforms;
	}
	const TArray<EBoneTransform>& ESkeletonBoneMemoryPool::GetBoneToRootTransforms()const
	{
		return BoneToRootTransforms;
	}
	TArray<EBoneTransform>& ESkeletonBoneMemoryPool::GetBoneToRootTransforms()
	{
		return BoneToRootTransforms;
	}
	const EBoneTransform& ESkeletonBoneMemoryPool::GetBoneRelativeTransform(const EString& InBoneName)const
	{
		CheckSlow(IsValid());
#if _EDITOR_ONLY
		if (const UINT32 BoneIndex = (UINT32)(RawSkeletonPtr->GetBoneIndex(InBoneName)); BoneIndex < BoneRelativeTransforms.Length())
#endif
		{
			return (BoneRelativeTransforms[BoneIndex]);
		}
#if _EDITOR_ONLY
		else
		{
			EString ErrorInfo("Can not find [");
			ErrorInfo = ErrorInfo + InBoneName + "] bone in [" + (RawSkeletonPtr ? RawSkeletonPtr->GetSkeletonName() : "null") + "] skeleton.";
			PE_FAILED((ENGINE_UNKNOWN_ERROR), (*ErrorInfo));
		}
		return (EBoneTransform::Identity());
#endif
	}
	const EBoneTransform& ESkeletonBoneMemoryPool::GetBoneToRootTransform(const EString& InBoneName)const
	{
		CheckSlow(IsValid());
#if _EDITOR_ONLY
		if (const UINT32 BoneIndex = (UINT32)(RawSkeletonPtr->GetBoneIndex(InBoneName)); BoneIndex < BoneRelativeTransforms.Length())
#endif
		{
			return (BoneToRootTransforms[BoneIndex]);
		}
#if _EDITOR_ONLY
		else
		{
			EString ErrorInfo("Can not find [");
			ErrorInfo = ErrorInfo + InBoneName + "] bone in [" + (RawSkeletonPtr ? RawSkeletonPtr->GetSkeletonName() : "null") + "] skeleton.";
			PE_FAILED((ENGINE_UNKNOWN_ERROR), (*ErrorInfo));
		}
		return (EBoneTransform::Identity());
#endif
	}
	void ESkeletonBoneMemoryPool::SetBoneRelativePosition(const EString& InBoneName, const Vector3& InPosition)
	{
		CheckSlow(IsValid());
#if _EDITOR_ONLY
		if (const UINT32 BoneIndex = (UINT32)(RawSkeletonPtr->GetBoneIndex(InBoneName)); BoneIndex < BoneRelativeTransforms.Length())
#endif
		{
			BoneRelativeTransforms[BoneIndex].Position = InPosition;
		}
#if _EDITOR_ONLY
		else
		{
			EString ErrorInfo("Can not find [");
			ErrorInfo = ErrorInfo + InBoneName + "] bone in [" + (RawSkeletonPtr ? RawSkeletonPtr->GetSkeletonName() : "null") + "] skeleton.";
			PE_FAILED((ENGINE_UNKNOWN_ERROR), (*ErrorInfo));
		}
#endif
	}
	void ESkeletonBoneMemoryPool::SetBoneRelativeRotation(const EString& InBoneName, const Quaternion& InRotation)
	{
		CheckSlow(IsValid());
#if _EDITOR_ONLY
		if (const UINT32 BoneIndex = (UINT32)(RawSkeletonPtr->GetBoneIndex(InBoneName)); BoneIndex < BoneRelativeTransforms.Length())
#endif
		{
			BoneRelativeTransforms[BoneIndex].Rotation = InRotation;
		}
#if _EDITOR_ONLY
		else
		{
			EString ErrorInfo("Can not find [");
			ErrorInfo = ErrorInfo + InBoneName + "] bone in [" + (RawSkeletonPtr ? RawSkeletonPtr->GetSkeletonName() : "null") + "] skeleton.";
			PE_FAILED((ENGINE_UNKNOWN_ERROR), (*ErrorInfo));
		}
#endif
	}
	void ESkeletonBoneMemoryPool::SetBoneRelativeScaling(const EString& InBoneName, const Vector3& InScaling)
	{
		CheckSlow(IsValid());
#if _EDITOR_ONLY
		if (const UINT32 BoneIndex = (UINT32)(RawSkeletonPtr->GetBoneIndex(InBoneName)); BoneIndex < BoneRelativeTransforms.Length())
#endif
		{
			BoneRelativeTransforms[BoneIndex].Scaling = InScaling;
		}
#if _EDITOR_ONLY
		else
		{
			EString ErrorInfo("Can not find [");
			ErrorInfo = ErrorInfo + InBoneName + "] bone in [" + (RawSkeletonPtr ? RawSkeletonPtr->GetSkeletonName() : "null") + "] skeleton.";
			PE_FAILED((ENGINE_UNKNOWN_ERROR), (*ErrorInfo));
		}
#endif
	}
	void ESkeletonBoneMemoryPool::SetBoneRelativeTransform(const EString& InBoneName, const Vector3& InPosition, const Quaternion& InRotation)
	{
		CheckSlow(IsValid());
#if _EDITOR_ONLY
		if (const UINT32 BoneIndex = (UINT32)(RawSkeletonPtr->GetBoneIndex(InBoneName)); BoneIndex < BoneRelativeTransforms.Length())
#endif
		{
			BoneRelativeTransforms[BoneIndex].Position = InPosition;
			BoneRelativeTransforms[BoneIndex].Rotation = InRotation;
		}
#if _EDITOR_ONLY
		else
		{
			EString ErrorInfo("Can not find [");
			ErrorInfo = ErrorInfo + InBoneName + "] bone in [" + (RawSkeletonPtr ? RawSkeletonPtr->GetSkeletonName() : "null") + "] skeleton.";
			PE_FAILED((ENGINE_UNKNOWN_ERROR), (*ErrorInfo));
		}
#endif
	}
	void ESkeletonBoneMemoryPool::SetBoneRelativeTransform(const EString& InBoneName, const Vector3& InPosition, const Vector3& InScaling)
	{
		CheckSlow(IsValid());
#if _EDITOR_ONLY
		if (const UINT32 BoneIndex = (UINT32)(RawSkeletonPtr->GetBoneIndex(InBoneName)); BoneIndex < BoneRelativeTransforms.Length())
#endif
		{
			BoneRelativeTransforms[BoneIndex].Position = InPosition;
			BoneRelativeTransforms[BoneIndex].Scaling = InScaling;
		}
#if _EDITOR_ONLY
		else
		{
			EString ErrorInfo("Can not find [");
			ErrorInfo = ErrorInfo + InBoneName + "] bone in [" + (RawSkeletonPtr ? RawSkeletonPtr->GetSkeletonName() : "null") + "] skeleton.";
			PE_FAILED((ENGINE_UNKNOWN_ERROR), (*ErrorInfo));
		}
#endif
	}
	void ESkeletonBoneMemoryPool::SetBoneRelativeTransform(const EString& InBoneName, const Quaternion& InRotation, const Vector3& InScaling)
	{
		CheckSlow(IsValid());
#if _EDITOR_ONLY
		if (const UINT32 BoneIndex = (UINT32)(RawSkeletonPtr->GetBoneIndex(InBoneName)); BoneIndex < BoneRelativeTransforms.Length())
#endif
		{
			BoneRelativeTransforms[BoneIndex].Rotation = InRotation;
			BoneRelativeTransforms[BoneIndex].Scaling = InScaling;
		}
#if _EDITOR_ONLY
		else
		{
			EString ErrorInfo("Can not find [");
			ErrorInfo = ErrorInfo + InBoneName + "] bone in [" + (RawSkeletonPtr ? RawSkeletonPtr->GetSkeletonName() : "null") + "] skeleton.";
			PE_FAILED((ENGINE_UNKNOWN_ERROR), (*ErrorInfo));
		}
#endif
	}
	void ESkeletonBoneMemoryPool::SetBoneRelativeTransform(const EString& InBoneName, const EBoneTransform& InBoneTransform)
	{
		CheckSlow(IsValid());
#if _EDITOR_ONLY
		if (const UINT32 BoneIndex = (UINT32)(RawSkeletonPtr->GetBoneIndex(InBoneName)); BoneIndex < BoneRelativeTransforms.Length())
#endif
		{
			BoneRelativeTransforms[BoneIndex] = InBoneTransform;
		}
#if _EDITOR_ONLY
		else
		{
			EString ErrorInfo("Can not find [");
			ErrorInfo = ErrorInfo + InBoneName + "] bone in [" + (RawSkeletonPtr ? RawSkeletonPtr->GetSkeletonName() : "null") + "] skeleton.";
			PE_FAILED((ENGINE_UNKNOWN_ERROR), (*ErrorInfo));
		}
#endif
	}

	ESkeletonAsset::ESkeletonAsset(const EString& InAssetPath, const EString& InAssetName
#if _EDITOR_ONLY
		, const EString& InDebugName
#endif
	) : TBaseAsset<ESkeleton>(InAssetPath, InAssetName
#if _EDITOR_ONLY
		, InDebugName
#endif
	)
	{

	}
	ESkeletonAsset::~ESkeletonAsset()
	{
	}
	BOOL32 ESkeletonAsset::InitResource()
	{
		// Skeleton resource do not need to initialize.
		if (IsInitialized())
		{
#if _EDITOR_ONLY
			{
				EString ErrorInfo = EString("Skeleton name=[") + GetAssetName() + "] path = [" + GetAssetPath() + "] has been Initialized.";
				PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorInfo));
			}
#endif
			return TRUE;
		}
		bIsInitialized = TRUE;
		return TRUE;
	}

	ESkeletonAssetManager::ESkeletonAssetManager()
	{
#if _EDITOR_ONLY
		DebugName = ENGINE_SKELETON_ASSET_MANAGER_NAME;
#endif
	}
	ESkeletonAssetManager::~ESkeletonAssetManager()
	{
	}
	void ESkeletonAssetManager::Initialize()
	{

	}
	void ESkeletonAssetManager::ShutDown()
	{
		ClearSkeletons();
	}
#if _EDITOR_ONLY
	BOOL32 ESkeletonAssetManager::ImportSkeleton(const EString& InAssetName, const EString& InImportFullPathName, const EString& InSavePath)
	{
		EString TempFullPathName(InSavePath);
		TempFullPathName = TempFullPathName + InAssetName + ESettings::ENGINE_ASSET_NAME_TYPE;
		if ((InImportFullPathName.Length() < 3u) || (TempFullPathName.Length() < 10u))
		{
#if _EDITOR_ONLY
			{
				EString ErrorData("Error file path for skeleton importer (import file path : ");
				ErrorData = ErrorData + InImportFullPathName + ", save assset path : " + TempFullPathName + ").";
				PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorData));
			}
#endif
			return FALSE;
		}
		EString ImportPathName; EString ImportFileType;
		if (!(SplitByLastSign('.', InImportFullPathName, ImportPathName, ImportFileType)))
		{
#if _EDITOR_ONLY
			{
				EString ErrorData("Error file path for skeleton importer (import file path : ");
				ErrorData = ErrorData + InImportFullPathName + ", save assset path : " + TempFullPathName + ").";
				PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorData));
			}
#endif
			return FALSE;
		}
		//TODO Check import type(like fbx, ab...).
		if (ImportPathName.Length() <= 3u)
		{
#if _EDITOR_ONLY
			{
				EString ErrorData("Error file path for skeleton importer (import file path : ");
				ErrorData = ErrorData + InImportFullPathName + ", save assset path : " + TempFullPathName + ").";
				PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorData));
			}
#endif
			return FALSE;
		}
		EString UsedSkeletonName;
		if (InAssetName.Length() < 2u)
		{
			UsedSkeletonName = ESettings::ENGINE_DEFAULT_NAME;
		}
		else
		{
			UsedSkeletonName = InAssetName;
		}
		ESkeleton AssimpSkeleton(UsedSkeletonName);
		CAssimpManager* AssimpManager = CAssimpManager::GetManagerSingleton();
		if ((AssimpManager->ReadSkeletonFile(InImportFullPathName, AssimpSkeleton)) != (CAssimpManager::CReadFileStateType::ASSIMP_READ_FILE_STATE_SUCCEED))
		{
#if _EDITOR_ONLY
			{
				EString ErrorData("Assimp importer can not load skeleton file from path (import file path : ");
				ErrorData = ErrorData + InImportFullPathName + ", save assset path : " + TempFullPathName + ").";
				PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorData));
			}
#endif
			return FALSE;
		}
		if (!(SaveSkeletonResource(InSavePath, InAssetName, (&(AssimpSkeleton)))))
		{
			return FALSE;
		}
		return TRUE;
	}
#endif
	BOOL32 ESkeletonAssetManager::LoadSkeletonAsset(const EString& InLoadPath, const EString& InLoadName, const ESkeletonAsset*& OutSkeletonAsset)
	{
		EString TempFullPathName(InLoadPath);
		TempFullPathName = TempFullPathName + InLoadName + ESettings::ENGINE_ASSET_NAME_TYPE;
		ESkeletonAsset* ResultSkeletonAsset = SkeletonAssetDataManager.Find(TempFullPathName);
		if (ResultSkeletonAsset)
		{
			OutSkeletonAsset = ResultSkeletonAsset;
			return TRUE;
		}
		ResultSkeletonAsset = LoadSkeletonAsset(InLoadPath, InLoadName);
		if (!ResultSkeletonAsset)
		{
			return FALSE;
		}
		if (!ResultSkeletonAsset->InitResource())
		{
			delete ResultSkeletonAsset;
			return FALSE;
		}
		if (SkeletonAssetDataManager.Add(TempFullPathName, ResultSkeletonAsset, TRUE) == 0u)
		{
#if _EDITOR_ONLY
			{
				EString ErrorInfo = EString("Skeleton asset path = [") + TempFullPathName + "] add into manager list failed.";
				PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorInfo));
			}
#endif
			delete ResultSkeletonAsset;
			return FALSE;
		}
		OutSkeletonAsset = ResultSkeletonAsset;
		return TRUE;
	}
	void ESkeletonAssetManager::ClearSkeletons()
	{
		SkeletonAssetDataManager.Clear();
	}
	ESkeletonAsset* ESkeletonAssetManager::LoadSkeletonAsset(const EString& InLoadPath, const EString& InLoadName)
	{
		ESkeletonAsset* NewSkeletonAsset = nullptr;
		ESkeleton* LoadedSkeletonResource = LoadSkeletonResource(InLoadPath, InLoadName);
		if ((!LoadedSkeletonResource) || (!(LoadedSkeletonResource->IsResourceValid())))
		{
			return NewSkeletonAsset;
		}

		NewSkeletonAsset = new ESkeletonAsset(InLoadPath, InLoadName
#if _EDITOR_ONLY
			, InLoadName
#endif
		);
		if (!(NewSkeletonAsset->StorageResourceInternal(
			[LoadedSkeletonResource]()->ESkeleton*
			{
				return LoadedSkeletonResource;
			})))
		{
			//TODO
			delete NewSkeletonAsset;
			NewSkeletonAsset = nullptr;
			return NewSkeletonAsset;
		}
		return NewSkeletonAsset;
	}
	BOOL32 ESkeletonAssetManager::SaveSkeletonAsset(const EString& InSavePath, const EString& InSaveName, const ESkeletonAsset* InSkeletonAsset)
	{
		EString TempFullPathName(InSavePath);
		TempFullPathName = TempFullPathName + InSaveName + ESettings::ENGINE_ASSET_NAME_TYPE;
		if (TempFullPathName.Length() < 10u)
		{
#if _EDITOR_ONLY
			{
				EString ErrorData("Save skeleton asset path name check failed (save file path : ");
				ErrorData += InSavePath;
				ErrorData += ", save file name : ";
				ErrorData += InSaveName;
				ErrorData += ").";
				PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorData));
			}
#endif
			return FALSE;
		}
		if (!InSkeletonAsset)
		{
#if _EDITOR_ONLY
			{
				EString ErrorData("Save skeleton asset failed (save file path : ");
				ErrorData += TempFullPathName;
				ErrorData += ") this asset is null.";
				PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorData));
			}
#endif
			return FALSE;
		}
		const ESkeleton* SavedSkeletonResource = InSkeletonAsset->GetStoragedResource();
		if ((!SavedSkeletonResource) || (!(SavedSkeletonResource->IsResourceValid())))
		{
#if _EDITOR_ONLY
			{
				EString ErrorData("Save skeleton asset failed (save file path : ");
				ErrorData += TempFullPathName;
				ErrorData += ", skeleton asset name : ";
				ErrorData += (POBJ_DEBUGNAME_GET(InSkeletonAsset));
				ErrorData += ") this asset is not contain valid skeleton resource.";
				PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorData));
			}
#endif
			return FALSE;
		}
		return (SaveSkeletonResource(InSavePath, InSaveName, SavedSkeletonResource));
	}
	ESkeleton* ESkeletonAssetManager::LoadSkeletonResource(const EString& InLoadPath, const EString& InLoadName)
	{
		EString TempFullPathName(InLoadPath);
		TempFullPathName = TempFullPathName + InLoadName + ESettings::ENGINE_ASSET_NAME_TYPE;
		if (TempFullPathName.Length() < 10u)
		{
#if _EDITOR_ONLY
			{
				EString ErrorData("Load skeleton asset path name check failed (load file path : ");
				ErrorData += InLoadPath;
				ErrorData += ", load file name : ";
				ErrorData += InLoadName;
				ErrorData += ").";
				PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorData));
			}
#endif
			return nullptr;
		}
		ESkeleton* LoadedSkeletonResource = nullptr;
		void* ReadFileMem = nullptr; ULONG ReadFileSize = 0u;
		if (!EFileHelper::ReadFileAsBinary(TempFullPathName, ReadFileMem, ReadFileSize))
		{
			if (ReadFileMem)
			{
				delete[]ReadFileMem;
			}
#if _EDITOR_ONLY
			{
				EString ErrorData("Load skeleton asset failed (load file path : ");
				ErrorData += TempFullPathName;
				ErrorData += ").";
				PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorData));
			}
#endif
			return LoadedSkeletonResource;
		}

		const void* RstMemPtr = ReadFileMem;
		LONGLONG RstMemSize = static_cast<LONGLONG>(ReadFileSize);

#define LOAD_ASSET_MEMORY(__MemType, __MemSize, __OutputValue) \
		__MemType __OutputValue;\
		{\
			PE_CHECK((ENGINE_ASSET_ERROR), ("Check load skeleton asset [rest memory size] failed."), (RstMemSize >= (__MemSize)));\
			const __MemType* TempPtr = (const __MemType*)RstMemPtr;\
			(__OutputValue) = TempPtr[0];\
			RstMemPtr = &(TempPtr[1]);\
			RstMemSize -= (__MemSize);\
		}\

#define LOAD_ASSET_STRING_MEMORY(__LengthMax, __OutputEString) \
		EString __OutputEString;\
		{\
			const UINT32 StrStride = sizeof(CHAR) * (__LengthMax);\
			PE_CHECK((ENGINE_ASSET_ERROR), ("Check load skeleton asset [rest memory size] failed."), (RstMemSize >= StrStride));\
			const CHAR* TempPtr = (const CHAR*)RstMemPtr;\
			CHAR StrValues[(__LengthMax)];\
			::memcpy_s(StrValues, StrStride, TempPtr, StrStride);\
			(__OutputEString) = StrValues;\
			RstMemPtr = &(TempPtr[(__LengthMax)]);\
			RstMemSize -= StrStride;\
		}\

#define LOAD_ASSET_PTR_MEMORY(__ElementStride, __ElementNum, __PtrType, __Ptr) \
		__PtrType* __Ptr = nullptr;\
		{\
			const UINT32 MemSize = (__ElementStride) * (__ElementNum);\
			PE_CHECK((ENGINE_ASSET_ERROR), ("Check load skeleton asset [rest memory size] failed."), (RstMemSize >= MemSize));\
			void* NewPtr = new BYTE[MemSize];\
			const BYTE* TempPtr = (const BYTE*)RstMemPtr;\
			::memcpy_s(NewPtr, MemSize, TempPtr, MemSize);\
			(__Ptr) = (__PtrType*)NewPtr;\
			RstMemPtr = &(TempPtr[MemSize]);\
			RstMemSize -= MemSize;\
		}\

		{
			{
				LOAD_ASSET_MEMORY(UINT32, sizeof(UINT32), TempAssetType);
				EAssetType AssetType = static_cast<EAssetType>(TempAssetType);
				if (AssetType != EAssetType::ASSET_TYPE_SKELETON)
				{
#if _EDITOR_ONLY
					{
						EString ErrorData("Load skeleton asset failed (load file path : ");
						ErrorData += TempFullPathName;
						ErrorData += ") this asset is not skeleton type.";
						PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorData));
					}
#endif
					delete[]ReadFileMem;
					return LoadedSkeletonResource;
				}
			}
			{
				LOAD_ASSET_MEMORY(UINT32, sizeof(UINT32), TempSkeletonType);
				ESkeletonType SkeletonType = static_cast<ESkeletonType>(TempSkeletonType);
				if ((TempSkeletonType == ESkeletonType::SKELETON_TYPE_UNKNOWN) || (TempSkeletonType >= ESkeletonType::SKELETON_TYPE_COUNT))
				{
#if _EDITOR_ONLY
					{
						EString ErrorData("Load skeleton asset failed (load file path : ");
						ErrorData += TempFullPathName;
						ErrorData += ") this asset is not skeleton type.";
						PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorData));
					}
#endif
					delete[]ReadFileMem;
					return LoadedSkeletonResource;
				}
			}
			{
				LOAD_ASSET_STRING_MEMORY(ESettings::ENGINE_SKELETON_NAME_LENGTH_MAX, TempSkeletonName);
				LoadedSkeletonResource = new ESkeleton(TempSkeletonName);
			}
			{
				LOAD_ASSET_MEMORY(USHORT, sizeof(USHORT), TempBoneNum);

				ESkeleton::EBonePart&	BonePart	= LoadedSkeletonResource->Bones;
				TMap<EString, USHORT>&	BoneMapping	= LoadedSkeletonResource->BoneMapping;

				for (SHORT i = 0u, n = static_cast<SHORT>(TempBoneNum); i < n; i++)
				{
					LOAD_ASSET_MEMORY(SHORT, sizeof(SHORT), TempIndex);
					LOAD_ASSET_STRING_MEMORY(ESettings::ENGINE_BONE_NAME_LENGTH_MAX, TempBoneName);
					LOAD_ASSET_MEMORY(Vector3, sizeof(Vector3), TempDefaultPosition);
					LOAD_ASSET_MEMORY(Quaternion, sizeof(Quaternion), TempDefaultRotation);
					LOAD_ASSET_MEMORY(Vector3, sizeof(Vector3), TempDefaultScaling);
					LOAD_ASSET_MEMORY(SHORT, sizeof(SHORT), TempParentIndex);
					LOAD_ASSET_MEMORY(USHORT, sizeof(USHORT), TempChildrenNum);

					EBoneData& BoneData = BonePart.Add_Default_GetRef();

					for (USHORT ChildIndex = 0u; ChildIndex < TempChildrenNum; ChildIndex++)
					{
						LOAD_ASSET_MEMORY(SHORT, sizeof(SHORT), TempChildIndex);

						BoneData.Children.Add(TempChildIndex);
					}

					PE_CHECK((ENGINE_ASSET_ERROR), ("Check read skeleton resource's bone index failed"), (TempIndex == i));

					BoneData.Index				= TempIndex;
					BoneData.Name				= TempBoneName;
					BoneData.DefaultPosition	= TempDefaultPosition;
					BoneData.DefaultRotation	= TempDefaultRotation;
					BoneData.DefaultScaling		= TempDefaultScaling;
					BoneData.Parent				= TempParentIndex;
				}
				for (USHORT i = 0u; i < TempBoneNum; i++)
				{
					LOAD_ASSET_STRING_MEMORY(ESettings::ENGINE_BONE_NAME_LENGTH_MAX, TempBoneName);
					LOAD_ASSET_MEMORY(USHORT, sizeof(USHORT), TempBoneIndex);

					BoneMapping.Add(TempBoneName, TempBoneIndex);
				}
			}

			PE_CHECK((ENGINE_ASSET_ERROR), ("Check read skeleton resource rest memory already failed."), (RstMemSize == 0));
		}

#undef LOAD_ASSET_MEMORY
#undef LOAD_ASSET_STRING_MEMORY
#undef LOAD_ASSET_PTR_MEMORY

		delete[]ReadFileMem;
		if (!(LoadedSkeletonResource->IsResourceValid()))
		{
			delete LoadedSkeletonResource;
			LoadedSkeletonResource = nullptr;
		}
		return LoadedSkeletonResource;
	}
	BOOL32 ESkeletonAssetManager::SaveSkeletonResource(const EString& InSavePath, const EString& InSaveName, const ESkeleton* InSkeletonResource)
	{
		if ((!InSkeletonResource) || (!(InSkeletonResource->IsResourceValid())))
		{
			PE_FAILED((ENGINE_ASSET_ERROR), ("Check skeleton resource error, skeleton resource is null."));
			return FALSE;
		}
		EString TempFullPathName(InSavePath);
		TempFullPathName = TempFullPathName + InSaveName + ESettings::ENGINE_ASSET_NAME_TYPE;
		if (TempFullPathName.Length() < 10u)
		{
#if _EDITOR_ONLY
			{
				EString ErrorData("Save skeleton asset path name check failed (save file path : ");
				ErrorData += InSavePath;
				ErrorData += ", save file name : ";
				ErrorData += InSaveName;
				ErrorData += ").";
				PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorData));
			}
#endif
			return FALSE;
		}

		auto CalculateSkeletonBytes = [](const ESkeleton* InSkeleton)->ULONG
		{
			ULONG Result = 0u;

			Result += sizeof(UINT32);		// EAssetType
			Result += sizeof(UINT32);		// ESkeletonType
			Result += sizeof(CHAR) * ESettings::ENGINE_SKELETON_NAME_LENGTH_MAX;	// Skeleton name
			Result += sizeof(USHORT);		// Bone num and bone mapping

			const ESkeleton::EBonePart& BonePart = InSkeleton->GetBonePart();
			for (UINT32 i = 0u, n = BonePart.Length(); i < n; i++)
			{
				const EBoneData& BoneData = BonePart[i];

				Result += sizeof(SHORT);		// Bone data index
				Result += sizeof(CHAR) * ESettings::ENGINE_BONE_NAME_LENGTH_MAX;	// Bone data name
				Result += sizeof(Vector3);		// Bone data default position
				Result += sizeof(Quaternion);	// Bone data default rotation
				Result += sizeof(Vector3);		// Bone data default scaling
				Result += sizeof(SHORT);		// Bone data parent index
				Result += sizeof(USHORT);		// Bone data children num
				Result += sizeof(SHORT) * BoneData.Children.Length();	// Bone data children index
			}
			Result += (sizeof(CHAR) * ESettings::ENGINE_BONE_NAME_LENGTH_MAX + sizeof(USHORT)) * BonePart.Length();	// Bone mapping datas

			return Result;
		};

		const ULONG OutputMemSize = CalculateSkeletonBytes(InSkeletonResource);
		BYTE* OutputMem = new BYTE[OutputMemSize];

		void* RstMemPtr = OutputMem;
		LONGLONG RstMemSize = static_cast<LONGLONG>(OutputMemSize);

#define SAVE_ASSET_MEMORY(__Type, __Value) \
		{\
			PE_CHECK((ENGINE_ASSET_ERROR), ("Check save skeleton asset [rest memory size] failed."), (RstMemSize >= (sizeof(__Type))));\
			__Type* TempPtr = (__Type*)RstMemPtr;\
			TempPtr[0] = (__Value);\
			RstMemPtr = &(TempPtr[1]);\
			RstMemSize -= (sizeof(__Type));\
		}\

#define SAVE_ASSET_STRING_MEMORY(__EString, __LengthMax) \
		{\
			const UINT32 NameLengthMax = sizeof(CHAR) * (__LengthMax);\
			PE_CHECK((ENGINE_ASSET_ERROR), ("Check save skeleton asset [rest memory size] failed."), (RstMemSize >= NameLengthMax));\
			const EString& SavedName = (__EString);\
			const UINT32 NameLength = EMath::Clamp(static_cast<UINT32>(sizeof(CHAR) * (SavedName.Length() + 1u)), 0u, NameLengthMax);\
			CHAR* TempPtr = (CHAR*)RstMemPtr;\
			::memcpy_s(TempPtr, NameLengthMax, (*SavedName), NameLength);\
			RstMemPtr = &(TempPtr[(__LengthMax)]);\
			RstMemSize -= NameLengthMax;\
		}\

#define SAVE_ASSET_PTR_MEMORY(__ElementStride, __ElementNum, __Ptr) \
		{\
			const UINT32 MemSize = (__ElementStride) * (__ElementNum);\
			PE_CHECK((ENGINE_ASSET_ERROR), ("Check save skeleton asset [rest memory size] failed."), (RstMemSize >= MemSize));\
			BYTE* TempPtr = (BYTE*)RstMemPtr;\
			::memcpy_s(TempPtr, MemSize, (__Ptr), MemSize);\
			RstMemPtr = &(TempPtr[MemSize]);\
			RstMemSize -= MemSize;\
		}\

		{
			SAVE_ASSET_MEMORY(UINT32, static_cast<UINT32>(EAssetType::ASSET_TYPE_SKELETON));
			SAVE_ASSET_MEMORY(UINT32, static_cast<UINT32>(InSkeletonResource->GetSkeletonType()));
			SAVE_ASSET_STRING_MEMORY(InSkeletonResource->GetSkeletonName(), ESettings::ENGINE_SKELETON_NAME_LENGTH_MAX);

			const ESkeleton::EBonePart&		BonePart	= InSkeletonResource->GetBonePart();
			const TMap<EString, USHORT>&	BoneMapping = InSkeletonResource->GetBoneMapping();
			const USHORT BoneNum = static_cast<USHORT>(BonePart.Length());	// Skeleton bone num can not greater than 65535u.
			PE_CHECK((ENGINE_ASSET_ERROR), ("Check save skeleton asset bone num failed, bone mapping num is not match bone num."), ((BoneNum > 0u) && (BoneNum == static_cast<USHORT>(BoneMapping.Length()))));

			SAVE_ASSET_MEMORY(USHORT, BoneNum);

			for (USHORT i = 0u; i < BoneNum; i++)
			{
				const EBoneData& BondData = BonePart[i];

				SAVE_ASSET_MEMORY(SHORT, BondData.Index);
				SAVE_ASSET_STRING_MEMORY(BondData.Name, ESettings::ENGINE_BONE_NAME_LENGTH_MAX);
				SAVE_ASSET_MEMORY(Vector3, BondData.DefaultPosition);
				SAVE_ASSET_MEMORY(Quaternion, BondData.DefaultRotation);
				SAVE_ASSET_MEMORY(Vector3, BondData.DefaultScaling);
				SAVE_ASSET_MEMORY(SHORT, BondData.Parent);

				const TArray<SHORT>& ChildrenDatas = BondData.Children;
				const USHORT ChildrenNum = static_cast<USHORT>(ChildrenDatas.Length());

				SAVE_ASSET_MEMORY(USHORT, ChildrenNum);

				for (USHORT ChildIndex = 0u; ChildIndex < ChildrenNum; ChildIndex++)
				{
					SAVE_ASSET_MEMORY(SHORT, ChildrenDatas[ChildIndex]);
				}
			}
			for (auto ItMapping = BoneMapping.Begin(); ItMapping != BoneMapping.End(); ItMapping++)
			{
				SAVE_ASSET_STRING_MEMORY(ItMapping->first, ESettings::ENGINE_BONE_NAME_LENGTH_MAX);
				SAVE_ASSET_MEMORY(USHORT, ItMapping->second);
			}

			PE_CHECK((ENGINE_ASSET_ERROR), ("Check write skeleton resource rest memory already failed."), (RstMemSize == 0));
		}

#undef SAVE_ASSET_MEMORY
#undef SAVE_ASSET_STRING_MEMORY
#undef SAVE_ASSET_PTR_MEMORY

		if (EFileHelper::SaveBytesToFile(TempFullPathName, OutputMem, OutputMemSize))
		{
			delete[]OutputMem;
			return TRUE;
		}
		delete[]OutputMem;
		return FALSE;
	}

	void TryLoadSkeleton(const EString& InLoadPath, const EString& InLoadName, const ESkeletonAsset*& OutAsset, const EString* InImportPath, const EString* InImportName, const EString* InImportFileType)
	{
		ESkeletonAssetManager* SkeletonAssetManager = ESkeletonAssetManager::GetManagerSingleton();
		PE_CHECK((ENGINE_ASSET_ERROR), ("Check output skeleton asset pointer is initialized failed."), (!OutAsset));
		if (SkeletonAssetManager->LoadSkeletonAsset(InLoadPath, InLoadName, OutAsset))
		{
			return;
		}
		PE_FAILED((ENGINE_ASSET_ERROR), ("Load skeleton asset failed."));
#if _EDITOR_ONLY
		if ((!!InImportPath) && (!!InImportName) && (!!InImportFileType))
		{
			EString TempImportFullPath(*InImportPath);
			TempImportFullPath = TempImportFullPath + (*InImportName) + (".") + (*InImportFileType);
			BOOL32 Result = SkeletonAssetManager->ImportSkeleton(
				InLoadName,
				TempImportFullPath,
				InLoadPath);
			if (Result)
			{
				if (!(SkeletonAssetManager->LoadSkeletonAsset(InLoadPath, InLoadName, OutAsset)))
				{
					PE_FAILED((ENGINE_ASSET_ERROR), ("Try load skeleton asset failed."));
				}
			}
			else
			{
				PE_FAILED((ENGINE_ASSET_ERROR), ("Try import skeleton asset failed."));
			}
		}
#endif
	}

};