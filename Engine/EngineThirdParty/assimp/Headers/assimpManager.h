#pragma once

#include <CoreMinimal.h>
#include <Base/DataStructure/Text/String.h>
#include <Base/DataStructure/Container/Array.h>
#include <Base/DataStructure/Container/Map.h>
#include <RenderCommon.h>
#include <MeshAsset/MeshAsset.h>

class aiScene;
class aiMesh;

namespace PigeonEngine
{
	class CAssimpManager : public EManagerBase
	{

		EClass(CAssimpManager, EManagerBase)

	public:
		enum CReadFileStateType
		{
			ASSIMP_READ_FILE_STATE_SUCCEED,
			ASSIMP_READ_FILE_STATE_FAILED,
			ASSIMP_READ_FILE_STATE_ERROR
		};
	private:
		struct StoredMeshLayoutDesc
		{
			EVertexLayoutType	BaseVertexLayout;
			UINT				TryStoredLayoutNum;
			UINT				TryStoredLayoutSlot[EMesh::MeshVertexLayoutPartMaxNum];
		};
		BOOL FindMeshesAndVertexLayouts(const aiScene* InScene, TArray<const aiMesh*>& OutMeshes, TArray<TArray<RShaderSemanticType>>& OutLayouts, TArray<BOOL>& OutIsSkeletonMesh);

		template<typename TDataType>
		void TryAddStaticMeshVertexPart(const TArray<const TDataType*>& InDatas, const TArray<UINT>& InDataElementNum, const EVertexLayoutType InStoredLayoutBaseType, const UINT* InStoredLayoutSlots, const UINT InStoredLayoutNum, const UINT InStrideInBytes, const UINT InSuccessAddMaxNum, EStaticMesh& OutMesh);
		TArray<StoredMeshLayoutDesc> GetShouldStoredMeshLayoutDescriptions(const RShaderSemanticType* InLayouts, const UINT InLayoutNum);
		void TranslateAssimpMeshToEngineMeshInternal(const RShaderSemanticType* InEngineLayouts, const UINT InEngineLayoutNum, const TArray<const aiMesh*>& InMeshes, TArray<EStaticMesh>& OutMeshes);
	public:
		virtual void Initialize()override;
		virtual void ShutDown()override;
	public:
		CReadFileStateType ReadStaticMeshFile(const EString& path, TArray<EStaticMesh>& OutMeshes);
		//CReadFileStateType ReadSkeletonMeshAndBoneFile(const EString& path, EMesh::ESubmeshPart& subMesh, UINT& vertexStride, CHAR*& vertices, UINT& numVertices, TArray<UINT>& indices, UINT& numIndices, ESkeletonInfo& skeleton, TMap<EString, SHORT>& boneIndexMap, TArray<USHORT>& boneList);
		//CReadFileStateType ReadSkeletonAnimationFile(const EString& path, TMap<EString, TMap<EString, EAnimationInfo>>& animationDatas);

		CLASS_MANAGER_VIRTUAL_SINGLETON_BODY(CAssimpManager)

	};
};