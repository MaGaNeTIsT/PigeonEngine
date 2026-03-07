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
			: Index(-2), Name(EEngineSettings::ENGINE_DEFAULT_NAME), DefaultPosition(Vector3::Zero())
			, DefaultRotation(Quaternion::Identity()), DefaultScaling(Vector3::One()), Parent(-2)
		{
		}
		EBoneData(const EBoneData& Other)noexcept
			: Index(Other.Index), Name(Other.Name), DefaultPosition(Other.DefaultPosition)
			, DefaultRotation(Other.DefaultRotation), DefaultScaling(Other.DefaultScaling), Parent(Other.Parent)
		{
			if (Other.Children.Num() > 0)
			{
				for (UINT32 i = 0u, n = Other.Children.Num<UINT32>(); i < n; i++)
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
			Children.Empty();
			Index			= Other.Index;
			Name			= Other.Name;
			DefaultPosition	= Other.DefaultPosition;
			DefaultRotation	= Other.DefaultRotation;
			DefaultScaling	= Other.DefaultScaling;
			Parent			= Other.Parent;
			if (Other.Children.Num() > 0)
			{
				for (UINT32 i = 0u, n = Other.Children.Num<UINT32>(); i < n; i++)
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

	extern PE_INLINE Matrix4x4 MakeMatrix4x4(const struct EBoneTransform& InBoneTransform);

	struct EBoneTransform
	{
		EBoneTransform(const Vector3& InPosition, const Quaternion& InRotation, const Vector3& InScaling) : Position(InPosition), Rotation(InRotation), Scaling(InScaling) {}
		EBoneTransform() : Position(Vector3::Zero()), Rotation(Quaternion::Identity()), Scaling(Vector3::One()) {}
		EBoneTransform(const EBoneTransform& Other) : Position(Other.Position), Rotation(Other.Rotation), Scaling(Other.Scaling) {}
		EBoneTransform& operator=(const EBoneTransform& Other)
		{
			Position = Other.Position; Rotation = Other.Rotation; Scaling = Other.Scaling;
			return (*this);
		}
		static const EBoneTransform& Identity()
		{
			static EBoneTransform _StaticBoneTransformIdentity(Vector3::Zero(), Quaternion::Identity(), Vector3::One());
			return (_StaticBoneTransformIdentity);
		}

		PE_INLINE Matrix4x4 ToMatrix4x4()const
		{
			return MakeMatrix4x4(*this);
		}

		Vector3			Position;
		Quaternion		Rotation;
		Vector3			Scaling;
	};

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
		EBonePart&						GetBonePart();
		const EBonePart&				GetBonePart()const;
		TMap<EString, USHORT>&			GetBoneMapping();
		const TMap<EString, USHORT>&	GetBoneMapping()const;
		INT32							GetBoneCount()const;
		USHORT							GetBoneIndex(const EString& InBoneName)const;
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
		void						SetBoneNum(const UINT32 InBoneNum);
		UINT32						GetBoneNum()const;
		const RStructuredBuffer&	GetRenderResource(ESkeletonRenderResourceType InType)const;
		void						UpdateRenderResource(const TArray<Matrix4x4>& InMatrices);
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

#define _USE_MATRIX_FOR_BONE_TO_ROOT	1

	class ESkeletonBoneMemoryPool
	{
	public:
		ESkeletonBoneMemoryPool(const ESkeleton* InRawSkeletonPtr);
	public:
		BOOL32	IsValid()const;
		void	GenerateFromSkeleton(const ESkeleton* InRawSkeletonPtr);
		void	RecursionToRootTransforms();
	public:
		const ESkeleton*				GetRawSkeleton()const;
		const EBoneTransform&			GetRootTransform()const;
		const TArray<EBoneTransform>&	GetBoneRelativeTransforms()const;
		TArray<EBoneTransform>&			GetBoneRelativeTransforms();
#if (_USE_MATRIX_FOR_BONE_TO_ROOT)
		const TArray<Matrix4x4>&		GetBoneToRootTransforms()const;
		TArray<Matrix4x4>&				GetBoneToRootTransforms();
		const Matrix4x4&				GetBoneToRootTransform(const EString& InBoneName)const;
#else
		const TArray<EBoneTransform>&	GetBoneToRootTransforms()const;
		TArray<EBoneTransform>&			GetBoneToRootTransforms();
		const EBoneTransform&			GetBoneToRootTransform(const EString& InBoneName)const;
#endif
		const EBoneTransform&			GetBoneRelativeTransform(const EString& InBoneName)const;
	public:
		void	SetRootTransform(const Vector3& InPosition, const Quaternion& InRotation, const Vector3& InScaling);
		void	SetBoneRelativePosition(const EString& InBoneName, const Vector3& InPosition);
		void	SetBoneRelativeRotation(const EString& InBoneName, const Quaternion& InRotation);
		void	SetBoneRelativeScaling(const EString& InBoneName, const Vector3& InScaling);
		void	SetBoneRelativeTransform(const EString& InBoneName, const Vector3& InPosition, const Quaternion& InRotation);
		void	SetBoneRelativeTransform(const EString& InBoneName, const Vector3& InPosition, const Vector3& InScaling);
		void	SetBoneRelativeTransform(const EString& InBoneName, const Quaternion& InRotation, const Vector3& InScaling);
		void	SetBoneRelativeTransform(const EString& InBoneName, const EBoneTransform& InBoneTransform);
	public:
		template<typename _TCustomType, typename _TLambdaType, typename _TConditionType>
		void BackwardRecursionBone(SHORT InBoneIndex, _TCustomType InValue, const _TLambdaType& InFunc, const _TConditionType& InCond)
		{
			if ((InBoneIndex < 0) || (!RawSkeletonPtr) || (((SHORT)(RawSkeletonPtr->GetBoneCount())) <= InBoneIndex))
			{
				return;
			}
			const EBoneData& CurrentBoneData = (RawSkeletonPtr->GetBonePart())[InBoneIndex];
			if (InCond(CurrentBoneData, InValue))
			{
				_TCustomType NewValue = InFunc(CurrentBoneData, InValue);
				if (CurrentBoneData.Children.Num() > 0)
				{
					for (INT32 ChildIndex = 0, ChildNum = CurrentBoneData.Children.Num(); ChildIndex < ChildNum; ChildIndex++)
					{
						BackwardRecursionBone<_TCustomType, _TLambdaType, _TConditionType>(CurrentBoneData.Children[ChildIndex], NewValue, InFunc, InCond);
					}
				}
			}
		}
		template<typename _TCustomType, typename _TLambdaType, typename _TConditionType>
		void BackwardRecursionBone(SHORT InBoneIndex, _TCustomType InValue, const _TLambdaType& InFunc, const _TConditionType& InCond)const
		{
			if ((InBoneIndex < 0) || (!RawSkeletonPtr) || (((SHORT)(RawSkeletonPtr->GetBoneCount())) <= InBoneIndex))
			{
				return;
			}
			const EBoneData& CurrentBoneData = (RawSkeletonPtr->GetBonePart())[InBoneIndex];
			if (InCond(CurrentBoneData, InValue))
			{
				_TCustomType NewValue = InFunc(CurrentBoneData, InValue);
				if (CurrentBoneData.Children.Length() > 0u)
				{
					for (UINT32 ChildIndex = 0u, ChildNum = CurrentBoneData.Children.Length(); ChildIndex < ChildNum; ChildIndex++)
					{
						BackwardRecursionBone<_TCustomType, _TLambdaType, _TConditionType>(CurrentBoneData.Children[ChildIndex], NewValue, InFunc, InCond);
					}
				}
			}
		}
		template<typename _TLambdaType, typename _TConditionType>
		void BackwardRecursionBone(SHORT InBoneIndex, const _TLambdaType& InFunc, const _TConditionType& InCond)
		{
			if ((InBoneIndex < 0) || (!RawSkeletonPtr) || (((SHORT)(RawSkeletonPtr->GetBoneCount())) <= InBoneIndex))
			{
				return;
			}
			const EBoneData& CurrentBoneData = (RawSkeletonPtr->GetBonePart())[InBoneIndex];
			if (InCond(CurrentBoneData))
			{
				InFunc(CurrentBoneData);
				if (CurrentBoneData.Children.Length() > 0u)
				{
					for (UINT32 ChildIndex = 0u, ChildNum = CurrentBoneData.Children.Length(); ChildIndex < ChildNum; ChildIndex++)
					{
						BackwardRecursionBone<_TLambdaType, _TConditionType>(CurrentBoneData.Children[ChildIndex], InFunc, InCond);
					}
				}
			}
		}
		template<typename _TLambdaType, typename _TConditionType>
		void BackwardRecursionBone(SHORT InBoneIndex, const _TLambdaType& InFunc, const _TConditionType& InCond)const
		{
			if ((InBoneIndex < 0) || (!RawSkeletonPtr) || (((SHORT)(RawSkeletonPtr->GetBoneCount())) <= InBoneIndex))
			{
				return;
			}
			const EBoneData& CurrentBoneData = (RawSkeletonPtr->GetBonePart())[InBoneIndex];
			if (InCond(CurrentBoneData))
			{
				InFunc(CurrentBoneData);
				if (CurrentBoneData.Children.Length() > 0u)
				{
					for (UINT32 ChildIndex = 0u, ChildNum = CurrentBoneData.Children.Length(); ChildIndex < ChildNum; ChildIndex++)
					{
						BackwardRecursionBone<_TLambdaType, _TConditionType>(CurrentBoneData.Children[ChildIndex], InFunc, InCond);
					}
				}
			}
		}
		template<typename _TCustomType, typename _TLambdaType, typename _TConditionType>
		void ForwardRecursionBone(SHORT InBoneIndex, _TCustomType InValue, const _TLambdaType& InFunc, const _TConditionType& InCond)
		{
			if ((InBoneIndex < 0) || (!RawSkeletonPtr) || (((SHORT)(RawSkeletonPtr->GetBoneCount())) <= InBoneIndex))
			{
				return;
			}
			const EBoneData& CurrentBoneData = (RawSkeletonPtr->GetBonePart())[InBoneIndex];
			if (InCond(CurrentBoneData, InValue))
			{
				_TCustomType NewValue = InFunc(CurrentBoneData, InValue);
				ForwardRecursionBone<_TCustomType, _TLambdaType, _TConditionType>(CurrentBoneData.Parent, NewValue, InFunc, InCond);
			}
		}
		template<typename _TCustomType, typename _TLambdaType, typename _TConditionType>
		void ForwardRecursionBone(SHORT InBoneIndex, _TCustomType InValue, const _TLambdaType& InFunc, const _TConditionType& InCond)const
		{
			if ((InBoneIndex < 0) || (!RawSkeletonPtr) || (((SHORT)(RawSkeletonPtr->GetBoneCount())) <= InBoneIndex))
			{
				return;
			}
			const EBoneData& CurrentBoneData = (RawSkeletonPtr->GetBonePart())[InBoneIndex];
			if (InCond(CurrentBoneData, InValue))
			{
				_TCustomType NewValue = InFunc(CurrentBoneData, InValue);
				ForwardRecursionBone<_TCustomType, _TLambdaType, _TConditionType>(CurrentBoneData.Parent, NewValue, InFunc, InCond);
			}
		}
		template<typename _TLambdaType, typename _TConditionType>
		void ForwardRecursionBone(SHORT InBoneIndex, const _TLambdaType& InFunc, const _TConditionType& InCond)
		{
			if ((InBoneIndex < 0) || (!RawSkeletonPtr) || (((SHORT)(RawSkeletonPtr->GetBoneCount())) <= InBoneIndex))
			{
				return;
			}
			const EBoneData& CurrentBoneData = (RawSkeletonPtr->GetBonePart())[InBoneIndex];
			if (InCond(CurrentBoneData))
			{
				InFunc(CurrentBoneData);
				ForwardRecursionBone<_TLambdaType, _TConditionType>(CurrentBoneData.Parent, InFunc, InCond);
			}
		}
		template<typename _TLambdaType, typename _TConditionType>
		void ForwardRecursionBone(SHORT InBoneIndex, const _TLambdaType& InFunc, const _TConditionType& InCond)const
		{
			if ((InBoneIndex < 0) || (!RawSkeletonPtr) || (((SHORT)(RawSkeletonPtr->GetBoneCount())) <= InBoneIndex))
			{
				return;
			}
			const EBoneData& CurrentBoneData = (RawSkeletonPtr->GetBonePart())[InBoneIndex];
			if (InCond(CurrentBoneData))
			{
				InFunc(CurrentBoneData);
				ForwardRecursionBone<_TLambdaType, _TConditionType>(CurrentBoneData.Parent, InFunc, InCond);
			}
		}
	private:
		const ESkeleton*		RawSkeletonPtr;
		EBoneTransform			SkeletonRootTransform;
		TArray<EBoneTransform>	BoneRelativeTransforms;
#if (_USE_MATRIX_FOR_BONE_TO_ROOT)
		TArray<Matrix4x4>		BoneToRootTransforms;
#else
		TArray<EBoneTransform>	BoneToRootTransforms;
#endif
	public:
		ESkeletonBoneMemoryPool();
		ESkeletonBoneMemoryPool(const ESkeletonBoneMemoryPool& Other);
		virtual ~ESkeletonBoneMemoryPool();
		ESkeletonBoneMemoryPool& operator=(const ESkeletonBoneMemoryPool& Other);
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