#include "SkeletonAsset.h"
#include <RenderDevice/DeviceD3D11.h>
#include <Config/EngineConfig.h>
#if _EDITOR_ONLY
#include "../../../EngineThirdParty/assimp/Headers/assimpManager.h"
#endif

namespace PigeonEngine
{

	template <typename _TAssetResourceType, typename _TAssetRenderResourceType, typename _AssetType>
	static void RegisterSkeletonClassTypes()
	{
		RegisterRenderBaseAssetClassTypes<_TAssetResourceType, _TAssetRenderResourceType>();
		RegisterClassType<_AssetType, TRenderBaseAsset<_TAssetResourceType, _TAssetRenderResourceType>>();
	}

	static void RegisterClassTypes()
	{
		RegisterClassType<ESkeleton, EObjectBase, EResourceInterface>();
		RegisterClassType<ESkeletonRenderResource, EObjectBase, RRenderResourceInterface>();
		RegisterClassType<ESkeletonAssetManager, EManagerBase>();

		RegisterSkeletonClassTypes<ESkeleton, ESkeletonRenderResource, ESkeletonAsset>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

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
		SkeletonName = ENGINE_DEFAULT_NAME;
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
	const ESkeleton::EBonePart& ESkeleton::GetBonePart()const
	{
		return Bones;
	}
	const TMap<EString, USHORT>& ESkeleton::GetBoneMapping()const
	{
		return BoneMapping;
	}
	UINT32 ESkeleton::GetBoneCount()const
	{
		return (Bones.Length());
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
			PE_FAILED((ENGINE_ASSET_ERROR), ErrorInfo);
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
			PE_FAILED((ENGINE_ASSET_ERROR), ErrorInfo);
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

	ESkeletonRenderResource::ESkeletonRenderResource(ESkeleton* InSkeleton)
		: Skeleton(InSkeleton)
	{
	}
	ESkeletonRenderResource::~ESkeletonRenderResource()
	{
		ReleaseRenderResource();
	}
	BOOL32 ESkeletonRenderResource::IsRenderResourceValid()const
	{
		if ((!!Skeleton) && Skeleton->IsResourceValid())
		{
			BOOL32 Result = TRUE;
			for (UINT32 i = 0u, n = ESkeletonRenderResourceType::SKELETON_RENDER_RESOURCE_COUNT; i < n; i++)
			{
				Result = Result && (RenderResource[i].IsRenderResourceValid());
			}
			return Result;
		}
		return FALSE;
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
		if ((!!Skeleton) && Skeleton->IsResourceValid())
		{
			RDeviceD3D11* RenderDevice = RDeviceD3D11::GetDeviceSingleton();
			const UINT32 SkeletonBoneNum = Skeleton->GetBoneCount();
			for (UINT32 i = 0u, n = ESkeletonRenderResourceType::SKELETON_RENDER_RESOURCE_COUNT; i < n; i++)
			{
				DirectX::XMFLOAT4X4* SkeletonMatrices = new DirectX::XMFLOAT4X4[RCommonSettings::RENDER_SKELETON_BONE_NUM_MAX];
				{
					DirectX::XMFLOAT4X4 TempIdentity;
					DirectX::XMStoreFloat4x4((&TempIdentity), DirectX::XMMatrixIdentity());
					for (UINT32 i = 0u; (i < SkeletonBoneNum) && (i < RCommonSettings::RENDER_SKELETON_BONE_NUM_MAX); i++)
					{
						SkeletonMatrices[i] = TempIdentity;
					}
				}
				RSubresourceDataDesc SubresourceDataDesc;
				SubresourceDataDesc.pSysMem = SkeletonMatrices;
				if (!(RenderDevice->CreateStructuredBuffer(
					(RenderResource[i]),
					RStructuredBufferDesc(sizeof(DirectX::XMFLOAT4X4), SkeletonBoneNum, TRUE),
					(&SubresourceDataDesc))))
				{
#if _EDITOR_ONLY
					EString ErrorInfo("Create skeleton resource [name = ");
					ErrorInfo += Skeleton->GetDebugName();
					ErrorInfo += "] bones buffer [num = ";
					ErrorInfo += ToString(SkeletonBoneNum);
					ErrorInfo += "] failed.";
					PE_FAILED((ENGINE_ASSET_ERROR), (ErrorInfo));
#endif
					delete[]SkeletonMatrices;
					return FALSE;
				}
				delete[]SkeletonMatrices;
			}
			return TRUE;
		}
		return FALSE;
	}
	void ESkeletonRenderResource::ReleaseRenderResource()
	{
		Skeleton = nullptr;
		for (UINT32 i = 0u, n = ESkeletonRenderResourceType::SKELETON_RENDER_RESOURCE_COUNT; i < n; i++)
		{
			RenderResource[i].ReleaseRenderResource();
		}
	}

	ESkeletonAsset::ESkeletonAsset(const EString& InAssetPath, const EString& InAssetName
#if _EDITOR_ONLY
		, const EString& InDebugName
#endif
	) : TRenderBaseAsset<ESkeleton, ESkeletonRenderResource>(InAssetPath, InAssetName
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
		if (IsInitialized())
		{
#if _EDITOR_ONLY
			{
				EString ErrorInfo = EString("Skeleton name=[") + GetAssetName() + "] path = [" + GetAssetPath() + "] has been Initialized.";
				PE_FAILED((ENGINE_ASSET_ERROR), (ErrorInfo));
			}
#endif
			return TRUE;
		}
		if (CreateRenderResourceInternal(
			[](ESkeleton* InResource)->ESkeletonRenderResource*
			{
				ESkeletonRenderResource* RenderResource = nullptr;
				if ((!!InResource) && (InResource->IsResourceValid()))
				{
					RenderResource = new ESkeletonRenderResource(InResource);
					RenderResource->InitRenderResource();
				}
				return RenderResource;
			},
			TRUE))
		{
			bIsInitialized = TRUE;
			return TRUE;
		}
		return FALSE;
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
		TempFullPathName = TempFullPathName + InAssetName + ENGINE_ASSET_NAME_TYPE;
		if ((InImportFullPathName.Length() < 3u) || (TempFullPathName.Length() < 10u))
		{
#if _EDITOR_ONLY
			{
				EString ErrorData("Error file path for skeleton importer (import file path : ");
				ErrorData = ErrorData + InImportFullPathName + ", save assset path : " + TempFullPathName + ").";
				PE_FAILED((ENGINE_ASSET_ERROR), (ErrorData));
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
				PE_FAILED((ENGINE_ASSET_ERROR), (ErrorData));
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
				PE_FAILED((ENGINE_ASSET_ERROR), (ErrorData));
			}
#endif
			return FALSE;
		}
		EString UsedSkeletonName;
		if (InAssetName.Length() < 2u)
		{
			UsedSkeletonName = ENGINE_DEFAULT_NAME;
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
				PE_FAILED((ENGINE_ASSET_ERROR), (ErrorData));
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
		TempFullPathName = TempFullPathName + InLoadName + ENGINE_ASSET_NAME_TYPE;
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
				PE_FAILED((ENGINE_ASSET_ERROR), (ErrorInfo));
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
		TempFullPathName = TempFullPathName + InSaveName + ENGINE_ASSET_NAME_TYPE;
		if (TempFullPathName.Length() < 10u)
		{
#if _EDITOR_ONLY
			{
				EString ErrorData("Save skeleton asset path name check failed (save file path : ");
				ErrorData += InSavePath;
				ErrorData += ", save file name : ";
				ErrorData += InSaveName;
				ErrorData += ").";
				PE_FAILED((ENGINE_ASSET_ERROR), (ErrorData));
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
				PE_FAILED((ENGINE_ASSET_ERROR), (ErrorData));
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
				ErrorData += InSkeletonAsset->GetDebugName();
				ErrorData += ") this asset is not contain valid skeleton resource.";
				PE_FAILED((ENGINE_ASSET_ERROR), (ErrorData));
			}
#endif
			return FALSE;
		}
		return (SaveSkeletonResource(InSavePath, InSaveName, SavedSkeletonResource));
	}
	ESkeleton* ESkeletonAssetManager::LoadSkeletonResource(const EString& InLoadPath, const EString& InLoadName)
	{
		EString TempFullPathName(InLoadPath);
		TempFullPathName = TempFullPathName + InLoadName + ENGINE_ASSET_NAME_TYPE;
		if (TempFullPathName.Length() < 10u)
		{
#if _EDITOR_ONLY
			{
				EString ErrorData("Load skeleton asset path name check failed (load file path : ");
				ErrorData += InLoadPath;
				ErrorData += ", load file name : ";
				ErrorData += InLoadName;
				ErrorData += ").";
				PE_FAILED((ENGINE_ASSET_ERROR), (ErrorData));
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
				PE_FAILED((ENGINE_ASSET_ERROR), (ErrorData));
			}
#endif
			return LoadedSkeletonResource;
		}

		const void* RstMemPtr = ReadFileMem;
		LONGLONG RstMemSize = static_cast<LONGLONG>(ReadFileSize);

#define LOAD_ASSET_MEMORY(__MemType, __MemSize, __OutputValue) \
		__MemType __OutputValue;\
		{\
			Check((ENGINE_ASSET_ERROR), ("Check load skeleton asset [rest memory size] failed."), (RstMemSize >= (__MemSize)));\
			const __MemType* TempPtr = (const __MemType*)RstMemPtr;\
			(__OutputValue) = TempPtr[0];\
			RstMemPtr = &(TempPtr[1]);\
			RstMemSize -= (__MemSize);\
		}\

#define LOAD_ASSET_STRING_MEMORY(__LengthMax, __OutputEString) \
		EString __OutputEString;\
		{\
			const UINT32 StrStride = sizeof(CHAR) * (__LengthMax);\
			Check((ENGINE_ASSET_ERROR), ("Check load skeleton asset [rest memory size] failed."), (RstMemSize >= StrStride));\
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
			Check((ENGINE_ASSET_ERROR), ("Check load skeleton asset [rest memory size] failed."), (RstMemSize >= MemSize));\
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
						PE_FAILED((ENGINE_ASSET_ERROR), (ErrorData));
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
						PE_FAILED((ENGINE_ASSET_ERROR), (ErrorData));
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

					Check((ENGINE_ASSET_ERROR), ("Check read skeleton resource's bone index failed"), (TempIndex == i));

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

			Check((ENGINE_ASSET_ERROR), ("Check read skeleton resource rest memory already failed."), (RstMemSize == 0));
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
		TempFullPathName = TempFullPathName + InSaveName + ENGINE_ASSET_NAME_TYPE;
		if (TempFullPathName.Length() < 10u)
		{
#if _EDITOR_ONLY
			{
				EString ErrorData("Save skeleton asset path name check failed (save file path : ");
				ErrorData += InSavePath;
				ErrorData += ", save file name : ";
				ErrorData += InSaveName;
				ErrorData += ").";
				PE_FAILED((ENGINE_ASSET_ERROR), (ErrorData));
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
			Check((ENGINE_ASSET_ERROR), ("Check save skeleton asset [rest memory size] failed."), (RstMemSize >= (sizeof(__Type))));\
			__Type* TempPtr = (__Type*)RstMemPtr;\
			TempPtr[0] = (__Value);\
			RstMemPtr = &(TempPtr[1]);\
			RstMemSize -= (sizeof(__Type));\
		}\

#define SAVE_ASSET_STRING_MEMORY(__EString, __LengthMax) \
		{\
			const UINT32 NameLengthMax = sizeof(CHAR) * (__LengthMax);\
			Check((ENGINE_ASSET_ERROR), ("Check save skeleton asset [rest memory size] failed."), (RstMemSize >= NameLengthMax));\
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
			Check((ENGINE_ASSET_ERROR), ("Check save skeleton asset [rest memory size] failed."), (RstMemSize >= MemSize));\
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
			Check((ENGINE_ASSET_ERROR), ("Check save skeleton asset bone num failed, bone mapping num is not match bone num."), ((BoneNum > 0u) && (BoneNum == static_cast<USHORT>(BoneMapping.Length()))));

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

			Check((ENGINE_ASSET_ERROR), ("Check write skeleton resource rest memory already failed."), (RstMemSize == 0));
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

};