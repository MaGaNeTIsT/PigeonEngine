#pragma once

#include <CoreMinimal.h>
#include <Base/DataStructure/Text/String.h>
#include <Base/DataStructure/Container/Array.h>
#include <Base/DataStructure/Container/Map.h>
#include <RenderCommon.h>
#include <MeshAsset/MeshAsset.h>

namespace PigeonEngine
{
	class CAssimpManager : public EManagerBase
	{

		EClass(CAssimpManager, EManagerBase)

	public:
		enum CAssimpReadFileState
		{
			ASSIMP_READ_FILE_STATE_SUCCEED,
			ASSIMP_READ_FILE_STATE_FAILED,
			ASSIMP_READ_FILE_STATE_ERROR
		};
	public:
		virtual void Initialize()override;
		virtual void ShutDown()override;
	public:
		CAssimpReadFileState ReadStaticMeshFile(const EString& path, EStaticMesh& OutMesh);
		CAssimpReadFileState ReadSkeletonMeshAndBoneFile(const EString& path, EMesh::ESubmeshPart& subMesh, UINT& vertexStride, CHAR*& vertices, UINT& numVertices, TArray<UINT>& indices, UINT& numIndices, ESkeletonInfo& skeleton, TMap<EString, SHORT>& boneIndexMap, TArray<USHORT>& boneList);
		CAssimpReadFileState ReadSkeletonAnimationFile(const EString& path, TMap<EString, TMap<EString, EAnimationInfo>>& animationDatas);

		CLASS_MANAGER_VIRTUAL_SINGLETON_BODY(CAssimpManager)

	};
};