#pragma once

#include <CoreMinimal.h>
#include <RenderResource.h>
#include <EngineCommon.h>
#include <BaseAsset.h>

namespace PigeonEngine
{

	enum ESkeletonType : UINT8
	{
		SKELETON_TYPE_UNKNOWN = 0,
		SKELETON_TYPE_NORMAL,
		SKELETON_TYPE_COUNT
	};

	struct EBoneData
	{
		EBoneData()noexcept
			: Index(-2), Name(ESettings::ENGINE_DEFAULT_NAME), DefaultPosition(Vector3::Zero())
			, DefaultRotation(Quaternion::Identity()), DefaultScaling(Vector3::One()), Parent(-2)
		{
		}
		EBoneData(const EBoneData& Other)noexcept
			: Index(Other.Index), Name(Other.Name), DefaultPosition(Other.DefaultPosition)
			, DefaultRotation(Other.DefaultRotation), DefaultScaling(Other.DefaultScaling), Parent(Other.Parent)
		{
			if (Other.Children.Length() > 0u)
			{
				for (UINT32 i = 0u, n = Other.Children.Length(); i < n; i++)
				{
					Children.Add(Other.Children[i]);
				}
			}
		}
		EBoneData(const EString& InName)noexcept
			: Index(-2), Name(InName), DefaultPosition(Vector3::Zero())
			, DefaultRotation(Quaternion::Identity()), DefaultScaling(Vector3::One()), Parent(-2)
		{
		}
		EBoneData& operator=(const EBoneData& Other)
		{
			Children.Clear();
			Index			= Other.Index;
			Name			= Other.Name;
			DefaultPosition	= Other.DefaultPosition;
			DefaultRotation	= Other.DefaultRotation;
			DefaultScaling	= Other.DefaultScaling;
			Parent			= Other.Parent;
			if (Other.Children.Length() > 0u)
			{
				for (UINT32 i = 0u, n = Other.Children.Length(); i < n; i++)
				{
					Children.Add(Other.Children[i]);
				}
			}
			return (*this);
		}
		BOOL32 operator==(const EBoneData& Other)
		{
			return (Name == Other.Name);
		}
		BOOL32 operator!=(const EBoneData& Other)
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

	struct EBoneTransform
	{
		EBoneTransform() : Position(Vector3::Zero()), Rotation(Quaternion::Identity()), Scaling(Vector3::One()) {}
		EBoneTransform(const EBoneTransform& Other) : Position(Other.Position), Rotation(Other.Rotation), Scaling(Other.Scaling) {}
		~EBoneTransform() {}
		EBoneTransform& operator=(const EBoneTransform& Other)
		{
			Position = Other.Position; Rotation = Other.Rotation; Scaling = Other.Scaling;
			return (*this);
		}

		Vector3			Position;
		Quaternion		Rotation;
		Vector3			Scaling;
	};

	extern PE_INLINE Matrix4x4 MakeMatrix4x4(const EBoneTransform& InBoneTransform);

	class ESkeleton : public EObjectBase, public EResourceInterface
	{
	public:
		typedef TArray<EBoneData>	EBonePart;
	public:
		virtual BOOL32	IsResourceValid()const override;
		virtual BOOL32	InitResource()override;
		virtual void	ReleaseResource()override;
	public:
		ESkeletonType					GetSkeletonType()const;
		const EString&					GetSkeletonName()const;
		const EBonePart&				GetBonePart()const;
		const TMap<EString, USHORT>&	GetBoneMapping()const;
		UINT32							GetBoneCount()const;
	public:
		BOOL32	AddBoneElement(EBoneData* InIndexData);
		BOOL32	RemoveBoneElement(const EString& InBoneName);
		BOOL32	RemoveBoneElement(USHORT InBoneIndex, EString* OutBoneName = nullptr);
		BOOL32	GetBoneElement(const EString& InBoneName, const EBoneData*& OutBoneData)const;
		BOOL32	GetBoneElement(USHORT InBoneIndex, const EBoneData*& OutBoneData)const;
	protected:
		void			RemoveBoneInternal(USHORT InBoneIndex);
	protected:
		ESkeletonType			SkeletonType;
		EString					SkeletonName;
		EBonePart				Bones;
		TMap<EString, USHORT>	BoneMapping;
	public:
		ESkeleton(const EString& InSkeletonName);
		virtual ~ESkeleton();
	private:
		friend class ESkeletonAssetManager;
	public:
		ESkeleton() = delete;

		CLASS_REMOVE_COPY_BODY(ESkeleton)
	};

	class ESkeletonRenderResource : public EObjectBase, public RRenderResourceInterface
	{
	public:
		enum ESkeletonRenderResourceType : UINT8
		{
			SKELETON_RENDER_RESOURCE_MATRIX						= 0,
			SKELETON_RENDER_RESOURCE_INVERSE_TRANSPOSE_MATRIX,
			SKELETON_RENDER_RESOURCE_COUNT
		};
	public:
		void			SetBoneNum(const UINT32 InBoneNum);
		UINT32			GetBoneNum()const;
		void			UpdateRenderResource(const Matrix4x4* InMatrices, const UINT32 InMatrixNum);
	public:
		virtual BOOL32	IsRenderResourceValid()const override;
		virtual BOOL32	InitRenderResource()override;
		virtual void	ReleaseRenderResource()override;
	protected:
		UINT32				BoneNum;
		RStructuredBuffer	RenderResource[ESkeletonRenderResourceType::SKELETON_RENDER_RESOURCE_COUNT];
	public:
		ESkeletonRenderResource();
		virtual ~ESkeletonRenderResource();
	private:
		friend class ESkeletonAssetManager;
	public:

		CLASS_REMOVE_COPY_BODY(ESkeletonRenderResource)

	};

	class ESkeletonMemoryPool
	{
	public:

	private:
		TArray<EBoneTransform> BoneTransforms;
	};

	class ESkeletonAsset : public TBaseAsset<ESkeleton>
	{
	public:
		ESkeletonAsset(const EString& InAssetPath, const EString& InAssetName
#if _EDITOR_ONLY
			, const EString& InDebugName
#endif
		);
		virtual ~ESkeletonAsset();
	public:
		virtual BOOL32	InitResource()override;
	private:
		friend class ESkeletonAssetManager;
	public:
		ESkeletonAsset() = delete;

		CLASS_REMOVE_COPY_BODY(ESkeletonAsset)

	};

	class ESkeletonAssetManager : public EManagerBase
	{
	public:
		typedef TAssetManager<EString, ESkeletonAsset>	ESkeletonAssetDataManager;
	public:
		virtual void	Initialize()override;
		virtual void	ShutDown()override;
	public:
#if _EDITOR_ONLY
		BOOL32	ImportSkeleton(const EString& InAssetName, const EString& InImportFullPathName, const EString& InSavePath);
#endif
		BOOL32	LoadSkeletonAsset(const EString& InLoadPath, const EString& InLoadName, const ESkeletonAsset*& OutSkeletonAsset);
	private:
		void	ClearSkeletons();
	private:
		ESkeletonAsset* LoadSkeletonAsset(const EString& InLoadPath, const EString& InLoadName);
		BOOL32 SaveSkeletonAsset(const EString& InSavePath, const EString& InSaveName, const ESkeletonAsset* InSkeletonAsset);
		ESkeleton* LoadSkeletonResource(const EString& InLoadPath, const EString& InLoadName);
		BOOL32 SaveSkeletonResource(const EString& InSavePath, const EString& InSaveName, const ESkeleton* InSkeletonResource);
	private:
		ESkeletonAssetDataManager SkeletonAssetDataManager;

		CLASS_MANAGER_VIRTUAL_SINGLETON_BODY(ESkeletonAssetManager)

	};

	extern void TryLoadSkeleton(const EString& InLoadPath, const EString& InLoadName,
		const ESkeletonAsset*& OutAsset,
		const EString* InImportPath = nullptr, const EString* InImportName = nullptr, const EString* InImportFileType = nullptr);

};