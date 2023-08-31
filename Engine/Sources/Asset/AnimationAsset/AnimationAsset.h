#pragma once

#include <CoreMinimal.h>
#include <EngineCommon.h>
#include <BaseAsset.h>

namespace PigeonEngine
{

	enum EAnimationBehaviourType : UINT8
	{
		ANIMATION_BEHAVIOUR_DEFAULT		= 0,
		ANIMATION_BEHAVIOUR_CONSTANT	= 1,
		ANIMATION_BEHAVIOUR_LINEAR		= 2,
		ANIMATION_BEHAVIOUR_REPEAT		= 3
	};

	struct EBoneAnimationCurve : public ETransformTimeCurve<FLOAT>
	{
		EBoneAnimationCurve()noexcept : BoneName(ENGINE_DEFAULT_NAME), PreState(EAnimationBehaviourType::ANIMATION_BEHAVIOUR_DEFAULT), PostState(EAnimationBehaviourType::ANIMATION_BEHAVIOUR_DEFAULT) {}
		EBoneAnimationCurve(const EBoneAnimationCurve& Other)noexcept : ETransformTimeCurve<FLOAT>(Other), BoneName(Other.BoneName), PreState(Other.PreState), PostState(Other.PostState) {}
		EBoneAnimationCurve(const EString& InBoneName)noexcept : BoneName(InBoneName), PreState(EAnimationBehaviourType::ANIMATION_BEHAVIOUR_DEFAULT), PostState(EAnimationBehaviourType::ANIMATION_BEHAVIOUR_DEFAULT) {}
		EBoneAnimationCurve& operator=(const EBoneAnimationCurve& Other)
		{
			if (PositionKeys.Length() > 0u)
			{
				PositionKeys.Clear();
			}
			if (RotationKeys.Length() > 0u)
			{
				RotationKeys.Clear();
			}
			if (ScalingKeys.Length() > 0u)
			{
				ScalingKeys.Clear();
			}
			CopyFromOtherCurve(Other);
			BoneName = Other.BoneName;
			PreState = Other.PreState;
			PostState = Other.PostState;
			return (*this);
		}
		BOOL IsValid()const
		{
			return ((PositionKeys.Length() > 0u) || (RotationKeys.Length() > 0u) || (ScalingKeys.Length() > 0u));
		}

		EString									BoneName;
		EAnimationBehaviourType					PreState;
		EAnimationBehaviourType					PostState;
	};

	struct ESkeletonAnimationClip
	{
		ESkeletonAnimationClip()noexcept : ClipName(ENGINE_DEFAULT_NAME), Duration(0.f), TicksPerSecond(0.f) {}
		ESkeletonAnimationClip(const ESkeletonAnimationClip& Other)noexcept
			: ClipName(Other.ClipName), Duration(Other.Duration), TicksPerSecond(Other.TicksPerSecond)
		{
			if (const UINT AnimationCurveNum = Other.AnimationCurves.Length(); AnimationCurveNum > 0u)
			{
				for (UINT i = 0u; i < AnimationCurveNum; i++)
				{
					AnimationCurves.Add(Other.AnimationCurves[i]);
				}
			}
		}
		ESkeletonAnimationClip(const EString& InClipName)noexcept : ClipName(InClipName), Duration(0.f), TicksPerSecond(0.f) {}
		ESkeletonAnimationClip& operator=(const ESkeletonAnimationClip& Other)noexcept
		{
			if (AnimationCurves.Length() > 0u)
			{
				AnimationCurves.Clear();
			}
			ClipName = Other.ClipName;
			Duration = Other.Duration;
			TicksPerSecond = Other.TicksPerSecond;
			if (const UINT AnimationCurveNum = Other.AnimationCurves.Length(); AnimationCurveNum > 0u)
			{
				for (UINT i = 0u; i < AnimationCurveNum; i++)
				{
					AnimationCurves.Add(Other.AnimationCurves[i]);
				}
			}
			return (*this);
		}
		BOOL IsValid()const
		{
			const UINT AnimationCurveNum = AnimationCurves.Length();
			BOOL Result = (Duration > 0.0f) && (TicksPerSecond > 0.0f) && (AnimationCurveNum > 0u);
			if (Result)
			{
				for (UINT i = 0u; i < AnimationCurveNum; i++)
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
	};

	class ESkeletonAnimation : public EObjectBase, public EResourceInterface
	{
	public:
		virtual BOOL	IsResourceValid()const override;
		virtual BOOL	InitResource()override;
		virtual void	ReleaseResource()override;
	public:
		const EString&							GetAnimationName()const;
		UINT									GetAnimationClipNum()const;
		const ESkeletonAnimationClip*			GetAnimationClip(UINT InIndex)const;
		const ESkeletonAnimationClip*			GetAnimationClip(const EString& InClipName)const;
		const TArray<ESkeletonAnimationClip>&	GetAnimationClips()const;
		const TMap<EString, UINT>&				GetAnimationClipMapping()const;
	public:
		void	RebuildWholeMapping();
		BOOL	AddAnimationClip(const ESkeletonAnimationClip& InClip);
		BOOL	RemoveAnimationClip(UINT InIndex);
		BOOL	RemoveAnimationClip(const EString& InClipName);
	protected:
		EString							AnimationName;
		TArray<ESkeletonAnimationClip>	AnimationClips;
		TMap<EString, UINT>				AnimationClipMapping;
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
		virtual BOOL	InitResource()override;
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
	public:
#if _EDITOR_ONLY
		BOOL	ImportSkeletonAnimation(const EString& InAssetName, const EString& InImportFullPathName, const EString& InSavePath);
#endif
		BOOL	LoadSkeletonAnimationAsset(const EString& InLoadPath, const EString& InLoadName, const ESkeletonAnimationAsset*& OutSkeletonAnimationAsset);
	private:
		void	ClearSkeletonAnimations();
	private:
		ESkeletonAnimationAsset* LoadSkeletonAnimationAsset(const EString& InLoadPath, const EString& InLoadName);
		BOOL SaveSkeletonAnimationAsset(const EString& InSavePath, const EString& InSaveName, const ESkeletonAnimationAsset* InSkeletonAnimationAsset);
		ESkeletonAnimation* LoadSkeletonAnimationResource(const EString& InLoadPath, const EString& InLoadName);
		BOOL SaveSkeletonAnimationResource(const EString& InSavePath, const EString& InSaveName, const ESkeletonAnimation* InSkeletonAnimationResource);
	private:
		ESkeletonAnimationAssetManager	SkeletonAnimationAssetManager;

		CLASS_MANAGER_VIRTUAL_SINGLETON_BODY(EAnimationManager)

	};

};