#pragma once

#include "../../../Sources/EngineCore/Main/Main.h"
#include <Base/DataStructure/BuiltInType.h>
#include <Base/DataStructure/BaseType.h>
#include <Base/DataStructure/Text/String.h>
#include <Base/DataStructure/Container/Array.h>
#include <Base/DataStructure/ObjectBase.h>
#include <RenderCommon.h>
#include <MeshAsset/MeshAsset.h>
#include <SkeletonAsset/SkeletonAsset.h>
#include <AnimationAsset/AnimationAsset.h>

namespace PigeonEngine
{
	class CAssimpManager : public EManagerBase
	{
	public:
		enum CReadFileStateType : UINT8
		{
			ASSIMP_READ_FILE_STATE_SUCCEED,
			ASSIMP_READ_FILE_STATE_FAILED,
			ASSIMP_READ_FILE_STATE_ERROR
		};
	public:
		virtual void Initialize()override;
		virtual void ShutDown()override;
	public:
		CReadFileStateType ReadStaticMeshFile(const EString& InPath, TArray<EStaticMesh>& OutMeshes);
		CReadFileStateType ReadSkeletonFile(const EString& InPath, ESkeleton& OutSkeleton);
		CReadFileStateType ReadSkinnedMeshFile(const EString& InPath, TArray<ESkinnedMesh>& OutMeshes);
		CReadFileStateType ReadSkeletonAnimationFile(const EString& path, TArray<ESkeletonAnimationClip>& OutSkeletonAnimationClips);

		CLASS_MANAGER_SINGLETON_BODY(CAssimpManager)

	};
};