#include "AnimationAsset.h"

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<ESkeletonAnimation, EObjectBase, EResourceInterface>();

		RegisterBaseAssetClassTypes<ESkeletonAnimation>();

		RegisterClassType<ESkeletonAnimationAsset, TBaseAsset<ESkeletonAnimation>>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	ESkeletonAnimation::ESkeletonAnimation(const EString& InMeshName)
		: AnimationName(InMeshName)
	{
	}
	ESkeletonAnimation::~ESkeletonAnimation()
	{
		ReleaseResource();
	}
	BOOL ESkeletonAnimation::IsResourceValid()const
	{
		BOOL Result = FALSE;
		if (const UINT AnimationClipNum = AnimationClips.Length(); AnimationClipNum > 0u)
		{
			for (UINT i = 0u; i < AnimationClipNum; i++)
			{
				Result = Result || (AnimationClips[i].IsValid());
			}
		}
		return Result;
	}
	BOOL ESkeletonAnimation::InitResource()
	{
		// Skeleton animation resource must init by animation manager.
		return TRUE;
	}
	void ESkeletonAnimation::ReleaseResource()
	{
		// Skeleton animation resource do not need release.
	}
	const EString& ESkeletonAnimation::GetAnimationName()const
	{
		return AnimationName;
	}
	UINT ESkeletonAnimation::GetAnimationClipNum()const
	{
		return (AnimationClips.Length());
	}
	const ESkeletonAnimationClip* ESkeletonAnimation::GetAnimationClip(const EString& InClipName)const
	{
		for (UINT i = 0u, n = AnimationClips.Length(); i < n; i++)
		{
			if (AnimationClips[i].ClipName == InClipName)
			{
				return (&(AnimationClips[i]));
			}
		}
		return nullptr;
	}
	const ESkeletonAnimationClip* ESkeletonAnimation::GetAnimationClip(UINT InIndex)const
	{
		if (InIndex < AnimationClips.Length())
		{
			return (&(AnimationClips[InIndex]));
		}
		return nullptr;
	}
	const TArray<ESkeletonAnimationClip>& ESkeletonAnimation::GetAnimationClips()const
	{
		return AnimationClips;
	}

	ESkeletonAnimationAsset::ESkeletonAnimationAsset(const EString& InAssetPath, const EString& InAssetName
#if _EDITOR_ONLY
		, const EString& InDebugName
#endif
	) : TBaseAsset<ESkeletonAnimation>(InAssetPath, InAssetName
#if _EDITOR_ONLY
		, InDebugName
#endif
	)
	{
	}
	ESkeletonAnimationAsset::~ESkeletonAnimationAsset()
	{
	}
	BOOL ESkeletonAnimationAsset::InitResource()
	{
		// Skeleton animation resource do not need to initialize.
		return TRUE;
	}

	EAnimationManager::EAnimationManager()
	{
#if _EDITOR_ONLY
		DebugName = ENGINE_ANIMATION_ASSET_MANAGER_NAME;
#endif
	}
	EAnimationManager::~EAnimationManager()
	{
	}
	void EAnimationManager::Initialize()
	{

	}
	void EAnimationManager::ShutDown()
	{
		ClearSkeletonAnimations();
	}
#if _EDITOR_ONLY
	BOOL EAnimationManager::ImportSkeletonAnimation(const EString& InAssetName, const EString& InImportFullPathName, const EString& InSavePath)
	{
		return FALSE;
	}
#endif
	BOOL EAnimationManager::LoadSkeletonAnimationAsset(const EString& InLoadPath, const EString& InLoadName, const ESkeletonAnimationAsset*& OutSkeletonAnimationAsset)
	{
		return FALSE;
	}
	void EAnimationManager::ClearSkeletonAnimations()
	{
		SkeletonAnimationAssetManager.Clear();
	}
	ESkeletonAnimationAsset* EAnimationManager::LoadSkeletonAnimationAsset(const EString& InLoadPath, const EString& InLoadName)
	{
		return nullptr;
	}
	BOOL EAnimationManager::SaveSkeletonAnimationAsset(const EString& InSavePath, const EString& InSaveName, const ESkeletonAnimationAsset* InSkeletonAnimationAsset)
	{
		return FALSE;
	}
	ESkeletonAnimation* EAnimationManager::LoadSkeletonAnimationResource(const EString& InLoadPath, const EString& InLoadName)
	{
		/*EString TempFullPathName(InLoadPath);
		TempFullPathName += InLoadName;
		TempFullPathName += ENGINE_ASSET_NAME_TYPE;
		if (TempFullPathName.Length() < 10u)
		{
#if _EDITOR_ONLY
			{
				EString ErrorData("Load skeleton animation asset path name check failed (load file path : ");
				ErrorData += InLoadPath;
				ErrorData += ", load file name : ";
				ErrorData += InLoadName;
				ErrorData += ").";
				PE_FAILED((ENGINE_ASSET_ERROR), (ErrorData));
			}
#endif
			return nullptr;
		}
		ESkeletonAnimation* LoadedSkeletonAnimationResource = nullptr;
		void* ReadFileMem = nullptr; ULONG ReadFileSize = 0u;
		if (!EFileHelper::ReadFileAsBinary(TempFullPathName, ReadFileMem, ReadFileSize))
		{
			if (ReadFileMem)
			{
				delete[]ReadFileMem;
			}
#if _EDITOR_ONLY
			{
				EString ErrorData("Load skeleton animation asset failed (load file path : ");
				ErrorData += TempFullPathName;
				ErrorData += ").";
				PE_FAILED((ENGINE_ASSET_ERROR), (ErrorData));
			}
#endif
			return LoadedSkeletonAnimationResource;
		}

		const void* RstMemPtr = ReadFileMem;
		LONGLONG RstMemSize = static_cast<LONGLONG>(ReadFileSize);

#define LOAD_ASSET_MEMORY(__MemType, __MemSize, __OutputValue) \
		__MemType __OutputValue;\
		{\
			Check((ENGINE_ASSET_ERROR), ("Check load skeleton animation asset [rest memory size] failed."), (RstMemSize >= (__MemSize)));\
			const __MemType* TempPtr = (const __MemType*)RstMemPtr;\
			(__OutputValue) = TempPtr[0];\
			RstMemPtr = &(TempPtr[1]);\
			RstMemSize -= (__MemSize);\
		}\

#define LOAD_ASSET_STRING_MEMORY(__LengthMax, __OutputEString) \
		EString __OutputEString;\
		{\
			const UINT StrStride = sizeof(CHAR) * (__LengthMax);\
			Check((ENGINE_ASSET_ERROR), ("Check load skeleton animation asset [rest memory size] failed."), (RstMemSize >= StrStride));\
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
			const UINT MemSize = (__ElementStride) * (__ElementNum);\
			Check((ENGINE_ASSET_ERROR), ("Check load skeleton animation asset [rest memory size] failed."), (RstMemSize >= MemSize));\
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
						EString ErrorData("Load skeleton animation asset failed (load file path : ");
						ErrorData += TempFullPathName;
						ErrorData += ") this asset is not skeleton animation type.";
						PE_FAILED((ENGINE_ASSET_ERROR), (ErrorData));
					}
#endif
					delete[]ReadFileMem;
					return LoadedSkeletonAnimationResource;
				}
			}
			{
				LOAD_ASSET_MEMORY(UINT32, sizeof(UINT32), TempSkeletonType);
				ESkeletonType SkeletonType = static_cast<ESkeletonType>(TempSkeletonType);
				if ((TempSkeletonType == ESkeletonType::SKELETON_TYPE_UNKNOWN) || (TempSkeletonType >= ESkeletonType::SKELETON_TYPE_COUNT))
				{
#if _EDITOR_ONLY
					{
						EString ErrorData("Load skeleton animation asset failed (load file path : ");
						ErrorData += TempFullPathName;
						ErrorData += ") this asset is not skeleton animation type.";
						PE_FAILED((ENGINE_ASSET_ERROR), (ErrorData));
					}
#endif
					delete[]ReadFileMem;
					return LoadedSkeletonAnimationResource;
				}
			}
			{
				LOAD_ASSET_STRING_MEMORY(ESettings::ENGINE_SKELETON_NAME_LENGTH_MAX, TempSkeletonName);
				LoadedSkeletonAnimationResource = new ESkeleton(TempSkeletonName);
			}
			{
				LOAD_ASSET_MEMORY(USHORT, sizeof(USHORT), TempBoneNum);

				ESkeleton::EBonePart& BonePart = LoadedSkeletonAnimationResource->Bones;
				TMap<EString, USHORT>& BoneMapping = LoadedSkeletonAnimationResource->BoneMapping;

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

					Check((ENGINE_ASSET_ERROR), ("Check read skeleton animation resource's bone index failed"), (TempIndex == i));

					BoneData.Index = TempIndex;
					BoneData.Name = TempBoneName;
					BoneData.DefaultPosition = TempDefaultPosition;
					BoneData.DefaultRotation = TempDefaultRotation;
					BoneData.DefaultScaling = TempDefaultScaling;
					BoneData.Parent = TempParentIndex;
				}
				for (USHORT i = 0u; i < TempBoneNum; i++)
				{
					LOAD_ASSET_STRING_MEMORY(ESettings::ENGINE_BONE_NAME_LENGTH_MAX, TempBoneName);
					LOAD_ASSET_MEMORY(USHORT, sizeof(USHORT), TempBoneIndex);

					BoneMapping.Add(TempBoneName, TempBoneIndex);
				}
			}

			Check((ENGINE_ASSET_ERROR), ("Check read skeleton animation resource rest memory already failed."), (RstMemSize == 0));
		}

#undef LOAD_ASSET_MEMORY
#undef LOAD_ASSET_STRING_MEMORY
#undef LOAD_ASSET_PTR_MEMORY

		delete[]ReadFileMem;
		if (!(LoadedSkeletonAnimationResource->IsResourceValid()))
		{
			delete LoadedSkeletonAnimationResource;
			LoadedSkeletonAnimationResource = nullptr;
		}
		return LoadedSkeletonAnimationResource;*/
		return nullptr;
	}
	BOOL EAnimationManager::SaveSkeletonAnimationResource(const EString& InSavePath, const EString& InSaveName, const ESkeletonAnimation* InSkeletonAnimationResource)
	{
//		if ((!InSkeletonAnimationResource) || (!(InSkeletonAnimationResource->IsResourceValid())))
//		{
//			PE_FAILED((ENGINE_ASSET_ERROR), ("Check skeleton animation resource error, skeleton animation resource is null."));
//			return FALSE;
//		}
//		EString TempFullPathName(InSavePath);
//		TempFullPathName += InSaveName;
//		TempFullPathName += ENGINE_ASSET_NAME_TYPE;
//		if (TempFullPathName.Length() < 10u)
//		{
//#if _EDITOR_ONLY
//			{
//				EString ErrorData("Save skeleton animation asset path name check failed (save file path : ");
//				ErrorData += InSavePath;
//				ErrorData += ", save file name : ";
//				ErrorData += InSaveName;
//				ErrorData += ").";
//				PE_FAILED((ENGINE_ASSET_ERROR), (ErrorData));
//			}
//#endif
//			return FALSE;
//		}
//
//		auto CalculateSkeletonAnimationBytes = [](const ESkeletonAnimation* InSkeletonAnimation)->ULONG
//		{
//			ULONG Result = 0u;
//
//			Result += sizeof(UINT32);		// EAssetType
//			Result += sizeof(UINT32);		// EAnimationType
//			Result += sizeof(CHAR) * ESettings::ENGINE_ANIMATION_NAME_LENGTH_MAX;	// Skeleton animation name
//			Result += sizeof(UINT32);		// Animation clip num
//
//			const ESkeleton::EBonePart& BonePart = InSkeletonAnimation->get();
//			for (UINT i = 0u, n = BonePart.Length(); i < n; i++)
//			{
//				const EBoneData& BoneData = BonePart[i];
//
//				Result += sizeof(SHORT);		// Bone data index
//				Result += sizeof(CHAR) * ESettings::ENGINE_BONE_NAME_LENGTH_MAX;	// Bone data name
//				Result += sizeof(Vector3);		// Bone data default position
//				Result += sizeof(Quaternion);	// Bone data default rotation
//				Result += sizeof(Vector3);		// Bone data default scaling
//				Result += sizeof(SHORT);		// Bone data parent index
//				Result += sizeof(USHORT);		// Bone data children num
//				Result += sizeof(SHORT) * BoneData.Children.Length();	// Bone data children index
//			}
//			Result += (sizeof(CHAR) * ESettings::ENGINE_BONE_NAME_LENGTH_MAX + sizeof(USHORT)) * BonePart.Length();	// Bone mapping datas
//
//			return Result;
//		};
//
//		const ULONG OutputMemSize = CalculateSkeletonAnimationBytes(InSkeletonAnimationResource);
//		BYTE* OutputMem = new BYTE[OutputMemSize];
//
//		void* RstMemPtr = OutputMem;
//		LONGLONG RstMemSize = static_cast<LONGLONG>(OutputMemSize);
//
//#define SAVE_ASSET_MEMORY(__Type, __Value) \
//		{\
//			Check((ENGINE_ASSET_ERROR), ("Check save skeleton animation asset [rest memory size] failed."), (RstMemSize >= (sizeof(__Type))));\
//			__Type* TempPtr = (__Type*)RstMemPtr;\
//			TempPtr[0] = (__Value);\
//			RstMemPtr = &(TempPtr[1]);\
//			RstMemSize -= (sizeof(__Type));\
//		}\
//
//#define SAVE_ASSET_STRING_MEMORY(__EString, __LengthMax) \
//		{\
//			const UINT NameLengthMax = sizeof(CHAR) * (__LengthMax);\
//			Check((ENGINE_ASSET_ERROR), ("Check save skeleton animation asset [rest memory size] failed."), (RstMemSize >= NameLengthMax));\
//			const EString& SavedName = (__EString);\
//			const UINT NameLength = EMath::Clamp(static_cast<UINT>(sizeof(CHAR) * (SavedName.Length() + 1u)), 0u, NameLengthMax);\
//			CHAR* TempPtr = (CHAR*)RstMemPtr;\
//			::memcpy_s(TempPtr, NameLengthMax, (*SavedName), NameLength);\
//			RstMemPtr = &(TempPtr[(__LengthMax)]);\
//			RstMemSize -= NameLengthMax;\
//		}\
//
//#define SAVE_ASSET_PTR_MEMORY(__ElementStride, __ElementNum, __Ptr) \
//		{\
//			const UINT MemSize = (__ElementStride) * (__ElementNum);\
//			Check((ENGINE_ASSET_ERROR), ("Check save skeleton animation asset [rest memory size] failed."), (RstMemSize >= MemSize));\
//			BYTE* TempPtr = (BYTE*)RstMemPtr;\
//			::memcpy_s(TempPtr, MemSize, (__Ptr), MemSize);\
//			RstMemPtr = &(TempPtr[MemSize]);\
//			RstMemSize -= MemSize;\
//		}\
//
//		{
//			SAVE_ASSET_MEMORY(UINT32, static_cast<UINT32>(EAssetType::ASSET_TYPE_SKELETON));
//			SAVE_ASSET_MEMORY(UINT32, static_cast<UINT32>(InSkeletonAnimationResource->GetSkeletonType()));
//			SAVE_ASSET_STRING_MEMORY(InSkeletonAnimationResource->GetSkeletonName(), ESettings::ENGINE_SKELETON_NAME_LENGTH_MAX);
//
//			const ESkeleton::EBonePart& BonePart = InSkeletonAnimationResource->GetBonePart();
//			const TMap<EString, USHORT>& BoneMapping = InSkeletonAnimationResource->GetBoneMapping();
//			const USHORT BoneNum = static_cast<USHORT>(BonePart.Length());	// Skeleton bone num can not greater than 65535u.
//			Check((ENGINE_ASSET_ERROR), ("Check save skeleton animation asset bone num failed, bone mapping num is not match bone num."), ((BoneNum > 0u) && (BoneNum == static_cast<USHORT>(BoneMapping.Length()))));
//
//			SAVE_ASSET_MEMORY(USHORT, BoneNum);
//
//			for (USHORT i = 0u; i < BoneNum; i++)
//			{
//				const EBoneData& BondData = BonePart[i];
//
//				SAVE_ASSET_MEMORY(SHORT, BondData.Index);
//				SAVE_ASSET_STRING_MEMORY(BondData.Name, ESettings::ENGINE_BONE_NAME_LENGTH_MAX);
//				SAVE_ASSET_MEMORY(Vector3, BondData.DefaultPosition);
//				SAVE_ASSET_MEMORY(Quaternion, BondData.DefaultRotation);
//				SAVE_ASSET_MEMORY(Vector3, BondData.DefaultScaling);
//				SAVE_ASSET_MEMORY(SHORT, BondData.Parent);
//
//				const TArray<SHORT>& ChildrenDatas = BondData.Children;
//				const USHORT ChildrenNum = static_cast<USHORT>(ChildrenDatas.Length());
//
//				SAVE_ASSET_MEMORY(USHORT, ChildrenNum);
//
//				for (USHORT ChildIndex = 0u; ChildIndex < ChildrenNum; ChildIndex++)
//				{
//					SAVE_ASSET_MEMORY(SHORT, ChildrenDatas[ChildIndex]);
//				}
//			}
//			for (auto ItMapping = BoneMapping.Begin(); ItMapping != BoneMapping.End(); ItMapping++)
//			{
//				SAVE_ASSET_STRING_MEMORY(ItMapping->first, ESettings::ENGINE_BONE_NAME_LENGTH_MAX);
//				SAVE_ASSET_MEMORY(USHORT, ItMapping->second);
//			}
//
//			Check((ENGINE_ASSET_ERROR), ("Check write skeleton animation resource rest memory already failed."), (RstMemSize == 0));
//		}
//
//#undef SAVE_ASSET_MEMORY
//#undef SAVE_ASSET_STRING_MEMORY
//#undef SAVE_ASSET_PTR_MEMORY
//
//		if (EFileHelper::SaveBytesToFile(TempFullPathName, OutputMem, OutputMemSize))
//		{
//			delete[]OutputMem;
//			return TRUE;
//		}
//		delete[]OutputMem;
		return FALSE;
	}

};