#pragma once

#include <CoreMinimal.h>
#include <Base/DataStructure/Container/Array.h>
#include <Base/DataStructure/Container/Map.h>
#include <Base/DataStructure/Text/String.h>
#include <BaseAsset.h>

namespace PigeonEngine
{

	struct EBoneNode
	{
		EBoneNode()noexcept
			: Index(-2), Name(ENGINE_BONE_DEFAULT_NAME), DefaultPosition(Vector3::Zero())
			, DefaultRotation(Quaternion::Identity()), DefaultScaling(Vector3::One())
			, BindPoseMatrix(Matrix4x4::Identity()), Parent(-2)
		{
		}
		EBoneNode(const EString& InName)noexcept
			: Index(-2), Name(InName), DefaultPosition(Vector3::Zero())
			, DefaultRotation(Quaternion::Identity()), DefaultScaling(Vector3::One())
			, BindPoseMatrix(Matrix4x4::Identity()), Parent(-2)
		{
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
		ESkeleton(const EString& InSkeletonName);
		virtual ~ESkeleton();
		virtual void Release();

	protected:
		EString					SkeletonName;
		TArray<EBoneNode>		BoneNodes;
		TMap<EString,UINT>		BoneMaps;
	public:
		ESkeleton() = delete;

		CLASS_REMOVE_COPY_BODY(ESkeleton)
	};

};