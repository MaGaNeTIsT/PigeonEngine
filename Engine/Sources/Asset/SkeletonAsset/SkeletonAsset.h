#pragma once

#include <CoreMinimal.h>
#include <Base/DataStructure/Container/Array.h>
#include <Base/DataStructure/Container/Map.h>
#include <Base/DataStructure/Text/String.h>
#include <BaseAsset.h>

namespace PigeonEngine
{

	struct EBoneData
	{
		EBoneData()noexcept
			: Index(-2), Name(ENGINE_BONE_DEFAULT_NAME), DefaultPosition(Vector3::Zero())
			, DefaultRotation(Quaternion::Identity()), DefaultScaling(Vector3::One())
			, BindPoseMatrix(Matrix4x4::Identity()), Parent(-2)
		{
		}
		EBoneData(const EString& InName)noexcept
			: Index(-2), Name(InName), DefaultPosition(Vector3::Zero())
			, DefaultRotation(Quaternion::Identity()), DefaultScaling(Vector3::One())
			, BindPoseMatrix(Matrix4x4::Identity()), Parent(-2)
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
		Matrix4x4		BindPoseMatrix;
		SHORT			Parent;
		TArray<SHORT>	Children;
	};

	class ESkeleton : public EObjectBase
	{

		EClass(ESkeleton, EObjectBase)

	public:
		typedef TArray<EBoneData>	EBonePart;
	public:
		ESkeleton(const EString& InSkeletonName);
		virtual ~ESkeleton();
		virtual void Release();
	public:
		BOOL	AddBoneElement(EBoneData* InIndexData);
		BOOL	RemoveBoneElement(const EString& InBoneName);
		BOOL	RemoveBoneElement(USHORT InBoneIndex, EString* OutBoneName = nullptr);
		BOOL	GetBoneElement(const EString& InBoneName, const EBoneData*& OutBoneData)const;
		BOOL	GetBoneElement(USHORT InBoneIndex, const EBoneData*& OutBoneData)const;
	protected:
		EString					SkeletonName;
		EBonePart				Bones;
		TMap<EString, USHORT>	BoneMapping;
	public:
		ESkeleton() = delete;

		CLASS_REMOVE_COPY_BODY(ESkeleton)
	};

};