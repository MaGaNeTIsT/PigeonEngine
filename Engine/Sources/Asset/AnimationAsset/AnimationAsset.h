#pragma once

#include <CoreMinimal.h>
#include <EngineCommon.h>
#include <BaseAsset.h>
#include <AssetLoadManager.h>
#include <Animation/AnimationNotify.h>

namespace PigeonEngine
{

	/*
	* The animation asset file starts with the asset type, which is the one field every
	* PAsset shares because the content browser classifies files by it. The rest of the
	* layout is deliberately unversioned for now: the asset system is expected to move to
	* an Archive-based format, and during development an animation asset can simply be
	* re-imported when this layout changes.
	*/

	enum EAnimationBehaviourType : UINT8
	{
		ANIMATION_BEHAVIOUR_DEFAULT		= 0,
		ANIMATION_BEHAVIOUR_CONSTANT	= 1,
		ANIMATION_BEHAVIOUR_LINEAR		= 2,
		ANIMATION_BEHAVIOUR_REPEAT		= 3
	};

	struct EBoneAnimationCurve : public ETransformTimeCurve<FLOAT>
	{
		EBoneAnimationCurve()noexcept : BoneName(EEngineSettings::ENGINE_DEFAULT_NAME), PreState(EAnimationBehaviourType::ANIMATION_BEHAVIOUR_DEFAULT), PostState(EAnimationBehaviourType::ANIMATION_BEHAVIOUR_DEFAULT) {}
		EBoneAnimationCurve(const EBoneAnimationCurve& Other)noexcept : ETransformTimeCurve<FLOAT>(Other), BoneName(Other.BoneName), PreState(Other.PreState), PostState(Other.PostState) {}
		EBoneAnimationCurve(const EString& InBoneName)noexcept : BoneName(InBoneName), PreState(EAnimationBehaviourType::ANIMATION_BEHAVIOUR_DEFAULT), PostState(EAnimationBehaviourType::ANIMATION_BEHAVIOUR_DEFAULT) {}
		EBoneAnimationCurve& operator=(const EBoneAnimationCurve& Other)
		{
			if (PositionKeys.Num() > 0)
			{
				PositionKeys.Empty();
			}
			if (RotationKeys.Num() > 0)
			{
				RotationKeys.Empty();
			}
			if (ScalingKeys.Num() > 0)
			{
				ScalingKeys.Empty();
			}
			CopyFromOtherCurve(Other);
			BoneName = Other.BoneName;
			PreState = Other.PreState;
			PostState = Other.PostState;
			return (*this);
		}
		BOOL32 IsValid()const
		{
			return ((PositionKeys.Num() > 0) || (RotationKeys.Num() > 0) || (ScalingKeys.Num() > 0));
		}

		EString						BoneName;
		EAnimationBehaviourType		PreState;
		EAnimationBehaviourType		PostState;
	};

	struct ESkeletonAnimationClip
	{
		ESkeletonAnimationClip()noexcept : ClipName(EEngineSettings::ENGINE_DEFAULT_NAME), Duration(0.f), TicksPerSecond(0.f) {}
		ESkeletonAnimationClip(const ESkeletonAnimationClip& Other)noexcept
			: ClipName(Other.ClipName), Duration(Other.Duration), TicksPerSecond(Other.TicksPerSecond)
		{
			if (const UINT32 AnimationCurveNum = Other.AnimationCurves.Num<UINT32>(); AnimationCurveNum > 0u)
			{
				for (UINT32 i = 0u; i < AnimationCurveNum; i++)
				{
					AnimationCurves.Add(Other.AnimationCurves[i]);
				}
			}
			if (const UINT32 NotifyNum = Other.Notifies.Num<UINT32>(); NotifyNum > 0u)
			{
				for (UINT32 i = 0u; i < NotifyNum; i++)
				{
					Notifies.Add(Other.Notifies[i]);
				}
			}
		}
		ESkeletonAnimationClip(const EString& InClipName)noexcept : ClipName(InClipName), Duration(0.f), TicksPerSecond(0.f) {}
		ESkeletonAnimationClip& operator=(const ESkeletonAnimationClip& Other)noexcept
		{
			if (AnimationCurves.Num() > 0)
			{
				AnimationCurves.Empty();
			}
			if (Notifies.Num() > 0)
			{
				Notifies.Empty();
			}
			ClipName = Other.ClipName;
			Duration = Other.Duration;
			TicksPerSecond = Other.TicksPerSecond;
			if (const UINT32 AnimationCurveNum = Other.AnimationCurves.Num<UINT32>(); AnimationCurveNum > 0u)
			{
				for (UINT32 i = 0u; i < AnimationCurveNum; i++)
				{
					AnimationCurves.Add(Other.AnimationCurves[i]);
				}
			}
			if (const UINT32 NotifyNum = Other.Notifies.Num<UINT32>(); NotifyNum > 0u)
			{
				for (UINT32 i = 0u; i < NotifyNum; i++)
				{
					Notifies.Add(Other.Notifies[i]);
				}
			}
			return (*this);
		}
		BOOL32 IsValid()const
		{
			const UINT32 AnimationCurveNum = AnimationCurves.Num();
			// A tick rate of 0 is accepted here because some FBX files report it; the player
			// substitutes EEngineSettings::ENGINE_ANIMATION_DEFAULT_TICKS_PER_SECOND. A
			// negative rate is not a valid source value.
			BOOL32 Result = (Duration > 0.0f) && (TicksPerSecond >= 0.0f) && (AnimationCurveNum > 0u);
			if (Result)
			{
				for (UINT32 i = 0u; i < AnimationCurveNum; i++)
				{
					Result = Result && (AnimationCurves[i].IsValid());
				}
			}
			return Result;
		}

		EString							ClipName;
		FLOAT							Duration;
		FLOAT							TicksPerSecond;
		TArray<EBoneAnimationCurve>		AnimationCurves;
		TArray<FAnimNotifyEvent>		Notifies;
	};

	class ESkeletonAnimation : public EObjectBase, public EResourceInterface
	{
	public:
		virtual BOOL32	IsResourceValid()const override;
		virtual BOOL32	InitResource()override;
		virtual void	ReleaseResource()override;
	public:
		const EString&							GetAnimationName()const;
		INT32									GetAnimationClipNum()const;
		const ESkeletonAnimationClip*			GetAnimationClip(INT32 InIndex)const;
		const ESkeletonAnimationClip*			GetAnimationClip(const EString& InClipName)const;
		const TArray<ESkeletonAnimationClip>&	GetAnimationClips()const;
		const TMap<EString, INT32>&				GetAnimationClipMapping()const;
		// Mutable access is for the editor, which edits the notify list in place before
		// saving the asset. Runtime playback only reads the clips.
		TArray<ESkeletonAnimationClip>&			GetAnimationClips();
		ESkeletonAnimationClip*					GetAnimationClip(INT32 InIndex);
		ESkeletonAnimationClip*					GetAnimationClip(const EString& InClipName);
		PE_NODISCARD const EString&				GetSkeletonName()const;
		PE_NODISCARD const EString&				GetRootBoneName()const;
	public:
		void	RebuildWholeMapping();
		BOOL32	AddAnimationClip(const ESkeletonAnimationClip& InClip);
		BOOL32	RemoveAnimationClip(INT32 InIndex);
		BOOL32	RemoveAnimationClip(const EString& InClipName);
		void	SetSkeletonName(const EString& InSkeletonName);
		void	SetRootBoneName(const EString& InRootBoneName);
	protected:
		EString							AnimationName;
		/*
		* The skeleton the clip was authored against, when the source or the importer could
		* name it. It is a compatibility hint rather than a hard reference: the runtime bind
		* still resolves each curve by bone name, so a renamed or retargeted skeleton can be
		* used, with a warning when the names differ.
		*/
		EString							SkeletonName;
		EString							RootBoneName;
		TArray<ESkeletonAnimationClip>	AnimationClips;
		TMap<EString, INT32>			AnimationClipMapping;
	public:
		ESkeletonAnimation(const EString& InName);
		virtual ~ESkeletonAnimation();
	private:
		friend class EAnimationManager;
	public:
		ESkeletonAnimation() = delete;

		CLASS_REMOVE_COPY_BODY(ESkeletonAnimation)

	};

	class ESkeletonAnimationAsset : public TBaseAsset<ESkeletonAnimation>
	{
	public:
		ESkeletonAnimationAsset(const EString& InAssetPath, const EString& InAssetName
#if _EDITOR_ONLY
			, const EString& InDebugName
#endif
		);
		virtual ~ESkeletonAnimationAsset();
	public:
		virtual BOOL32	InitResource()override;
	private:
		friend class EAnimationManager;
	public:
		ESkeletonAnimationAsset() = delete;

		CLASS_REMOVE_COPY_BODY(ESkeletonAnimationAsset)
	};

	class EAnimationManager : public EManagerBase
	{
	public:
		typedef TAssetManager<EString, ESkeletonAnimationAsset>	ESkeletonAnimationAssetManager;
	public:
		virtual void	Initialize()override;
		virtual void	ShutDown()override;
		virtual void	Uninit()override;
	public:
#if _EDITOR_ONLY
		/*
		* Imports an animation asset from a raw source file. InSkeletonName and InRootBoneName
		* are the skeleton the clips were authored against and the bone that carries root
		* motion; both may be empty when the source or the importer cannot name them. They are
		* written into the asset and used as compatibility hints, not as hard references.
		*/
		BOOL32	ImportSkeletonAnimation(const EString& InAssetName, const EString& InImportFullPathName, const EString& InSavePath
			, const EString& InSkeletonName = EString(), const EString& InRootBoneName = EString());
#endif
		BOOL32	LoadSkeletonAnimationAsset(const EString& InLoadPath, const EString& InLoadName, const ESkeletonAnimationAsset*& OutSkeletonAnimationAsset, const BOOL32 bTryLoad = FALSE);
		// Editor save: writes the asset back to the path and name it was loaded from.
		BOOL32	SaveSkeletonAnimationAsset(const ESkeletonAnimationAsset* InSkeletonAnimationAsset);
		// Re-applies every standalone notify asset to loaded clip events in place. Event
		// addresses stay stable, so active notify-state windows remain valid.
		void	RefreshNotifyReferences();
		/*
		* Any thread: the animation when it is already loaded, otherwise FALSE and a null output
		* after recording one load request for the load thread (EAssetLoadManager). Never blocks
		* and never reads the disk on the calling thread; ask again next frame. Load* above is the
		* load thread's synchronous half. InImportSource is the editor's fallback for an animation
		* that was never imported: the load thread imports from the raw source and loads again.
		* InOnLoadComplete runs on the load thread once the request is done; see
		* EAssetLoadCompleteType.
		*/
		BOOL32	RequireLoadSkeletonAnimationAsset(const EString& InLoadPath, const EString& InLoadName
			, const ESkeletonAnimationAsset*& OutSkeletonAnimationAsset
			, const FAssetImportSource* InImportSource = nullptr
			, EAssetLoadCompleteType InOnLoadComplete = nullptr);
	private:
		void	ClearSkeletonAnimations();
		// Releases every asset this manager caches. Shared by ShutDown and Uninit so the two
		// paths cannot drift apart.
		void	ReleaseAllAssets();
	private:
		ESkeletonAnimationAsset* LoadSkeletonAnimationAsset(const EString& InLoadPath, const EString& InLoadName, const BOOL32 bTryLoad = FALSE);
		BOOL32 SaveSkeletonAnimationAsset(const EString& InSavePath, const EString& InSaveName, const ESkeletonAnimationAsset* InSkeletonAnimationAsset);
		ESkeletonAnimation* LoadSkeletonAnimationResource(const EString& InLoadPath, const EString& InLoadName, const BOOL32 bTryLoad = FALSE);
		BOOL32 SaveSkeletonAnimationResource(const EString& InSavePath, const EString& InSaveName, const ESkeletonAnimation* InSkeletonAnimationResource);
	private:
		ESkeletonAnimationAssetManager	SkeletonAnimationAssetManager;

		CLASS_MANAGER_VIRTUAL_SINGLETON_BODY(EAnimationManager)

	};

	/*
	* Loads an animation asset, and when the PAsset is not on disk yet and an import source
	* is supplied, imports it from the raw file and loads again. This is the editor's fallback;
	* a shipping build only performs the load.
	*/
	extern void TryLoadSkeletonAnimation(const EString& InLoadPath, const EString& InLoadName,
		const ESkeletonAnimationAsset*& OutAsset,
		const EString* InImportPath = nullptr, const EString* InImportName = nullptr, const EString* InImportFileType = nullptr);

};
