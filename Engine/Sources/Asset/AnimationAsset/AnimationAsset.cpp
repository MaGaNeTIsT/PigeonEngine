#include "AnimationAsset.h"
#include <Asset/AssetLoadManager.h>
#if _EDITOR_ONLY
#include <Editor/EditorLogManager.h>
#include "../../../EngineThirdParty/assimp/Headers/assimpManager.h"
#endif

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<ESkeletonAnimation, EObjectBase, EResourceInterface>();

		RegisterBaseAssetClassTypes<ESkeletonAnimation>();

		RegisterClassType<ESkeletonAnimationAsset, TBaseAsset<ESkeletonAnimation>>();

		RegisterClassType<EAnimationManager, EManagerBase>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	ESkeletonAnimation::ESkeletonAnimation(const EString& InName)
		: AnimationName(InName)
		, SkeletonName(EEngineSettings::ENGINE_DEFAULT_NAME)
		, RootBoneName(EEngineSettings::ENGINE_DEFAULT_NAME)
	{
	}
	ESkeletonAnimation::~ESkeletonAnimation()
	{
		ReleaseResource();
	}
	BOOL32 ESkeletonAnimation::IsResourceValid()const
	{
		BOOL32 Result = FALSE;
		if (const INT32 AnimationClipNum = AnimationClips.Num(); AnimationClipNum > 0)
		{
			for (INT32 i = 0; i < AnimationClipNum; i++)
			{
				Result = Result || (AnimationClips[i].IsValid());
			}
		}
		PE_CHECK((ENGINE_ASSET_ERROR), ("Check if skeleton animation mapping num fits failed."), (AnimationClipMapping.Num() == AnimationClips.Num()));
		return Result;
	}
	BOOL32 ESkeletonAnimation::InitResource()
	{
		// Skeleton animation resource must init by animation manager.
		return TRUE;
	}
	void ESkeletonAnimation::ReleaseResource()
	{
		// Skeleton animation resource do not need release.
		AnimationClips.Empty();
		AnimationClipMapping.Empty();
	}
	const EString& ESkeletonAnimation::GetAnimationName()const
	{
		return AnimationName;
	}
	INT32 ESkeletonAnimation::GetAnimationClipNum()const
	{
		return (AnimationClips.Num());
	}
	const ESkeletonAnimationClip* ESkeletonAnimation::GetAnimationClip(INT32 InIndex)const
	{
		if ((InIndex >= 0) && (InIndex < AnimationClips.Num()))
		{
			return (&(AnimationClips[InIndex]));
		}
		return nullptr;
	}
	const ESkeletonAnimationClip* ESkeletonAnimation::GetAnimationClip(const EString& InClipName)const
	{
		if (INT32 FindIndex = 0; AnimationClipMapping.FindValue(InClipName, FindIndex))
		{
			return (GetAnimationClip(FindIndex));
		}
		return nullptr;
	}
	const TArray<ESkeletonAnimationClip>& ESkeletonAnimation::GetAnimationClips()const
	{
		return AnimationClips;
	}
	const TMap<EString, INT32>& ESkeletonAnimation::GetAnimationClipMapping()const
	{
		return AnimationClipMapping;
	}
	TArray<ESkeletonAnimationClip>& ESkeletonAnimation::GetAnimationClips()
	{
		return AnimationClips;
	}
	ESkeletonAnimationClip* ESkeletonAnimation::GetAnimationClip(INT32 InIndex)
	{
		if ((InIndex >= 0) && (InIndex < AnimationClips.Num()))
		{
			return (&(AnimationClips[InIndex]));
		}
		return nullptr;
	}
	ESkeletonAnimationClip* ESkeletonAnimation::GetAnimationClip(const EString& InClipName)
	{
		INT32 FindIndex = 0;
		if (AnimationClipMapping.FindValue(InClipName, FindIndex))
		{
			return (GetAnimationClip(FindIndex));
		}
		return nullptr;
	}
	const EString& ESkeletonAnimation::GetSkeletonName()const
	{
		return SkeletonName;
	}
	const EString& ESkeletonAnimation::GetRootBoneName()const
	{
		return RootBoneName;
	}
	void ESkeletonAnimation::SetSkeletonName(const EString& InSkeletonName)
	{
		SkeletonName = InSkeletonName;
	}
	void ESkeletonAnimation::SetRootBoneName(const EString& InRootBoneName)
	{
		RootBoneName = InRootBoneName;
	}
	void ESkeletonAnimation::RebuildWholeMapping()
	{
		AnimationClipMapping.Empty();
		for (INT32 i = 0u, n = AnimationClips.Num(); i < n; i++)
		{
			AnimationClipMapping.Add(AnimationClips[i].ClipName, i);
		}
		PE_CHECK((ENGINE_ASSET_ERROR), ("Check if skeleton animation mapping num fits failed."), (AnimationClipMapping.Num() == AnimationClips.Num()));
	}
	BOOL32 ESkeletonAnimation::AddAnimationClip(const ESkeletonAnimationClip& InClip)
	{
		if (InClip.IsValid())
		{
			if (!(AnimationClipMapping.ContainsKey(InClip.ClipName)))
			{
				AnimationClips.Add(InClip);
				AnimationClipMapping.Add(InClip.ClipName, AnimationClips.Num() - 1);
				PE_CHECK((ENGINE_ASSET_ERROR), ("Check if skeleton animation mapping num fits failed."), (AnimationClipMapping.Num() == AnimationClips.Num()));
				return TRUE;
			}
#if _EDITOR_ONLY
			else
			{
				EString ErrorInfo = EString("Try to add animation clip but already exist [animation clip name = ") + InClip.ClipName + "].";
				PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorInfo));
			}
#endif
		}
		PE_FAILED((ENGINE_ASSET_ERROR), ("Add skeleton animation clip failed."));
		return FALSE;
	}
	BOOL32 ESkeletonAnimation::RemoveAnimationClip(INT32 InIndex)
	{
		if ((InIndex >= 0) && (InIndex < AnimationClips.Num()))
		{
			AnimationClips.RemoveAt(InIndex);

			//TODO
			// This operation is quite slow because we
			// rebuild whole mapping when remove animation clip.
			// This will be fixed in any time.
			RebuildWholeMapping();

			return TRUE;
		}
		return FALSE;
	}
	BOOL32 ESkeletonAnimation::RemoveAnimationClip(const EString& InClipName)
	{
		if (INT32 FindIndex = 0u; AnimationClipMapping.FindValue(InClipName, FindIndex))
		{
			return (RemoveAnimationClip(FindIndex));
		}
		return FALSE;
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
	BOOL32 ESkeletonAnimationAsset::InitResource()
	{
		// Skeleton animation resource do not need to initialize.
		if (IsInitialized())
		{
#if _EDITOR_ONLY
			{
				EString ErrorInfo = EString("Skeleton animation name=[") + GetAssetName() + "] path = [" + GetAssetPath() + "] has been Initialized.";
				PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorInfo));
			}
#endif
			return TRUE;
		}
		bIsInitialized = TRUE;
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
		ReleaseAllAssets();
	}
	void EAnimationManager::Uninit()
	{
		ReleaseAllAssets();
	}
	void EAnimationManager::ReleaseAllAssets()
	{
		ClearSkeletonAnimations();
	}
#if _EDITOR_ONLY
	BOOL32 EAnimationManager::ImportSkeletonAnimation(const EString& InAssetName, const EString& InImportFullPathName, const EString& InSavePath
		, const EString& InSkeletonName, const EString& InRootBoneName)
	{
		EString TempFullPathName(InSavePath);
		TempFullPathName = TempFullPathName + InAssetName + EEngineSettings::ENGINE_ASSET_NAME_TYPE;
		if ((InImportFullPathName.Length() < 3u) || (TempFullPathName.Length() < 10u))
		{
#if _EDITOR_ONLY
			{
				EString ErrorData("Error file path for skeleton animation importer (import file path : ");
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
				EString ErrorData("Error file path for skeleton animation importer (import file path : ");
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
				EString ErrorData("Error file path for skeleton animation importer (import file path : ");
				ErrorData = ErrorData + InImportFullPathName + ", save assset path : " + TempFullPathName + ").";
				PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorData));
			}
#endif
			return FALSE;
		}
		EString UsedSkeletonAnimation;
		if (InAssetName.Length() < 2u)
		{
			UsedSkeletonAnimation = EEngineSettings::ENGINE_DEFAULT_NAME;
		}
		else
		{
			UsedSkeletonAnimation = InAssetName;
		}
		ESkeletonAnimation AssimpSkeletonAnimation(UsedSkeletonAnimation);
		AssimpSkeletonAnimation.SetSkeletonName(InSkeletonName);
		AssimpSkeletonAnimation.SetRootBoneName(InRootBoneName);
		CAssimpManager* AssimpManager = CAssimpManager::GetManagerSingleton();
		TArray<ESkeletonAnimationClip> AssimpSkeletonAnimationClips;
		if ((AssimpManager->ReadSkeletonAnimationFile(InImportFullPathName, AssimpSkeletonAnimationClips)) != (CAssimpManager::CReadFileStateType::ASSIMP_READ_FILE_STATE_SUCCEED))
		{
#if _EDITOR_ONLY
			{
				EString ErrorData("Assimp importer can not load skeleton animation file from path (import file path : ");
				ErrorData = ErrorData + InImportFullPathName + ", save assset path : " + TempFullPathName + ").";
				PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorData));
			}
#endif
			return FALSE;
		}
		for (UINT32 i = 0u, n = AssimpSkeletonAnimationClips.Num(); i < n; i++)
		{
			AssimpSkeletonAnimation.AddAnimationClip(AssimpSkeletonAnimationClips[i]);
		}
		return (SaveSkeletonAnimationResource(InSavePath, InAssetName, (&(AssimpSkeletonAnimation))));
	}
#endif
	BOOL32 EAnimationManager::LoadSkeletonAnimationAsset(const EString& InLoadPath, const EString& InLoadName
		, const ESkeletonAnimationAsset*& OutSkeletonAnimationAsset, const BOOL32 bTryLoad)
	{
		EString TempFullPathName(InLoadPath);
		TempFullPathName = TempFullPathName + InLoadName + EEngineSettings::ENGINE_ASSET_NAME_TYPE;
		ESkeletonAnimationAsset* ResultAsset = SkeletonAnimationAssetManager.Find(TempFullPathName);
		if (ResultAsset)
		{
			OutSkeletonAnimationAsset = ResultAsset;
			return TRUE;
		}
		ResultAsset = LoadSkeletonAnimationAsset(InLoadPath, InLoadName, bTryLoad);
		if (!ResultAsset)
		{
			return FALSE;
		}
		if (!ResultAsset->InitResource())
		{
			delete ResultAsset;
			return FALSE;
		}
		if (SkeletonAnimationAssetManager.Add(TempFullPathName, ResultAsset, TRUE) == 0u)
		{
#if _EDITOR_ONLY
			{
				EString ErrorInfo = EString("Pixel shader path = [") + TempFullPathName + "] add into manager list failed.";
				PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorInfo));
			}
#endif
			delete ResultAsset;
			return FALSE;
		}
		OutSkeletonAnimationAsset = ResultAsset;
		return TRUE;
	}
	BOOL32 EAnimationManager::RequireLoadSkeletonAnimationAsset(const EString& InLoadPath, const EString& InLoadName
		, const ESkeletonAnimationAsset*& OutSkeletonAnimationAsset
		, const FAssetImportSource* InImportSource
		, EAssetLoadCompleteType InOnLoadComplete)
	{
		const EString FullPathName = MakeAssetFullPathName(InLoadPath, InLoadName);
		if (ESkeletonAnimationAsset* LoadedAsset = SkeletonAnimationAssetManager.Find(FullPathName))
		{
			OutSkeletonAnimationAsset = LoadedAsset;
			return TRUE;
		}
		OutSkeletonAnimationAsset = nullptr;
		// The import source is copied into the request: a request outlives the frame that made
		// it, so it may not read the caller's strings.
		FAssetImportSource ImportSource;
		if (InImportSource)
		{
			ImportSource = *InImportSource;
		}
		EAssetLoadManager::GetManagerSingleton()->RequestLoad(
			MakeAssetLoadRequestKey(PE_TEXT("SkeletonAnimation"), FullPathName),
			[InLoadPath, InLoadName, ImportSource]() -> BOOL32
			{
				const ESkeletonAnimationAsset* LoadedAsset = nullptr;
				if (ImportSource.IsValid())
				{
					// The editor form: load, and when the PAsset is not on disk yet, import from
					// the raw source and load again. Both halves run here, on the load thread.
					TryLoadSkeletonAnimation(InLoadPath, InLoadName, LoadedAsset, &ImportSource.ImportPath
						, &ImportSource.ImportName, &ImportSource.ImportFileType);
				}
				else
				{
					EAnimationManager::GetManagerSingleton()->LoadSkeletonAnimationAsset(InLoadPath, InLoadName, LoadedAsset);
				}
				return ((LoadedAsset != nullptr) ? TRUE : FALSE);
			},
			static_cast<EAssetLoadCompleteType&&>(InOnLoadComplete));
		return FALSE;
	}
	BOOL32 EAnimationManager::SaveSkeletonAnimationAsset(const ESkeletonAnimationAsset* InSkeletonAnimationAsset)
	{
		if (!InSkeletonAnimationAsset)
		{
			return FALSE;
		}
		return SaveSkeletonAnimationAsset(InSkeletonAnimationAsset->GetAssetPath()
			, InSkeletonAnimationAsset->GetAssetName(), InSkeletonAnimationAsset);
	}
	void EAnimationManager::ClearSkeletonAnimations()
	{
		SkeletonAnimationAssetManager.Clear();
	}
	ESkeletonAnimationAsset* EAnimationManager::LoadSkeletonAnimationAsset(const EString& InLoadPath, const EString& InLoadName, const BOOL32 bTryLoad)
	{
		ESkeletonAnimationAsset* NewAsset = nullptr;
		ESkeletonAnimation* LoadedResource = LoadSkeletonAnimationResource(InLoadPath, InLoadName, bTryLoad);
		if ((!LoadedResource) || (!(LoadedResource->IsResourceValid())))
		{
			return NewAsset;
		}
		NewAsset = new ESkeletonAnimationAsset(InLoadPath, InLoadName
#if _EDITOR_ONLY
			, InLoadName
#endif
		);
		if (!(NewAsset->StorageResourceInternal(
			[LoadedResource]()->ESkeletonAnimation*
			{
				return LoadedResource;
			})))
		{
			//TODO
			delete NewAsset;
			NewAsset = nullptr;
			return NewAsset;
		}
		return NewAsset;
	}
	BOOL32 EAnimationManager::SaveSkeletonAnimationAsset(const EString& InSavePath, const EString& InSaveName, const ESkeletonAnimationAsset* InSkeletonAnimationAsset)
	{
		if (!InSkeletonAnimationAsset)
		{
			PE_FAILED((ENGINE_ASSET_ERROR), ("Error try to save a null skeleton animation asset."));
			return FALSE;
		}
		const ESkeletonAnimation* SavedResource = InSkeletonAnimationAsset->GetStoragedResource();
		if ((!SavedResource) || (!(SavedResource->IsResourceValid())))
		{
			PE_FAILED((ENGINE_ASSET_ERROR), ("Error try to save a skeleton animation asset without resource."));
			return FALSE;
		}
		return (SaveSkeletonAnimationResource(InSavePath, InSaveName, SavedResource));
	}
	ESkeletonAnimation* EAnimationManager::LoadSkeletonAnimationResource(const EString& InLoadPath, const EString& InLoadName, const BOOL32 bTryLoad)
	{
		EString TempFullPathName(InLoadPath);
		TempFullPathName = TempFullPathName + InLoadName + EEngineSettings::ENGINE_ASSET_NAME_TYPE;
		if (TempFullPathName.Length() < 10u)
		{
#if _EDITOR_ONLY
			{
				EString ErrorData("Load skeleton animation asset path name check failed (load file path : ");
				ErrorData += InLoadPath;
				ErrorData += ", load file name : ";
				ErrorData += InLoadName;
				ErrorData += ").";
				PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorData));
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
				if (bTryLoad)
				{
					PE_LOG_WARN(ErrorData);
				}
				else
				{
					PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorData));
				}
			}
#endif
			return LoadedSkeletonAnimationResource;
		}

		const void* RstMemPtr = ReadFileMem;
		LONGLONG RstMemSize = static_cast<LONGLONG>(ReadFileSize);

#define LOAD_ASSET_MEMORY(__MemType, __MemSize, __OutputValue) \
		__MemType __OutputValue;\
		{\
			PE_CHECK((ENGINE_ASSET_ERROR), ("Check load skeleton animation asset [rest memory size] failed."), (RstMemSize >= (__MemSize)));\
			const __MemType* TempPtr = (const __MemType*)RstMemPtr;\
			(__OutputValue) = TempPtr[0];\
			RstMemPtr = &(TempPtr[1]);\
			RstMemSize -= (__MemSize);\
		}\

#define LOAD_ASSET_STRING_MEMORY(__LengthMax, __OutputEString) \
		EString __OutputEString;\
		{\
			const UINT32 StrStride = sizeof(CHAR) * (__LengthMax);\
			PE_CHECK((ENGINE_ASSET_ERROR), ("Check load skeleton animation asset [rest memory size] failed."), (RstMemSize >= StrStride));\
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
			PE_CHECK((ENGINE_ASSET_ERROR), ("Check load skeleton animation asset [rest memory size] failed."), (RstMemSize >= MemSize));\
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
				if (AssetType != EAssetType::ASSET_TYPE_ANIMATION)
				{
#if _EDITOR_ONLY
					{
						EString ErrorData("Load skeleton animation asset failed (load file path : ");
						ErrorData += TempFullPathName;
						ErrorData += ") this asset is not skeleton animation type.";
						PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorData));
					}
#endif
					delete[]ReadFileMem;
					return LoadedSkeletonAnimationResource;
				}
			}
			{
				LOAD_ASSET_MEMORY(UINT32, sizeof(UINT32), TempAnimationType);
				EAnimationType AnimationType = static_cast<EAnimationType>(TempAnimationType);
				if ((AnimationType == EAnimationType::ANIMATION_TYPE_UNKNOWN) || (AnimationType >= EAnimationType::ANIMATION_TYPE_COUNT))
				{
#if _EDITOR_ONLY
					{
						EString ErrorData("Load skeleton animation asset failed (load file path : ");
						ErrorData += TempFullPathName;
						ErrorData += ") this asset is not skeleton animation type.";
						PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorData));
					}
#endif
					delete[]ReadFileMem;
					return LoadedSkeletonAnimationResource;
				}
			}
			{
				LOAD_ASSET_STRING_MEMORY(EEngineSettings::ENGINE_ANIMATION_NAME_LENGTH_MAX, TempAnimationName);
				LOAD_ASSET_STRING_MEMORY(EEngineSettings::ENGINE_SKELETON_NAME_LENGTH_MAX, TempSkeletonName);
				LOAD_ASSET_STRING_MEMORY(EEngineSettings::ENGINE_BONE_NAME_LENGTH_MAX, TempRootBoneName);
				LoadedSkeletonAnimationResource = new ESkeletonAnimation(TempAnimationName);
				LoadedSkeletonAnimationResource->SkeletonName = TempSkeletonName;
				LoadedSkeletonAnimationResource->RootBoneName = TempRootBoneName;
			}
			{
				LOAD_ASSET_MEMORY(UINT32, sizeof(UINT32), TempAnimationClipNum);

				TArray<ESkeletonAnimationClip>& AnimationClips = LoadedSkeletonAnimationResource->AnimationClips;

				for (UINT32 i = 0u; i < TempAnimationClipNum; i++)
				{
					LOAD_ASSET_STRING_MEMORY(EEngineSettings::ENGINE_ANIMATION_CLIP_NAME_LENGTH_MAX, TempAnimationClipName);
					LOAD_ASSET_MEMORY(FLOAT, sizeof(FLOAT), TempDuration);
					LOAD_ASSET_MEMORY(FLOAT, sizeof(FLOAT), TempTicksPerSecond);
					LOAD_ASSET_MEMORY(UINT32, sizeof(UINT32), TempAnimationCurveNum);

					ESkeletonAnimationClip& AnimationClip = AnimationClips.AddDefaultGetRef();
					AnimationClip.ClipName = TempAnimationClipName;
					AnimationClip.Duration = TempDuration;
					AnimationClip.TicksPerSecond = TempTicksPerSecond;

					TArray<EBoneAnimationCurve>& BoneAnimationCurves = AnimationClip.AnimationCurves;

					for (UINT32 j = 0u; j < TempAnimationCurveNum; j++)
					{
						LOAD_ASSET_STRING_MEMORY(EEngineSettings::ENGINE_BONE_NAME_LENGTH_MAX, TempBoneName);
						LOAD_ASSET_MEMORY(EAnimationBehaviourType, sizeof(EAnimationBehaviourType), TempPreState);
						LOAD_ASSET_MEMORY(EAnimationBehaviourType, sizeof(EAnimationBehaviourType), TempPostState);

						EBoneAnimationCurve& BoneAnimationCurve = BoneAnimationCurves.AddDefaultGetRef();
						BoneAnimationCurve.BoneName = TempBoneName;
						BoneAnimationCurve.PreState = TempPreState;
						BoneAnimationCurve.PostState = TempPostState;

						// Bone animation curve transform key part
						{
							TArray<EBoneAnimationCurve::TPositionKey>& TransformPositionKeys = BoneAnimationCurve.PositionKeys;
							TArray<EBoneAnimationCurve::TRotationKey>& TransformRotationKeys = BoneAnimationCurve.RotationKeys;
							TArray<EBoneAnimationCurve::TScalingKey>& TransformScalingKeys = BoneAnimationCurve.ScalingKeys;

							LOAD_ASSET_MEMORY(UINT32, sizeof(UINT32), TempPositionKeyNum);
							LOAD_ASSET_MEMORY(UINT32, sizeof(UINT32), TempRotationKeyNum);
							LOAD_ASSET_MEMORY(UINT32, sizeof(UINT32), TempScalingKeyNum);

							for (UINT32 k = 0u; k < TempPositionKeyNum; k++)
							{
								LOAD_ASSET_MEMORY(EBoneAnimationCurve::TPositionKey, sizeof(EBoneAnimationCurve::TPositionKey), TempPositionKey);

								TransformPositionKeys.Add(TempPositionKey);
							}
							for (UINT32 k = 0u; k < TempRotationKeyNum; k++)
							{
								LOAD_ASSET_MEMORY(EBoneAnimationCurve::TRotationKey, sizeof(EBoneAnimationCurve::TRotationKey), TempRotationKey);

								TransformRotationKeys.Add(TempRotationKey);
							}
							for (UINT32 k = 0u; k < TempScalingKeyNum; k++)
							{
								LOAD_ASSET_MEMORY(EBoneAnimationCurve::TScalingKey, sizeof(EBoneAnimationCurve::TScalingKey), TempScalingKey);

								TransformScalingKeys.Add(TempScalingKey);
							}
						}
					}

					LOAD_ASSET_MEMORY(UINT32, sizeof(UINT32), TempNotifyNum);
					for (UINT32 j = 0u; j < TempNotifyNum; j++)
					{
						LOAD_ASSET_STRING_MEMORY(EEngineSettings::ENGINE_ANIMATION_NOTIFY_NAME_LENGTH_MAX, TempNotifyName);
						LOAD_ASSET_STRING_MEMORY(EEngineSettings::ENGINE_ANIMATION_NOTIFY_CLASS_NAME_LENGTH_MAX, TempNotifyClassName);
						LOAD_ASSET_MEMORY(FLOAT, sizeof(FLOAT), TempNotifyTime);
						LOAD_ASSET_STRING_MEMORY(EEngineSettings::ENGINE_ANIMATION_NOTIFY_PAYLOAD_LENGTH_MAX, TempNotifyPayload);
						LOAD_ASSET_MEMORY(UINT32, sizeof(UINT32), TempNotifyIsState);
						LOAD_ASSET_MEMORY(FLOAT, sizeof(FLOAT), TempNotifyStateDuration);

						FAnimNotifyEvent& Notify = AnimationClip.Notifies.AddDefaultGetRef();
						Notify.NotifyName = TempNotifyName;
						Notify.NotifyClassName = TempNotifyClassName;
						Notify.Time = TempNotifyTime;
						Notify.PayloadJson = TempNotifyPayload;
						Notify.bIsState = ((TempNotifyIsState != 0u) ? TRUE : FALSE);
						Notify.StateDuration = TempNotifyStateDuration;
						// Build the cached notify object once, here, so playback never allocates.
						Notify.Compile();
					}
				}
			}
			PE_CHECK((ENGINE_ASSET_ERROR), ("Check read skeleton animation resource rest memory already failed."), (RstMemSize == 0));
		}

#undef LOAD_ASSET_MEMORY
#undef LOAD_ASSET_STRING_MEMORY
#undef LOAD_ASSET_PTR_MEMORY

		delete[]ReadFileMem;

		LoadedSkeletonAnimationResource->RebuildWholeMapping();
		if (!(LoadedSkeletonAnimationResource->IsResourceValid()))
		{
			delete LoadedSkeletonAnimationResource;
			LoadedSkeletonAnimationResource = nullptr;
		}
		return LoadedSkeletonAnimationResource;
	}
	BOOL32 EAnimationManager::SaveSkeletonAnimationResource(const EString& InSavePath, const EString& InSaveName, const ESkeletonAnimation* InSkeletonAnimationResource)
	{
		if ((!InSkeletonAnimationResource) || (!(InSkeletonAnimationResource->IsResourceValid())))
		{
			PE_FAILED((ENGINE_ASSET_ERROR), ("Check skeleton animation resource error, skeleton animation resource is null."));
			return FALSE;
		}
		EString TempFullPathName(InSavePath);
		TempFullPathName = TempFullPathName + InSaveName + EEngineSettings::ENGINE_ASSET_NAME_TYPE;
		if (TempFullPathName.Length() < 10u)
		{
#if _EDITOR_ONLY
			{
				EString ErrorData("Save skeleton animation asset path name check failed (save file path : ");
				ErrorData += InSavePath;
				ErrorData += ", save file name : ";
				ErrorData += InSaveName;
				ErrorData += ").";
				PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorData));
			}
#endif
			return FALSE;
		}

		auto CalculateSkeletonAnimationBytes = [](const ESkeletonAnimation* InSkeletonAnimation)->ULONG
		{
			ULONG Result = 0u;

			Result += sizeof(UINT32);		// EAssetType
			Result += sizeof(UINT32);		// EAnimationType
			Result += sizeof(CHAR) * EEngineSettings::ENGINE_ANIMATION_NAME_LENGTH_MAX;	// Skeleton animation name
			Result += sizeof(CHAR) * EEngineSettings::ENGINE_SKELETON_NAME_LENGTH_MAX;	// Bound skeleton name
			Result += sizeof(CHAR) * EEngineSettings::ENGINE_BONE_NAME_LENGTH_MAX;		// Root bone name
			Result += sizeof(UINT32);		// Animation clip num

			const TArray<ESkeletonAnimationClip>& AnimationClips = InSkeletonAnimation->GetAnimationClips();
			for (INT32 i = 0, n = AnimationClips.Num(); i < n; i++)
			{
				const ESkeletonAnimationClip& AnimationClip = AnimationClips[i];

				Result += sizeof(CHAR) * EEngineSettings::ENGINE_ANIMATION_CLIP_NAME_LENGTH_MAX;	// Animation clip name
				Result += sizeof(FLOAT);	// Animation clip duration
				Result += sizeof(FLOAT);	// Animation clip ticks per second
				Result += sizeof(UINT32);	// Animation clip bone animation curve num

				const TArray<EBoneAnimationCurve>& BoneAnimationCurves = AnimationClip.AnimationCurves;
				for (INT32 j = 0, m = BoneAnimationCurves.Num(); j < m; j++)
				{
					const EBoneAnimationCurve& BoneAnimationCurve = BoneAnimationCurves[j];

					Result += sizeof(CHAR) * EEngineSettings::ENGINE_BONE_NAME_LENGTH_MAX;	// Bone animation curve bone name
					Result += sizeof(EAnimationBehaviourType);	// Bone animation curve pre state
					Result += sizeof(EAnimationBehaviourType);	// Bone animation curve post state

					// Bone animation curve transform key part
					{
						Result += sizeof(UINT32);	// Bone animation curve position key num
						Result += sizeof(UINT32);	// Bone animation curve rotation key num
						Result += sizeof(UINT32);	// Bone animation curve scaling key num

						Result += sizeof(EBoneAnimationCurve::TPositionKey) * BoneAnimationCurve.PositionKeys.Num<UINT32>();	// Bone animation curve position key datas
						Result += sizeof(EBoneAnimationCurve::TRotationKey) * BoneAnimationCurve.RotationKeys.Num<UINT32>();	// Bone animation curve rotation key datas
						Result += sizeof(EBoneAnimationCurve::TScalingKey) * BoneAnimationCurve.ScalingKeys.Num<UINT32>();		// Bone animation curve scaling key datas
					}
				}

				const TArray<FAnimNotifyEvent>& Notifies = AnimationClip.Notifies;
				Result += sizeof(UINT32);	// Animation clip notify num
				for (INT32 j = 0, m = Notifies.Num(); j < m; j++)
				{
					Result += sizeof(CHAR) * EEngineSettings::ENGINE_ANIMATION_NOTIFY_NAME_LENGTH_MAX;			// Notify name
					Result += sizeof(CHAR) * EEngineSettings::ENGINE_ANIMATION_NOTIFY_CLASS_NAME_LENGTH_MAX;	// Notify class name
					Result += sizeof(FLOAT);																		// Notify time
					Result += sizeof(CHAR) * EEngineSettings::ENGINE_ANIMATION_NOTIFY_PAYLOAD_LENGTH_MAX;		// Notify payload
					Result += sizeof(UINT32);																		// Notify is-state flag
					Result += sizeof(FLOAT);																		// Notify state duration
				}
			}

			return Result;
		};

		const ULONG OutputMemSize = CalculateSkeletonAnimationBytes(InSkeletonAnimationResource);
		BYTE* OutputMem = new BYTE[OutputMemSize];
		// The fixed-width string fields are only partly overwritten by a short name. Clearing
		// the buffer first makes every unused tail byte deterministic and guarantees that a
		// field truncated at its maximum still ends in a terminator.
		::memset(OutputMem, 0, OutputMemSize);

		void* RstMemPtr = OutputMem;
		LONGLONG RstMemSize = static_cast<LONGLONG>(OutputMemSize);

#define SAVE_ASSET_MEMORY(__Type, __Value) \
		{\
			PE_CHECK((ENGINE_ASSET_ERROR), ("Check save skeleton animation asset [rest memory size] failed."), (RstMemSize >= (sizeof(__Type))));\
			__Type* TempPtr = (__Type*)RstMemPtr;\
			TempPtr[0] = (__Value);\
			RstMemPtr = &(TempPtr[1]);\
			RstMemSize -= (sizeof(__Type));\
		}\

#define SAVE_ASSET_STRING_MEMORY(__EString, __LengthMax) \
		{\
			const UINT32 NameLengthMax = sizeof(CHAR) * (__LengthMax);\
			PE_CHECK((ENGINE_ASSET_ERROR), ("Check save skeleton animation asset [rest memory size] failed."), (RstMemSize >= NameLengthMax));\
			const EString& SavedName = (__EString);\
			const UINT32 NameLength = EMath::Clamp(static_cast<UINT32>(sizeof(CHAR) * (SavedName.Length() + 1u)), 0u, NameLengthMax);\
			CHAR* TempPtr = (CHAR*)RstMemPtr;\
			::memcpy_s(TempPtr, NameLengthMax, (*SavedName), NameLength);\
			if (NameLength >= NameLengthMax)\
			{\
				TempPtr[NameLengthMax - 1u] = '\0';\
			}\
			RstMemPtr = &(TempPtr[(__LengthMax)]);\
			RstMemSize -= NameLengthMax;\
		}\

#define SAVE_ASSET_PTR_MEMORY(__ElementStride, __ElementNum, __Ptr) \
		{\
			const UINT32 MemSize = (__ElementStride) * (__ElementNum);\
			PE_CHECK((ENGINE_ASSET_ERROR), ("Check save skeleton animation asset [rest memory size] failed."), (RstMemSize >= MemSize));\
			BYTE* TempPtr = (BYTE*)RstMemPtr;\
			::memcpy_s(TempPtr, MemSize, (__Ptr), MemSize);\
			RstMemPtr = &(TempPtr[MemSize]);\
			RstMemSize -= MemSize;\
		}\

		{
			SAVE_ASSET_MEMORY(UINT32, static_cast<UINT32>(EAssetType::ASSET_TYPE_ANIMATION));
			SAVE_ASSET_MEMORY(UINT32, static_cast<UINT32>(EAnimationType::ANIMATION_TYPE_SKELETON));
			SAVE_ASSET_STRING_MEMORY(InSkeletonAnimationResource->GetAnimationName(), EEngineSettings::ENGINE_ANIMATION_NAME_LENGTH_MAX);
			SAVE_ASSET_STRING_MEMORY(InSkeletonAnimationResource->GetSkeletonName(), EEngineSettings::ENGINE_SKELETON_NAME_LENGTH_MAX);
			SAVE_ASSET_STRING_MEMORY(InSkeletonAnimationResource->GetRootBoneName(), EEngineSettings::ENGINE_BONE_NAME_LENGTH_MAX);

			const TArray<ESkeletonAnimationClip>& AnimationClips = InSkeletonAnimationResource->GetAnimationClips();
			const INT32 AnimationClipNum = AnimationClips.Num();

			SAVE_ASSET_MEMORY(UINT32, static_cast<UINT32>(AnimationClipNum));

			for (INT32 i = 0; i < AnimationClipNum; i++)
			{
				const ESkeletonAnimationClip& AnimationClip = AnimationClips[i];

				SAVE_ASSET_STRING_MEMORY(AnimationClip.ClipName, EEngineSettings::ENGINE_ANIMATION_CLIP_NAME_LENGTH_MAX);
				SAVE_ASSET_MEMORY(FLOAT, AnimationClip.Duration);
				SAVE_ASSET_MEMORY(FLOAT, AnimationClip.TicksPerSecond);

				const TArray<EBoneAnimationCurve>& BoneAnimationCurves = AnimationClip.AnimationCurves;
				const INT32 BoneAnimationCurveNum = BoneAnimationCurves.Num();

				SAVE_ASSET_MEMORY(UINT32, static_cast<UINT32>(BoneAnimationCurveNum));

				for (INT32 j = 0; j < BoneAnimationCurveNum; j++)
				{
					const EBoneAnimationCurve& BoneAnimationCurve = BoneAnimationCurves[j];

					SAVE_ASSET_STRING_MEMORY(BoneAnimationCurve.BoneName, EEngineSettings::ENGINE_BONE_NAME_LENGTH_MAX);
					SAVE_ASSET_MEMORY(EAnimationBehaviourType, BoneAnimationCurve.PreState);
					SAVE_ASSET_MEMORY(EAnimationBehaviourType, BoneAnimationCurve.PostState);

					// Bone animation curve transform key part
					{
						const TArray<EBoneAnimationCurve::TPositionKey>& TransformPositionKeys = BoneAnimationCurve.PositionKeys;
						const TArray<EBoneAnimationCurve::TRotationKey>& TransformRotationKeys = BoneAnimationCurve.RotationKeys;
						const TArray<EBoneAnimationCurve::TScalingKey>& TransformScalingKeys = BoneAnimationCurve.ScalingKeys;
						const INT32 TransformPositionKeyNum = TransformPositionKeys.Num();
						const INT32 TransformRotationKeyNum = TransformRotationKeys.Num();
						const INT32 TransformScalingKeyNum = TransformScalingKeys.Num();

						SAVE_ASSET_MEMORY(UINT32, static_cast<UINT32>(TransformPositionKeyNum));
						SAVE_ASSET_MEMORY(UINT32, static_cast<UINT32>(TransformRotationKeyNum));
						SAVE_ASSET_MEMORY(UINT32, static_cast<UINT32>(TransformScalingKeyNum));

						for (INT32 k = 0; k < TransformPositionKeyNum; k++)
						{
							const EBoneAnimationCurve::TPositionKey& TransformPositionKey = TransformPositionKeys[k];

							SAVE_ASSET_MEMORY(EBoneAnimationCurve::TPositionKey, TransformPositionKey);
						}
						for (INT32 k = 0; k < TransformRotationKeyNum; k++)
						{
							const EBoneAnimationCurve::TRotationKey& TransformRotationKey = TransformRotationKeys[k];

							SAVE_ASSET_MEMORY(EBoneAnimationCurve::TRotationKey, TransformRotationKey);
						}
						for (INT32 k = 0; k < TransformScalingKeyNum; k++)
						{
							const EBoneAnimationCurve::TScalingKey& TransformScalingKey = TransformScalingKeys[k];

							SAVE_ASSET_MEMORY(EBoneAnimationCurve::TScalingKey, TransformScalingKey);
						}
					}
				}

				const TArray<FAnimNotifyEvent>& Notifies = AnimationClip.Notifies;
				const INT32 NotifyNum = Notifies.Num();
				SAVE_ASSET_MEMORY(UINT32, static_cast<UINT32>(NotifyNum));
				for (INT32 j = 0; j < NotifyNum; j++)
				{
					const FAnimNotifyEvent& Notify = Notifies[j];
					SAVE_ASSET_STRING_MEMORY(Notify.NotifyName, EEngineSettings::ENGINE_ANIMATION_NOTIFY_NAME_LENGTH_MAX);
					SAVE_ASSET_STRING_MEMORY(Notify.NotifyClassName, EEngineSettings::ENGINE_ANIMATION_NOTIFY_CLASS_NAME_LENGTH_MAX);
					SAVE_ASSET_MEMORY(FLOAT, Notify.Time);
					SAVE_ASSET_STRING_MEMORY(Notify.PayloadJson, EEngineSettings::ENGINE_ANIMATION_NOTIFY_PAYLOAD_LENGTH_MAX);
					SAVE_ASSET_MEMORY(UINT32, (Notify.bIsState ? 1u : 0u));
					SAVE_ASSET_MEMORY(FLOAT, Notify.StateDuration);
				}
			}
			PE_CHECK((ENGINE_ASSET_ERROR), ("Check write skeleton animation resource rest memory already failed."), (RstMemSize == 0));
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

	void TryLoadSkeletonAnimation(const EString& InLoadPath, const EString& InLoadName
		, const ESkeletonAnimationAsset*& OutAsset
		, const EString* InImportPath, const EString* InImportName, const EString* InImportFileType)
	{
		EAnimationManager* AnimationManager = EAnimationManager::GetManagerSingleton();
		PE_CHECK((ENGINE_ASSET_ERROR), ("Check output skeleton animation asset pointer is initialized failed."), (!OutAsset));
		if (AnimationManager->LoadSkeletonAnimationAsset(InLoadPath, InLoadName, OutAsset, TRUE))
		{
			return;
		}
#if _EDITOR_ONLY
		const BOOL32 bHasImport = (!!InImportPath) && (!!InImportName) && (!!InImportFileType);
		if (bHasImport)
		{
			PE_LOG_WARN(EString("Skeleton animation '") + InLoadName + "' not yet on disk; will attempt import.");
		}
		else
		{
			PE_FAILED((ENGINE_ASSET_ERROR), ("Load skeleton animation asset failed."));
		}
		if (bHasImport)
		{
			EString TempImportFullPath(*InImportPath);
			TempImportFullPath = TempImportFullPath + (*InImportName) + (".") + (*InImportFileType);
			if (AnimationManager->ImportSkeletonAnimation(InLoadName, TempImportFullPath, InLoadPath))
			{
				if (!(AnimationManager->LoadSkeletonAnimationAsset(InLoadPath, InLoadName, OutAsset, TRUE)))
				{
					PE_FAILED((ENGINE_ASSET_ERROR), ("Try load skeleton animation asset failed."));
				}
			}
			else
			{
				PE_FAILED((ENGINE_ASSET_ERROR), ("Try import skeleton animation asset failed."));
			}
		}
#else
		PE_FAILED((ENGINE_ASSET_ERROR), ("Load skeleton animation asset failed."));
#endif
	}

};
