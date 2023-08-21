#pragma once

#include <CoreMinimal.h>
#include <RenderResource.h>
#include <EngineCommon.h>
#include <BaseAsset.h>

namespace PigeonEngine
{

	struct EBoneData
	{
		EBoneData()noexcept
			: Index(-2), Name(ENGINE_DEFAULT_NAME), DefaultPosition(Vector3::Zero())
			, DefaultRotation(Quaternion::Identity()), DefaultScaling(Vector3::One()), Parent(-2)
		{
		}
		EBoneData(const EString& InName)noexcept
			: Index(-2), Name(InName), DefaultPosition(Vector3::Zero())
			, DefaultRotation(Quaternion::Identity()), DefaultScaling(Vector3::One()), Parent(-2)
		{
		}
		BOOL operator==(const EBoneData& Other)
		{
			return (Name == Other.Name);
		}
		BOOL operator!=(const EBoneData& Other)
		{
			return (Name != Other.Name);
		}

		SHORT			Index;
		EString			Name;
		Vector3			DefaultPosition;
		Quaternion		DefaultRotation;
		Vector3			DefaultScaling;
		SHORT			Parent;
		TArray<SHORT>	Children;
	};

	class ESkeleton : public EObjectBase, public EResourceInterface
	{
	public:
		typedef TArray<EBoneData>	EBonePart;
	public:
		virtual BOOL	IsResourceValid()const override;
		virtual BOOL	InitResource()override;
		virtual void	ReleaseResource()override;
	public:
		UINT	GetBoneCount()const;
		BOOL	AddBoneElement(EBoneData* InIndexData);
		BOOL	RemoveBoneElement(const EString& InBoneName);
		BOOL	RemoveBoneElement(USHORT InBoneIndex, EString* OutBoneName = nullptr);
		BOOL	GetBoneElement(const EString& InBoneName, const EBoneData*& OutBoneData)const;
		BOOL	GetBoneElement(USHORT InBoneIndex, const EBoneData*& OutBoneData)const;
	protected:
		void	RemoveBoneInternal(USHORT InBoneIndex);
	protected:
		EString					SkeletonName;
		EBonePart				Bones;
		TMap<EString, USHORT>	BoneMapping;
	public:
		ESkeleton(const EString& InSkeletonName);
		virtual ~ESkeleton();
	public:
		ESkeleton() = delete;

		CLASS_REMOVE_COPY_BODY(ESkeleton)
	};

	class ESkeletonRenderResource : public EObjectBase, public RRenderResourceInterface
	{
	public:
		virtual BOOL	IsRenderResourceValid()const override;
		virtual BOOL	InitRenderResource()override;
		virtual void	ReleaseRenderResource()override;
	protected:
		ESkeleton*				Skeleton;
		TArray<RBufferResource>	RenderResources;
	public:
		ESkeletonRenderResource(ESkeleton* InSkeleton);
		virtual ~ESkeletonRenderResource();
	public:
		ESkeletonRenderResource() = delete;

		CLASS_REMOVE_COPY_BODY(ESkeletonRenderResource)

	};

	class ESkeletonAsset : public TRenderBaseAsset<ESkeleton, ESkeletonRenderResource>
	{
	public:
		ESkeletonAsset(const EString& InSkeletonPath
#ifdef _EDITOR_ONLY
			, const EString& InDebugName
#endif
		);
		virtual ~ESkeletonAsset();
	public:
		const EString&	GetSkeletonPath()const;
	public:
		virtual BOOL	InitResource()override;
	protected:
		EString			SkeletonPath;
	public:
		ESkeletonAsset() = delete;

		CLASS_REMOVE_COPY_BODY(ESkeletonAsset)

	};

};