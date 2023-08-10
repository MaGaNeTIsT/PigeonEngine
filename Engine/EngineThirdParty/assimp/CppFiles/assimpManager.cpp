#include "../Headers/assimpManager.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace PigeonEngine
{
#define CUSTOM_IS_QNAN(f)	(f != f)

	static Assimp::Importer* _GAssetImporter = nullptr;

	ENGINE_INLINE static Quaternion AssimpTranslateQuaternion(const aiQuaternion& v)
	{
		return (Quaternion(v.x, v.y, v.z, v.w));
	}

	ENGINE_INLINE static Vector3 AssimpTranslateVector3(const aiVector3D& v)
	{
		return (Vector3(v.x, v.y, v.z));
	}

	ENGINE_INLINE static void AssimpTranslateMatrix(const aiMatrix4x4& m, Vector3& pos, Quaternion& rot, Vector3& scl)
	{
		aiVector3D aiScl, aiPos; aiQuaternion aiRot;
		m.Decompose(aiScl, aiRot, aiPos);
		pos = AssimpTranslateVector3(aiPos);
		scl = AssimpTranslateVector3(aiScl);
		rot = AssimpTranslateQuaternion(aiRot);
	}

	ENGINE_INLINE static Matrix4x4 AssimpTranslateMatrix(const aiMatrix4x4& m, const BOOL& useOrigin = TRUE)
	{
		if (useOrigin)
		{
			return (Matrix4x4(DirectX::XMMATRIX(&(m.a1))));
		}
		else
		{
			aiVector3D scl, pos; aiQuaternion rot;
			m.Decompose(scl, rot, pos);
			return (Matrix4x4(
				DirectX::XMMatrixTranspose(
					DirectX::XMMatrixAffineTransformation(
						DirectX::XMVectorSet(scl.x, scl.y, scl.z, 1.f),
						DirectX::XMVectorSet(0.f, 0.f, 0.f, 0.f),
						DirectX::XMVectorSet(rot.x, rot.y, rot.z, rot.w),
						DirectX::XMVectorSet(pos.x, pos.y, pos.z, 1.f)))));
		}
	}

	ENGINE_INLINE static EString AssimpTranslateString(const aiString& s)
	{
		return (EString(s.C_Str()));
	}

	ENGINE_INLINE static aiString AssimpTranslateString(const EString& s)
	{
		return (aiString(*s));
	}

	struct _GVertexSemanticName
	{
		_GVertexSemanticName() { Exist = FALSE; Offset = 0u; }
		BOOL	Exist;
		UINT	Offset;
	};

	struct _GMeshAssetImporterInfo
	{
		_GMeshAssetImporterInfo() { NumIndices = 0u; NumVertices = 0u; VertexStride = 0u; }
		EString				Name;
		UINT					NumIndices;
		UINT					NumVertices;
		UINT					VertexStride;
		_GVertexSemanticName	VertexColor;
		_GVertexSemanticName	VertexPosition;
		_GVertexSemanticName	VertexNormal;
		_GVertexSemanticName	VertexTangent;
		_GVertexSemanticName	VertexUV;
		_GVertexSemanticName	VertexBoneIndices;
		_GVertexSemanticName	VertexBoneWeight;
	};

	//void _GTranslateMeshDesc(const aiScene* scene, TArray<ESubmeshData>& subMesh, TArray<_GMeshAssetImporterInfo>& sceneMeshesInfo, UINT& totalNumVertices, UINT& totalNumIndices, const CustomStruct::CRenderInputLayoutDesc* inputLayoutDesc, const UINT& inputLayoutNum)
	//{
	//	if (subMesh.size() > 0)
	//	{
	//		subMesh.clear();
	//	}
	//	if (sceneMeshesInfo.size() > 0)
	//	{
	//		sceneMeshesInfo.clear();
	//	}
	//	totalNumVertices = 0u;
	//	totalNumIndices = 0u;

	//	if (scene == nullptr || !scene->HasMeshes())
	//	{
	//		return;
	//	}

	//	UINT numMeshes = scene->mNumMeshes;
	//	for (UINT indexMesh = 0u; indexMesh < numMeshes; indexMesh++)
	//	{
	//		const aiMesh* tempMesh = scene->mMeshes[indexMesh];
	//		if (tempMesh == nullptr)
	//		{
	//			continue;
	//		}

	//		_GMeshAssetImporterInfo tempPerMeshInfo;
	//		tempPerMeshInfo.Name = tempMesh->mName.C_Str();
	//		tempPerMeshInfo.NumVertices = tempMesh->mNumVertices;
	//		tempPerMeshInfo.NumIndices = tempMesh->mNumFaces * 3u;

	//		for (UINT i = 0u; i < inputLayoutNum; i++)
	//		{
	//			CustomStruct::CRenderInputLayoutDesc desc(inputLayoutDesc[i]);
	//			if (desc.SemanticName == CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_COLOR)
	//			{
	//				tempPerMeshInfo.VertexColor.Exist = TRUE;
	//				tempPerMeshInfo.VertexColor.Offset = tempPerMeshInfo.VertexStride;
	//			}
	//			else if (desc.SemanticName == CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_POSITION)
	//			{
	//				tempPerMeshInfo.VertexPosition.Exist = TRUE;
	//				tempPerMeshInfo.VertexPosition.Offset = tempPerMeshInfo.VertexStride;
	//			}
	//			else if (desc.SemanticName == CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_NORMAL)
	//			{
	//				tempPerMeshInfo.VertexNormal.Exist = TRUE;
	//				tempPerMeshInfo.VertexNormal.Offset = tempPerMeshInfo.VertexStride;
	//			}
	//			else if (desc.SemanticName == CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_TANGENT)
	//			{
	//				tempPerMeshInfo.VertexTangent.Exist = TRUE;
	//				tempPerMeshInfo.VertexTangent.Offset = tempPerMeshInfo.VertexStride;
	//			}
	//			else if (desc.SemanticName == CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_TEXCOORD)
	//			{
	//				tempPerMeshInfo.VertexUV.Exist = TRUE;
	//				tempPerMeshInfo.VertexUV.Offset = tempPerMeshInfo.VertexStride;
	//			}
	//			else if (desc.SemanticName == CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_BLENDINDICES)
	//			{
	//				tempPerMeshInfo.VertexBoneIndices.Exist = TRUE;
	//				tempPerMeshInfo.VertexBoneIndices.Offset = tempPerMeshInfo.VertexStride;
	//			}
	//			else if (desc.SemanticName == CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_BLENDWEIGHT)
	//			{
	//				tempPerMeshInfo.VertexBoneWeight.Exist = TRUE;
	//				tempPerMeshInfo.VertexBoneWeight.Offset = tempPerMeshInfo.VertexStride;
	//			}
	//			tempPerMeshInfo.VertexStride += desc.GetSemanticSizeByByte();
	//		}

	//		sceneMeshesInfo.push_back(tempPerMeshInfo);
	//	}

	//	subMesh.resize(sceneMeshesInfo.size());
	//	UINT indexSubMesh = 0u;
	//	for (UINT indexMesh = 0u; indexMesh < numMeshes; indexMesh++)
	//	{
	//		if (scene->mMeshes[indexMesh] == nullptr)
	//		{
	//			continue;
	//		}

	//		CustomStruct::CSubMeshInfo& subMeshInfo = subMesh[indexSubMesh];
	//		_GMeshAssetImporterInfo& sceneMeshInfo = sceneMeshesInfo[indexSubMesh];

	//		subMeshInfo.IndexStart = totalNumIndices;
	//		subMeshInfo.IndexCount = sceneMeshInfo.NumIndices;
	//		subMeshInfo.VertexStart = totalNumVertices;
	//		subMeshInfo.VertexCount = sceneMeshInfo.NumVertices;

	//		totalNumVertices += sceneMeshInfo.NumVertices;
	//		totalNumIndices += sceneMeshInfo.NumIndices;

	//		indexSubMesh += 1u;
	//	}
	//}

	//void _GTranslateMeshVertexData(const aiMesh* mesh, const _GMeshAssetImporterInfo& assetInfo, const ESubmeshData& subMeshInfo, CHAR*& verticesData, TArray<UINT>& indicesData, TArray<CustomStruct::CGameBoneNodeInfo>& allGameNodes, const TMap<EString, SHORT>& allGameNodeIndices, TArray<SHORT>& allBoneList)
	//{
	//	{
	//		const UINT numFaces = mesh->mNumFaces;
	//		aiFace* faces = mesh->mFaces;
	//		for (UINT indexFace = 0u; indexFace < numFaces; indexFace++)
	//		{
	//			aiFace& tempFace = faces[indexFace];
	//			for (UINT i = 0u; i < 3u; i++)
	//			{
	//				indicesData[subMeshInfo.IndexStart + (indexFace * 3u + i)] = tempFace.mIndices[i];
	//			}
	//		}
	//	}

	//	{
	//		const UINT	numVertices = mesh->mNumVertices;

	//		TArray<TArray<std::pair<USHORT, FLOAT>>> blendIndicesWeights;
	//		UINT* blendWriteIndex = nullptr;

	//		if (allGameNodes.size() != 0 && allGameNodeIndices.size() != 0 && allGameNodes.size() == allGameNodeIndices.size())
	//		{
	//			blendWriteIndex = new UINT[numVertices];
	//			blendIndicesWeights.resize(numVertices);

	//			auto _FindBoneListIndex = [](const EString& boneName, const TMap<EString, SHORT>& allNodeIndices)->SHORT {
	//				auto itFindName = allNodeIndices.find(boneName);
	//				if (itFindName != allNodeIndices.end())
	//				{
	//					return (itFindName->second);
	//				}
	//				return -1; };
	//			auto _GetBoneIndex = [](const SHORT& indexInAllGameNodes, TArray<SHORT>& boneList)->USHORT {
	//				USHORT boneListSize = static_cast<USHORT>(boneList.size());
	//				for (USHORT i = 0u; i < boneListSize; i++)
	//				{
	//					if (indexInAllGameNodes == boneList[i])
	//					{
	//						return i;
	//					}
	//				}
	//				boneList.push_back(indexInAllGameNodes);
	//				return (static_cast<USHORT>(boneList.size() - 1));
	//			};
	//			auto _SortBoneIndicesAndWeight = [](const UINT& num, TArray<std::pair<USHORT, FLOAT>>& inputIndicesWeights) {
	//				BOOL needLoop = TRUE;
	//				while (needLoop == TRUE)
	//				{
	//					BOOL isModify = FALSE;
	//					for (UINT index = 1u; index < num; index++)
	//					{
	//						if (inputIndicesWeights[index].second > inputIndicesWeights[index - 1].second)
	//						{
	//							USHORT indexOfSmaller = inputIndicesWeights[index - 1].first;
	//							FLOAT weightOfSmaller = inputIndicesWeights[index - 1].second;
	//							inputIndicesWeights[index - 1] = std::pair<USHORT, FLOAT>(inputIndicesWeights[index].first, inputIndicesWeights[index].second);
	//							inputIndicesWeights[index] = std::pair<USHORT, FLOAT>(indexOfSmaller, weightOfSmaller);
	//							isModify |= TRUE;
	//						}
	//					}
	//					needLoop = isModify;
	//				}};

	//			for (UINT indexVertex = 0u; indexVertex < numVertices; indexVertex++)
	//			{
	//				blendWriteIndex[indexVertex] = 0u;
	//			}
	//			const UINT boneNum = mesh->mNumBones;
	//			for (UINT boneIndex = 0u; boneIndex < boneNum; boneIndex++)
	//			{
	//				const aiBone* bone = mesh->mBones[boneIndex];
	//				if (bone == nullptr || bone->mName.length == 0u)
	//				{
	//					continue;
	//				}
	//				SHORT tempFindBoneIndex = _FindBoneListIndex(_GTranslateString(bone->mName), allGameNodeIndices);
	//				if (tempFindBoneIndex < 0)
	//				{
	//					continue;
	//				}

	//				// Save bind pose matrix of bone.
	//				allGameNodes[tempFindBoneIndex].BindPoseMatrix = _GTranslateMatrix(bone->mOffsetMatrix, TRUE);
	//				// [Get new bone list index] and [save bone index] that will be used for rendering.
	//				USHORT tempBoneIndex = _GetBoneIndex(tempFindBoneIndex, allBoneList);

	//				for (UINT boneWeightIndex = 0u; boneWeightIndex < bone->mNumWeights; boneWeightIndex++)
	//				{
	//					const aiVertexWeight& tempBoneWeight = bone->mWeights[boneWeightIndex];
	//					blendIndicesWeights[tempBoneWeight.mVertexId].push_back(std::pair<USHORT, FLOAT>(tempBoneIndex, tempBoneWeight.mWeight));
	//					blendWriteIndex[tempBoneWeight.mVertexId] += 1u;
	//				}
	//			}
	//			for (UINT vertexIndex = 0u; vertexIndex < numVertices; vertexIndex++)
	//			{
	//				if (blendWriteIndex[vertexIndex] <= 4u)
	//				{
	//					continue;
	//				}
	//				_SortBoneIndicesAndWeight(blendWriteIndex[vertexIndex], blendIndicesWeights[vertexIndex]);
	//			}
	//		}

	//		for (UINT indexVertex = 0u; indexVertex < numVertices; indexVertex++)
	//		{
	//			CHAR* tempVertex = &(verticesData[assetInfo.VertexStride * (indexVertex + subMeshInfo.VertexStart)]);

	//			{
	//				FLOAT* tempColor = (FLOAT*)(&(tempVertex[assetInfo.VertexColor.Offset]));
	//				if (assetInfo.VertexColor.Exist)
	//				{
	//					if (mesh->HasVertexColors(0u))
	//					{
	//						aiColor4D& tempSceneMeshColor = (mesh->mColors[0])[indexVertex];
	//						tempColor[0] = tempSceneMeshColor.r;
	//						tempColor[1] = tempSceneMeshColor.g;
	//						tempColor[2] = tempSceneMeshColor.b;
	//						tempColor[3] = tempSceneMeshColor.a;
	//					}
	//					else
	//					{
	//						tempColor[0] = 0.5f;
	//						tempColor[1] = 0.5f;
	//						tempColor[2] = 0.5f;
	//						tempColor[3] = 1.f;
	//					}
	//				}
	//			}

	//			{
	//				FLOAT* tempPosition = (FLOAT*)(&(tempVertex[assetInfo.VertexPosition.Offset]));
	//				if (assetInfo.VertexPosition.Exist)
	//				{
	//					if (mesh->HasPositions())
	//					{
	//						aiVector3D& tempSceneMeshPos = mesh->mVertices[indexVertex];
	//						tempPosition[0] = tempSceneMeshPos.x;
	//						tempPosition[1] = tempSceneMeshPos.y;
	//						tempPosition[2] = tempSceneMeshPos.z;
	//						tempPosition[3] = 1.f;
	//					}
	//					else
	//					{
	//						tempPosition[0] = 0.f;
	//						tempPosition[1] = 0.f;
	//						tempPosition[2] = 0.f;
	//						tempPosition[3] = 1.f;
	//					}
	//				}
	//			}

	//			{
	//				FLOAT* tempNormal = (FLOAT*)(&(tempVertex[assetInfo.VertexNormal.Offset]));
	//				if (assetInfo.VertexNormal.Exist)
	//				{
	//					if (mesh->HasNormals())
	//					{
	//						aiVector3D& tempSceneMeshNormal = mesh->mNormals[indexVertex];
	//						tempNormal[0] = tempSceneMeshNormal.x;
	//						tempNormal[1] = tempSceneMeshNormal.y;
	//						tempNormal[2] = tempSceneMeshNormal.z;
	//						tempNormal[3] = 0.f;
	//					}
	//					else
	//					{
	//						tempNormal[0] = 0.f;
	//						tempNormal[1] = 1.f;
	//						tempNormal[2] = 0.f;
	//						tempNormal[3] = 0.f;
	//					}
	//				}
	//			}

	//			{
	//				FLOAT* tempTangent = (FLOAT*)(&(tempVertex[assetInfo.VertexTangent.Offset]));
	//				if (assetInfo.VertexTangent.Exist)
	//				{
	//					if (mesh->HasTangentsAndBitangents())
	//					{
	//						aiVector3D& tempSceneMeshTangent = mesh->mTangents[indexVertex];
	//						tempTangent[0] = tempSceneMeshTangent.x;
	//						tempTangent[1] = tempSceneMeshTangent.y;
	//						tempTangent[2] = tempSceneMeshTangent.z;
	//						tempTangent[3] = 0.f;
	//					}
	//					else
	//					{
	//						tempTangent[0] = 1.f;
	//						tempTangent[1] = 0.f;
	//						tempTangent[2] = 0.f;
	//						tempTangent[3] = 0.f;
	//					}
	//				}
	//			}

	//			{
	//				FLOAT* tempUV = (FLOAT*)(&(tempVertex[assetInfo.VertexUV.Offset]));
	//				if (assetInfo.VertexUV.Exist)
	//				{
	//					if (mesh->HasTextureCoords(0u))
	//					{
	//						aiVector3D& tempSceneMeshUV = (mesh->mTextureCoords[0])[indexVertex];
	//						tempUV[0] = tempSceneMeshUV.x;
	//						tempUV[1] = tempSceneMeshUV.y;
	//					}
	//					else
	//					{
	//						tempUV[0] = 0.f;
	//						tempUV[1] = 0.f;
	//					}
	//				}
	//			}

	//			{
	//				USHORT* tempBoneIndices = (USHORT*)(&(tempVertex[assetInfo.VertexBoneIndices.Offset]));
	//				if (assetInfo.VertexBoneIndices.Exist)
	//				{
	//					if (blendWriteIndex != nullptr && blendIndicesWeights.size() > 0)
	//					{
	//						TArray<std::pair<USHORT, FLOAT>>& tempSceneMeshBoneIndices = blendIndicesWeights[indexVertex];
	//						UINT& tempSceneMeshBoneWriteIndex = blendWriteIndex[indexVertex];
	//						tempBoneIndices[0] = tempSceneMeshBoneWriteIndex > 0u ? tempSceneMeshBoneIndices[0].first : 0u;
	//						tempBoneIndices[1] = tempSceneMeshBoneWriteIndex > 1u ? tempSceneMeshBoneIndices[1].first : 0u;
	//						tempBoneIndices[2] = tempSceneMeshBoneWriteIndex > 2u ? tempSceneMeshBoneIndices[2].first : 0u;
	//						tempBoneIndices[3] = tempSceneMeshBoneWriteIndex > 3u ? tempSceneMeshBoneIndices[3].first : 0u;
	//					}
	//					else
	//					{
	//						tempBoneIndices[0] = 0u;
	//						tempBoneIndices[1] = 0u;
	//						tempBoneIndices[2] = 0u;
	//						tempBoneIndices[3] = 0u;
	//					}
	//				}
	//			}

	//			{
	//				FLOAT* tempBoneWeights = (FLOAT*)(&(tempVertex[assetInfo.VertexBoneWeight.Offset]));
	//				if (assetInfo.VertexBoneWeight.Exist)
	//				{
	//					if (blendWriteIndex != nullptr && blendIndicesWeights.size() > 0)
	//					{
	//						TArray<std::pair<USHORT, FLOAT>>& tempSceneMeshBoneWeights = blendIndicesWeights[indexVertex];
	//						UINT& tempSceneMeshBoneWriteIndex = blendWriteIndex[indexVertex];
	//						tempBoneWeights[0] = tempSceneMeshBoneWriteIndex > 0u ? tempSceneMeshBoneWeights[0].second : 1.f;
	//						tempBoneWeights[1] = tempSceneMeshBoneWriteIndex > 1u ? tempSceneMeshBoneWeights[1].second : 0.f;
	//						tempBoneWeights[2] = tempSceneMeshBoneWriteIndex > 2u ? tempSceneMeshBoneWeights[2].second : 0.f;
	//						tempBoneWeights[3] = tempSceneMeshBoneWriteIndex > 3u ? tempSceneMeshBoneWeights[3].second : 0.f;
	//					}
	//					else
	//					{
	//						tempBoneWeights[0] = 1.f;
	//						tempBoneWeights[1] = 0.f;
	//						tempBoneWeights[2] = 0.f;
	//						tempBoneWeights[3] = 0.f;
	//					}
	//				}
	//			}
	//		}

	//		if (blendWriteIndex != nullptr)
	//		{
	//			delete[]blendWriteIndex;
	//		}
	//	}
	//}

	//void _GTranslateDefaultMeshData(const aiScene* scene, TArray<ESubmeshData>& subMesh, UINT& vertexStride, CHAR*& vertices, UINT& numVertices, TArray<UINT>& indices, UINT& numIndices, const CustomStruct::CRenderInputLayoutDesc* inputLayoutDesc, const UINT& inputLayoutNum)
	//{
	//	vertexStride = 0u;
	//	numVertices = 0u;
	//	numIndices = 0u;
	//	if (vertices != nullptr)
	//	{
	//		delete[]vertices;
	//		vertices = nullptr;
	//	}
	//	if (indices.size() > 0)
	//	{
	//		indices.clear();
	//	}

	//	TArray<_GMeshAssetImporterInfo> sceneMeshesInfo;
	//	UINT totalNumVertices, totalNumIndices;
	//	_GTranslateMeshDesc(scene, subMesh, sceneMeshesInfo, totalNumVertices, totalNumIndices, inputLayoutDesc, inputLayoutNum);

	//	{
	//		vertexStride = sceneMeshesInfo[0].VertexStride;
	//		numVertices = totalNumVertices;
	//		numIndices = totalNumIndices;
	//		vertices = new CHAR[totalNumVertices * vertexStride];
	//		indices.resize(totalNumIndices);
	//	}

	//	TArray<CustomStruct::CGameBoneNodeInfo> tempSkeleton;
	//	const TMap<EString, SHORT> tempSkeletonNodeIndices;
	//	TArray<SHORT> tempBoneList;

	//	UINT numMeshes = scene->mNumMeshes;
	//	UINT indexSubMesh = 0u;
	//	for (UINT indexMesh = 0u; indexMesh < numMeshes; indexMesh++)
	//	{
	//		const aiMesh* tempMesh = scene->mMeshes[indexMesh];
	//		if (tempMesh == nullptr)
	//		{
	//			continue;
	//		}
	//		_GMeshAssetImporterInfo& sceneMeshInfo = sceneMeshesInfo[indexSubMesh];
	//		ESubmeshData& subMeshInfo = subMesh[indexSubMesh];
	//		_GTranslateMeshVertexData(tempMesh, sceneMeshInfo, subMeshInfo, vertices, indices, tempSkeleton, tempSkeletonNodeIndices, tempBoneList);
	//		indexSubMesh += 1u;
	//	}
	//}

	//void _GTranslateSkeletonMeshData(const aiScene* scene, TArray<ESubmeshData>& subMesh, UINT& vertexStride, CHAR*& vertices, UINT& numVertices, TArray<UINT>& indices, UINT& numIndices, TArray<CustomStruct::CGameBoneNodeInfo>& allGameNodes, const TMap<EString, SHORT>& allGameNodeIndices, TArray<USHORT>& allBoneList, const CustomStruct::CRenderInputLayoutDesc* inputLayoutDesc, const UINT& inputLayoutNum)
	//{
	//	vertexStride = 0u;
	//	numVertices = 0u;
	//	numIndices = 0u;
	//	if (vertices != nullptr)
	//	{
	//		delete[]vertices;
	//		vertices = nullptr;
	//	}
	//	if (indices.size() > 0)
	//	{
	//		indices.clear();
	//	}
	//	if (allBoneList.size() > 0)
	//	{
	//		allBoneList.clear();
	//	}

	//	TArray<_GMeshAssetImporterInfo> sceneMeshesInfo;
	//	UINT totalNumVertices, totalNumIndices;
	//	_GTranslateMeshDesc(scene, subMesh, sceneMeshesInfo, totalNumVertices, totalNumIndices, inputLayoutDesc, inputLayoutNum);

	//	{
	//		vertexStride = sceneMeshesInfo[0].VertexStride;
	//		numVertices = totalNumVertices;
	//		numIndices = totalNumIndices;
	//		vertices = new CHAR[totalNumVertices * vertexStride];
	//		indices.resize(totalNumIndices);
	//	}

	//	TArray<SHORT> tempBoneList;
	//	tempBoneList.push_back(-1);

	//	UINT numMeshes = scene->mNumMeshes;
	//	UINT indexSubMesh = 0u;
	//	for (UINT indexMesh = 0u; indexMesh < numMeshes; indexMesh++)
	//	{
	//		const aiMesh* tempMesh = scene->mMeshes[indexMesh];
	//		if (tempMesh == nullptr)
	//		{
	//			continue;
	//		}
	//		_GMeshAssetImporterInfo& sceneMeshInfo = sceneMeshesInfo[indexSubMesh];
	//		ESubmeshData& subMeshInfo = subMesh[indexSubMesh];
	//		_GTranslateMeshVertexData(tempMesh, sceneMeshInfo, subMeshInfo, vertices, indices, allGameNodes, allGameNodeIndices, tempBoneList);
	//		indexSubMesh += 1u;
	//	}

	//	if (tempBoneList.size() > 1)
	//	{
	//		allBoneList.resize(tempBoneList.size());
	//		allBoneList[0] = 0u;
	//		USHORT tempNumBoneList = static_cast<USHORT>(tempBoneList.size());
	//		for (USHORT boneIndex = 1u; boneIndex < tempNumBoneList; boneIndex++)
	//		{
	//			allBoneList[boneIndex] = static_cast<USHORT>(tempBoneList[boneIndex]);
	//		}
	//	}
	//}

	//void _GReadNodeTransformRecursion(const aiNode* node, TArray<CustomStruct::CGameBoneNodeInfo>& allGameNodes, const TMap<const aiNode*, EString>& allNodeNames, const TMap<EString, SHORT>& allNodeIndices, Matrix4x4 globalMatrix)
	//{
	//	if (node == nullptr)
	//	{
	//		return;
	//	}

	//	Matrix4x4 tempGlobalMatrix = _GTranslateMatrix(node->mTransformation, TRUE);
	//	{
	//		aiVector3D scl, pos; aiQuaternion rot;
	//		SHORT tempNodeIndex = allNodeIndices.find(allNodeNames.find(node)->second)->second;
	//		_GTranslateMatrix(node->mTransformation, allGameNodes[tempNodeIndex].DefaultPosition, allGameNodes[tempNodeIndex].DefaultRotation, allGameNodes[tempNodeIndex].DefaultScaling);
	//		tempGlobalMatrix = globalMatrix * tempGlobalMatrix;
	//	}

	//	for (UINT indexChild = 0u; indexChild < node->mNumChildren; indexChild++)
	//	{
	//		_GReadNodeTransformRecursion(node->mChildren[indexChild], allGameNodes, allNodeNames, allNodeIndices, tempGlobalMatrix);
	//	}
	//}

	//BOOL _GGatherSkeletonMeshAllNodeStructures(const aiScene* scene,
	//	TArray<ESubmeshData>& subMesh, UINT& vertexStride, CHAR*& vertices, UINT& numVertices, TArray<UINT>& indices, UINT& numIndices,
	//	TArray<CustomStruct::CGameBoneNodeInfo>& allGameNodes, TMap<EString, SHORT>& allGameNodeIndices, TArray<USHORT>& allBoneList,
	//	const CustomStruct::CRenderInputLayoutDesc* inputLayoutDesc, const UINT& inputLayoutNum)
	//{
	//	// Init all output data.
	//	{
	//		if (vertices != nullptr)
	//		{
	//			delete[]vertices;
	//			vertices = nullptr;
	//		}
	//		vertexStride = 0u;
	//		numVertices = 0u;
	//		numIndices = 0u;
	//		if (subMesh.size() > 0)
	//		{
	//			subMesh.clear();
	//		}
	//		if (indices.size() > 0)
	//		{
	//			indices.clear();
	//		}
	//		if (allGameNodes.size() > 0)
	//		{
	//			allGameNodes.clear();
	//		}
	//		if (allGameNodeIndices.size() > 0)
	//		{
	//			allGameNodeIndices.clear();
	//		}
	//		if (allBoneList.size() > 0)
	//		{
	//			allBoneList.clear();
	//		}
	//	}

	//	// Check scene pointer.
	//	if (scene == nullptr)
	//	{
	//		return FALSE;
	//	}

	//	// Read data structures of all nodes.
	//	TArray<const aiNode*> allNodes; TMap<const aiNode*, EString> allNodeNames; TMap<EString, SHORT> allNodeIndices;
	//	_GGatherSingleNodeRecursion(scene->mRootNode, allNodes, allNodeNames, allNodeIndices);

	//	// Check read state.
	//	if (allNodes.size() == 0 || allNodes.size() != allNodeIndices.size())
	//	{
	//		return FALSE;
	//	}

	//	// Create engine's hierarchical bones' data.
	//	{
	//		const UINT numAllNodes = static_cast<UINT>(allNodes.size());
	//		for (UINT indexNode = 0u; indexNode < numAllNodes; indexNode++)
	//		{
	//			auto itFindNodeName = allNodeNames.find(allNodes[indexNode]);
	//			CustomStruct::CGameBoneNodeInfo tempNewBone(itFindNodeName->second);
	//			tempNewBone.Index = static_cast<SHORT>(indexNode);
	//			tempNewBone.Parent = -1;
	//			if (allNodes[indexNode]->mParent != nullptr)
	//			{
	//				auto itFindNodeParentName = allNodeNames.find(allNodes[indexNode]->mParent);
	//				tempNewBone.Parent = allNodeIndices.find(itFindNodeParentName->second)->second;
	//			}
	//			allGameNodes.push_back(tempNewBone);
	//			allGameNodeIndices.insert_or_assign(tempNewBone.Name, tempNewBone.Index);
	//		}
	//	}

	//	// Read mesh data for skeleton mesh.
	//	_GTranslateSkeletonMeshData(scene, subMesh, vertexStride, vertices, numVertices, indices, numIndices, allGameNodes, allGameNodeIndices, allBoneList, inputLayoutDesc, inputLayoutNum);

	//	// Read skeleton default transform.
	//	Matrix4x4 dummyMat(Matrix4x4::Identity());
	//	_GReadNodeTransformRecursion(scene->mRootNode, allGameNodes, allNodeNames, allNodeIndices, dummyMat);

	//	// Save node's child relation.
	//	{
	//		const SHORT numAllGameNodes = static_cast<SHORT>(allGameNodes.size());
	//		for (SHORT indexNode = 0; indexNode < numAllGameNodes; indexNode++)
	//		{
	//			CustomStruct::CGameBoneNodeInfo& gameNode = allGameNodes[indexNode];
	//			if (indexNode != gameNode.Index || gameNode.Parent < 0 || gameNode.Parent >= numAllGameNodes)
	//			{
	//				continue;
	//			}
	//			CustomStruct::CGameBoneNodeInfo& gameParentNode = allGameNodes[gameNode.Parent];
	//			gameParentNode.Children.push_back(gameNode.Index);
	//		}
	//	}

	//	return TRUE;
	//}

	//void _GTranslateAnimationBehaviour(const aiAnimBehaviour& input, CustomStruct::CGameAnimationBehaviour& output)
	//{
	//	if (input == aiAnimBehaviour::aiAnimBehaviour_DEFAULT)
	//	{
	//		output = CustomStruct::CGameAnimationBehaviour::ANIMATION_BEHAVIOUR_DEFAULT;
	//	}
	//	else if (input == aiAnimBehaviour::aiAnimBehaviour_CONSTANT)
	//	{
	//		output = CustomStruct::CGameAnimationBehaviour::ANIMATION_BEHAVIOUR_CONSTANT;
	//	}
	//	else if (input == aiAnimBehaviour::aiAnimBehaviour_LINEAR)
	//	{
	//		output = CustomStruct::CGameAnimationBehaviour::ANIMATION_BEHAVIOUR_LINEAR;
	//	}
	//	else if (input == aiAnimBehaviour::aiAnimBehaviour_REPEAT)
	//	{
	//		output = CustomStruct::CGameAnimationBehaviour::ANIMATION_BEHAVIOUR_REPEAT;
	//	}
	//}

	//void _GGatherSingleAnimationDatas(const aiAnimation* animationNode, CustomStruct::CGameAnimationInfo& animationData)
	//{
	//	if (animationData.AnimationNodes.size() > 0)
	//	{
	//		animationData.AnimationNodes.clear();
	//	}

	//	UINT numChannels = animationNode->mNumChannels;
	//	for (UINT indexChannel = 0u; indexChannel < numChannels; indexChannel++)
	//	{
	//		const aiNodeAnim* tempChannel = animationNode->mChannels[indexChannel];
	//		if (tempChannel == nullptr || tempChannel->mNodeName.length < 1)
	//		{
	//			continue;
	//		}

	//		CustomStruct::CGameAnimationNodeInfo tempAnimationNodeInfo(_GTranslateString(tempChannel->mNodeName));

	//		for (UINT indexPosKey = 0u; indexPosKey < tempChannel->mNumPositionKeys && tempChannel->mPositionKeys != nullptr; indexPosKey++)
	//		{
	//			const aiVectorKey& tempChannelPosKey = tempChannel->mPositionKeys[indexPosKey];

	//			CustomStruct::CGameAnimationKey<Vector3> tempPosKey;
	//			tempPosKey.Time = tempChannelPosKey.mTime;
	//			tempPosKey.Value = _GTranslateVector3(tempChannelPosKey.mValue);

	//			tempAnimationNodeInfo.PositionKeys.push_back(tempPosKey);
	//		}

	//		for (UINT indexRotKey = 0u; indexRotKey < tempChannel->mNumRotationKeys && tempChannel->mRotationKeys != nullptr; indexRotKey++)
	//		{
	//			const aiQuatKey& tempChannelRotKey = tempChannel->mRotationKeys[indexRotKey];

	//			CustomStruct::CGameAnimationKey<Quaternion> tempRotKey;
	//			tempRotKey.Time = tempChannelRotKey.mTime;
	//			tempRotKey.Value = _GTranslateQuaternion(tempChannelRotKey.mValue);

	//			tempAnimationNodeInfo.RotationKeys.push_back(tempRotKey);
	//		}

	//		for (UINT indexSclKey = 0u; indexSclKey < tempChannel->mNumScalingKeys && tempChannel->mScalingKeys != nullptr; indexSclKey++)
	//		{
	//			const aiVectorKey& tempChannelSclKey = tempChannel->mScalingKeys[indexSclKey];

	//			CustomStruct::CGameAnimationKey<Vector3> tempSclKey;
	//			tempSclKey.Time = tempChannelSclKey.mTime;
	//			tempSclKey.Value = _GTranslateVector3(tempChannelSclKey.mValue);

	//			tempAnimationNodeInfo.ScalingKeys.push_back(tempSclKey);
	//		}

	//		_GTranslateAnimationBehaviour(tempChannel->mPreState, tempAnimationNodeInfo.PreState);
	//		_GTranslateAnimationBehaviour(tempChannel->mPostState, tempAnimationNodeInfo.PostState);

	//		// TODO is this necessary?
	//		{
	//			std::sort(tempAnimationNodeInfo.PositionKeys.begin(), tempAnimationNodeInfo.PositionKeys.end());
	//			std::sort(tempAnimationNodeInfo.RotationKeys.begin(), tempAnimationNodeInfo.RotationKeys.end());
	//			std::sort(tempAnimationNodeInfo.ScalingKeys.begin(), tempAnimationNodeInfo.ScalingKeys.end());
	//		}

	//		animationData.AnimationNodes.push_back(tempAnimationNodeInfo);
	//	}
	//}

	//BOOL _GGatherAllAnimationDatas(const EString& path, const aiScene* scene, TMap<EString, CustomStruct::CGameAnimationInfo>& animationDatas)
	//{
	//	if (animationDatas.size() > 0)
	//	{
	//		animationDatas.clear();
	//	}

	//	BOOL result = TRUE;

	//	UINT animationDummyIndex = 0u;
	//	UINT numAnimationNodes = scene->mNumAnimations;
	//	for (UINT indexAnimationNode = 0u; indexAnimationNode < numAnimationNodes; indexAnimationNode++)
	//	{
	//		const aiAnimation* tempAnimationNode = scene->mAnimations[indexAnimationNode];
	//		if (tempAnimationNode == nullptr)
	//		{
	//			continue;
	//		}

	//		EString tempAnimationNodeName;

	//		if (tempAnimationNode->mName.length > 0)
	//		{
	//			tempAnimationNodeName = _GTranslateString(tempAnimationNode->mName);
	//		}
	//		else
	//		{
	//			tempAnimationNodeName = _GConstDummyAnimationName + "_" + std::to_string(animationDummyIndex);
	//			animationDummyIndex += 1u;
	//		}

	//		{
	//			auto tempFindAnimation = animationDatas.find(tempAnimationNodeName);
	//			if (tempFindAnimation != animationDatas.end())
	//			{
	//				result = FALSE;
	//				continue;
	//			}
	//		}

	//		{
	//			auto tempCurrentAnimation = animationDatas.insert_or_assign(tempAnimationNodeName, CustomStruct::CGameAnimationInfo());
	//			CustomStruct::CGameAnimationInfo& tempAnimationData = tempCurrentAnimation.first->second;
	//			tempAnimationData.Name = path + "_" + tempAnimationNodeName;
	//			tempAnimationData.Duration = tempAnimationNode->mDuration;
	//			tempAnimationData.TicksPerSecond = tempAnimationNode->mTicksPerSecond;
	//			_GGatherSingleAnimationDatas(tempAnimationNode, tempAnimationData);
	//		}
	//	}

	//	return result;
	//}

	ENGINE_INLINE static void StoreVertexData(const aiVector2D* InOriginDatas, const UINT InOriginDataNum, const UINT InStoredDataStrideIn32Bits, FLOAT*& OutStoredDatas)
	{
		Check((ENGINE_ASSET_ERROR), ("Stored origin vertex data can not be null."), (!!InOriginDatas));
		Check((ENGINE_ASSET_ERROR), ("Number of stored origin vertex data is too small."), (InOriginDataNum >= 3u));
		Check((ENGINE_ASSET_ERROR), ("Stored vertex data's stride must be bigger than 2 in 32bits."), (InStoredDataStrideIn32Bits >= 2u));
		Check((ENGINE_ASSET_ERROR), ("Stored vertex data can not be null."), (!!OutStoredDatas));
		for (UINT i = 0u; i < InOriginDataNum; i++)
		{
			const aiVector2D& TempData = InOriginDatas[i];
			OutStoredDatas[i * InStoredDataStrideIn32Bits + 0u] = TempData.x;
			OutStoredDatas[i * InStoredDataStrideIn32Bits + 1u] = TempData.y;
		}
	}
	ENGINE_INLINE static void StoreVertexData(const aiVector3D* InOriginDatas, const UINT InOriginDataNum, const UINT InStoredDataStrideIn32Bits, FLOAT*& OutStoredDatas)
	{
		Check((ENGINE_ASSET_ERROR), ("Stored origin vertex data can not be null."), (!!InOriginDatas));
		Check((ENGINE_ASSET_ERROR), ("Number of stored origin vertex data is too small."), (InOriginDataNum >= 3u));
		Check((ENGINE_ASSET_ERROR), ("Stored vertex data's stride must be bigger than 2 in 32bits."), (InStoredDataStrideIn32Bits >= 2u));
		Check((ENGINE_ASSET_ERROR), ("Stored vertex data can not be null."), (!!OutStoredDatas));
		for (UINT i = 0u; i < InOriginDataNum; i++)
		{
			const aiVector3D& TempData = InOriginDatas[i];
			OutStoredDatas[i * InStoredDataStrideIn32Bits + 0u] = TempData.x;
			OutStoredDatas[i * InStoredDataStrideIn32Bits + 1u] = TempData.y;
			if (InStoredDataStrideIn32Bits > 2u)
			{
				OutStoredDatas[i * InStoredDataStrideIn32Bits + 2u] = TempData.z;
			}
		}
	}
	ENGINE_INLINE static void StoreVertexData(const aiColor4D* InOriginDatas, const UINT InOriginDataNum, const UINT InStoredDataStrideIn32Bits, FLOAT*& OutStoredDatas)
	{
		Check((ENGINE_ASSET_ERROR), ("Stored origin vertex data can not be null."), (!!InOriginDatas));
		Check((ENGINE_ASSET_ERROR), ("Number of stored origin vertex data is too small."), (InOriginDataNum >= 3u));
		Check((ENGINE_ASSET_ERROR), ("Stored vertex data's stride must be bigger than 2 in 32bits."), (InStoredDataStrideIn32Bits >= 2u));
		Check((ENGINE_ASSET_ERROR), ("Stored vertex data can not be null."), (!!OutStoredDatas));
		for (UINT i = 0u; i < InOriginDataNum; i++)
		{
			const aiColor4D& TempData = InOriginDatas[i];
			OutStoredDatas[i * InStoredDataStrideIn32Bits + 0u] = TempData.r;
			OutStoredDatas[i * InStoredDataStrideIn32Bits + 1u] = TempData.g;
			if (InStoredDataStrideIn32Bits > 2u)
			{
				OutStoredDatas[i * InStoredDataStrideIn32Bits + 2u] = TempData.b;
				OutStoredDatas[i * InStoredDataStrideIn32Bits + 3u] = TempData.a;
			}
		}
	}
	struct EReaderVertexWeights
	{
		EReaderVertexWeights()noexcept : MaxCount(0u)
		{
			for (UINT i = 0u; i < ESettings::ENGINE_BONE_WEIGHT_NUM_MAXIMUM; i++)
			{
				Indices[i] = 0u;
				Weights[i] = 0.f;
			}
		}
		USHORT	MaxCount;
		USHORT	Indices[ESettings::ENGINE_BONE_WEIGHT_NUM_MAXIMUM];
		FLOAT	Weights[ESettings::ENGINE_BONE_WEIGHT_NUM_MAXIMUM];
	};
	ENGINE_INLINE static void StoreSkinData(const TArray<EReaderVertexWeights>& InDatas, const UINT InMaxEffectNum, UINT*& OutIndices, FLOAT*& OutWeights)
	{
		Check((ENGINE_ASSET_ERROR), ("Stored origin skin data can not be null."), (InDatas.Length() > 0u));
		Check((ENGINE_ASSET_ERROR), ("Stored skin data's effect bone num must be bigger than 1."), (InMaxEffectNum >= 1u));
		Check((ENGINE_ASSET_ERROR), ("Stored indices data can not be null."), (!!OutIndices));
		Check((ENGINE_ASSET_ERROR), ("Stored weights data can not be null."), (!!OutWeights));
		for (UINT i = 0u, n = InDatas.Length(); i < n; i++)
		{
			const EReaderVertexWeights& TempData = InDatas[i];
			for (UINT EffectIndex = 0u; EffectIndex < InMaxEffectNum; EffectIndex++)
			{
				if (EffectIndex < TempData.MaxCount)
				{
					OutIndices[i * InMaxEffectNum + EffectIndex] = TempData.Indices[EffectIndex];
					OutWeights[i * InMaxEffectNum + EffectIndex] = TempData.Weights[EffectIndex];
				}
				else
				{
					OutIndices[i * InMaxEffectNum + EffectIndex] = 0u;
					OutWeights[i * InMaxEffectNum + EffectIndex] = 0.f;
				}
			}
		}
	}
	static void AssimpGatherSingleNodeRecursion(const aiNode* InCurrentNode, TArray<const aiNode*>& OutNodes, TMap<const aiNode*, EString>& OutNodeNames, TMap<EString, SHORT>& OutNodeIndices)
	{
		if (InCurrentNode == nullptr)
		{
			return;
		}
		OutNodes.Add(InCurrentNode);
		{
			SHORT NodeIndex = static_cast<SHORT>(OutNodes.Length() - 1u);
			EString TempNodeName;
			if (InCurrentNode->mName.length > 0)
			{
				TempNodeName = AssimpTranslateString(InCurrentNode->mName);
			}
			else
			{
				TempNodeName = ENGINE_DEFAULT_NAME;
			}
			EString TempOldName = TempNodeName + "_";
			EString TempNewName = TempNodeName;
			UINT tempSameNameIndex = 0u;
			while (OutNodeIndices.ContainsKey(TempNewName))
			{
				TempNewName = TempOldName + ToString(tempSameNameIndex);
				tempSameNameIndex += 1u;
			}
			OutNodeIndices.Add(TempNewName, NodeIndex);
			OutNodeNames.Add(InCurrentNode, TempNewName);
		}
		for (UINT ChildIndex = 0u; ChildIndex < InCurrentNode->mNumChildren; ChildIndex++)
		{
			AssimpGatherSingleNodeRecursion(InCurrentNode->mChildren[ChildIndex], OutNodes, OutNodeNames, OutNodeIndices);
		}
	}
	static void AssimpReadNodeDefaultTransformRecursion(const aiNode* InCurrentNode, const TMap<const aiNode*, EString>& InNodeNames, const TMap<EString, SHORT>& InNodeIndices, TArray<EBoneData>& OutBoneDatas)
	{
		if (InCurrentNode == nullptr)
		{
			return;
		}
		{
			EString TempNodeName;
			if (InNodeNames.FindValue(InCurrentNode, TempNodeName))
			{
				SHORT TempNodeIndex = -2;
				if (InNodeIndices.FindValue(TempNodeName, TempNodeIndex))
				{
					if (TempNodeIndex >= 0)
					{
						AssimpTranslateMatrix(InCurrentNode->mTransformation,
							OutBoneDatas[TempNodeIndex].DefaultPosition,
							OutBoneDatas[TempNodeIndex].DefaultRotation,
							OutBoneDatas[TempNodeIndex].DefaultScaling);
					}
				}
			}
		}
		for (UINT indexChild = 0u; indexChild < InCurrentNode->mNumChildren; indexChild++)
		{
			AssimpReadNodeDefaultTransformRecursion(InCurrentNode->mChildren[indexChild], InNodeNames, InNodeIndices, OutBoneDatas);
		}
	}
	CAssimpManager::CAssimpManager()
	{
#ifdef _EDITOR_ONLY
		DebugName = ENGINE_ASSIMP_MANAGER_NAME;
#endif
	}
	CAssimpManager::~CAssimpManager()
	{
	}
	BOOL CAssimpManager::FindMeshesAndVertexLayouts(const aiScene* InScene, TArray<const aiMesh*>& OutMeshes, TArray<TArray<RShaderSemanticType>>& OutLayouts, TArray<BOOL>& OutIsSkeletonMesh)
	{
		if (!(InScene->HasMeshes()))
		{
			return FALSE;
		}
		if (OutLayouts.Length() > 0u)
		{
			OutLayouts.Clear();
		}
		if (OutMeshes.Length() > 0u)
		{
			OutMeshes.Clear();
		}
		if (OutIsSkeletonMesh.Length() > 0u)
		{
			OutIsSkeletonMesh.Clear();
		}
		for (UINT i = 0u, n = InScene->mNumMeshes; i < n; i++)
		{
			const aiMesh* PerMesh = InScene->mMeshes[i];
			if (PerMesh && PerMesh->HasPositions() && PerMesh->HasFaces())
			{
				OutMeshes.Add(PerMesh);
			}
		}
		if (OutMeshes.Length() < 1u)
		{
			return FALSE;
		}
		for (UINT i = 0u, n = OutMeshes.Length(); i < n; i++)
		{
			OutLayouts.Add(TArray<RShaderSemanticType>());
			OutIsSkeletonMesh.Add(FALSE);
			const aiMesh* PerMesh = OutMeshes[i];
			TArray<RShaderSemanticType>& PerMeshLayout = OutLayouts[OutLayouts.Length() - 1u];
			if (PerMesh->HasPositions())
			{
				PerMeshLayout.Add(RShaderSemanticType::SHADER_SEMANTIC_POSITION);
			}
			if (PerMesh->HasNormals())
			{
				PerMeshLayout.Add(RShaderSemanticType::SHADER_SEMANTIC_NORMAL);
			}
			if (PerMesh->GetNumUVChannels() > 0)
			{
				UINT CurrentUV = 0u;
				for (UINT IndexUV = 0u, MaxUV = AI_MAX_NUMBER_OF_TEXTURECOORDS; IndexUV < MaxUV; IndexUV++)
				{
					if (PerMesh->HasTextureCoords(IndexUV))
					{
						switch (CurrentUV)
						{
						case 0u:
							PerMeshLayout.Add(RShaderSemanticType::SHADER_SEMANTIC_TEXCOORD0);
							break;
						case 1u:
							PerMeshLayout.Add(RShaderSemanticType::SHADER_SEMANTIC_TEXCOORD1);
							break;
						case 2u:
							PerMeshLayout.Add(RShaderSemanticType::SHADER_SEMANTIC_TEXCOORD2);
							break;
						case 3u:
							PerMeshLayout.Add(RShaderSemanticType::SHADER_SEMANTIC_TEXCOORD3);
							break;
						}
						CurrentUV++;
					}
				}
			}
			if (PerMesh->HasTangentsAndBitangents())
			{
				PerMeshLayout.Add(RShaderSemanticType::SHADER_SEMANTIC_TANGENT);
				PerMeshLayout.Add(RShaderSemanticType::SHADER_SEMANTIC_BINORMAL);
			}
			if (PerMesh->HasBones())
			{
				OutIsSkeletonMesh[OutIsSkeletonMesh.Length() - 1u] = TRUE;
				PerMeshLayout.Add(RShaderSemanticType::SHADER_SEMANTIC_BLENDINDICES);
				PerMeshLayout.Add(RShaderSemanticType::SHADER_SEMANTIC_BLENDWEIGHT);
			}
			if (PerMesh->GetNumColorChannels() > 0)
			{
				UINT CurrentColor = 0u;
				for (UINT IndexColor = 0u, MaxColor = AI_MAX_NUMBER_OF_COLOR_SETS; IndexColor < MaxColor; IndexColor++)
				{
					if (PerMesh->HasVertexColors(IndexColor))
					{
						switch (CurrentColor)
						{
						case 0u:
							PerMeshLayout.Add(RShaderSemanticType::SHADER_SEMANTIC_COLOR0);
							break;
						case 1u:
							PerMeshLayout.Add(RShaderSemanticType::SHADER_SEMANTIC_COLOR1);
							break;
						case 2u:
							PerMeshLayout.Add(RShaderSemanticType::SHADER_SEMANTIC_COLOR2);
							break;
						case 3u:
							PerMeshLayout.Add(RShaderSemanticType::SHADER_SEMANTIC_COLOR3);
							break;
						}
						CurrentColor++;
					}
				}
			}
		}
		return TRUE;
	}
	template<typename TDataType, typename TMeshType>
	void CAssimpManager::TryAddMeshVertexPart(const TArray<const TDataType*>& InDatas, const TArray<UINT>& InDataElementNum, const EVertexLayoutType InStoredLayoutBaseType, const UINT* InStoredLayoutSlots, const UINT InStoredLayoutNum, const UINT InStrideIn32Bits, const UINT InSuccessAddMaxNum, TMeshType& OutMesh)
	{
		UINT AddElementCounter = 0u;
		for (UINT TryStoredSlotIndex = 0u; TryStoredSlotIndex < InStoredLayoutNum; TryStoredSlotIndex++)
		{
			if (AddElementCounter >= InDatas.Length())
			{
				return;
			}
			UINT TryStoredLayoutSlot = InStoredLayoutSlots[TryStoredSlotIndex];
			if (OutMesh.CheckVertexLayoutPartExist(InStoredLayoutBaseType, TryStoredLayoutSlot))
			{
				continue;
			}
			const UINT DataElementNum = InDataElementNum[AddElementCounter];
			EVertexData TempVertexData;
			TempVertexData.PartType = InStoredLayoutBaseType;
			TempVertexData.ElementNum = DataElementNum;
			TempVertexData.Stride = InStrideIn32Bits * sizeof(FLOAT);
			TempVertexData.Datas = new FLOAT[InStrideIn32Bits * DataElementNum];
			StoreVertexData(InDatas[AddElementCounter], DataElementNum, InStrideIn32Bits, TempVertexData.Datas);
			if (OutMesh.AddVertexElement(&TempVertexData))
			{
				AddElementCounter += 1u;
				if (AddElementCounter >= InSuccessAddMaxNum)
				{
					return;
				}
			}
		}
	}
	void CAssimpManager::TryAddMeshSkinPart(const TArray<const aiBone*>& InBones, const UINT InVertexNum, const EVertexLayoutType InStoredLayoutBaseType, const UINT* InStoredLayoutSlots, const UINT InStoredLayoutNum, const UINT InSuccessAddMaxNum, ESkinnedMesh& OutMesh)
	{
		const UINT AssimpMeshSkinElementMaxCount = 1u;	//For assimp we assum that every mesh contains only ONE skinned data.
		{
			//Add bind pose for skinned datas.
			OutMesh.ClearBindPose();
			for (UINT i = 0u, n = InBones.Length(); i < n; i++)
			{
				const aiBone* TempBone = InBones[i];
				if (!TempBone)
				{
					continue;
				}
				OutMesh.AddBindPose(AssimpTranslateString(TempBone->mName), AssimpTranslateMatrix(TempBone->mOffsetMatrix, TRUE));
			}
			OutMesh.GenerateBindPoseIndex();
		}
		TArray<EReaderVertexWeights> TempVertexWeights;
		USHORT TempMaxWeightCount = 0u;
		{
			const ESkinnedMesh::EBindPoseIndex& TempBoneList = OutMesh.GetBindPoseIndex();
			if (TempBoneList.Length() == 0u)
			{
				return;
			}
			TempVertexWeights.Recapacity(InVertexNum);
			for (UINT i = 0u; i < InVertexNum; i++)
			{
				TempVertexWeights.Add(EReaderVertexWeights());
			}
			for (UINT i = 0u, n = InBones.Length(); i < n; i++)
			{
				const aiBone* TempBone = InBones[i];
				if (!TempBone)
				{
					continue;
				}
				for (UINT WeightIndex = 0u, WeightNum = TempBone->mNumWeights; WeightIndex < WeightNum; WeightIndex++)
				{
					const aiVertexWeight& TempAssimpWeight = TempBone->mWeights[WeightIndex];
					if (TempAssimpWeight.mVertexId < InVertexNum)
					{
						EReaderVertexWeights& ModifyWeight = TempVertexWeights[TempAssimpWeight.mVertexId];
						if (ModifyWeight.MaxCount < static_cast<USHORT>(ESettings::ENGINE_BONE_WEIGHT_NUM_MAXIMUM))
						{
							if (TempBoneList.FindValue(AssimpTranslateString(TempBone->mName), ModifyWeight.Indices[ModifyWeight.MaxCount]))
							{
								ModifyWeight.Weights[ModifyWeight.MaxCount] = TempAssimpWeight.mWeight;
								ModifyWeight.MaxCount += 1u;
								TempMaxWeightCount = EMath::Max(TempMaxWeightCount, ModifyWeight.MaxCount);
							}
						}
					}
#ifdef _EDITOR_ONLY
					else
					{
						PE_FAILED((ENGINE_ASSET_ERROR), ("Read skinned mesh bone's vertex id is not matching vertex max count, maybe used wrong bone list."));
					}
#endif
				}
			}
			if (TempMaxWeightCount == 0u)
			{
				return;
			}
		}
		{
			UINT AddElementCounter = 0u;
			for (UINT TryStoredSlotIndex = 0u; TryStoredSlotIndex < InStoredLayoutNum; TryStoredSlotIndex++)
			{
				if (AddElementCounter >= AssimpMeshSkinElementMaxCount)
				{
					return;
				}
				UINT TryStoredLayoutSlot = InStoredLayoutSlots[TryStoredSlotIndex];
				if (OutMesh.CheckVertexLayoutPartExist(InStoredLayoutBaseType, TryStoredLayoutSlot))
				{
					continue;
				}
				const UINT DataElementNum = InVertexNum;
				ESkinData TempSkinData;
				TempSkinData.PartType = InStoredLayoutBaseType;
				TempSkinData.ElementNum = DataElementNum;
				TempSkinData.Stride = TempMaxWeightCount * sizeof(FLOAT);
				TempSkinData.Indices = new UINT[TempMaxWeightCount * DataElementNum];
				TempSkinData.Weights = new FLOAT[TempMaxWeightCount * DataElementNum];
				StoreSkinData(TempVertexWeights, TempMaxWeightCount, TempSkinData.Indices, TempSkinData.Weights);
				if (OutMesh.AddSkinElement(&TempSkinData))
				{
					AddElementCounter += 1u;
					if (AddElementCounter >= InSuccessAddMaxNum)
					{
						return;
					}
				}
			}
		}
	}
	TArray<CAssimpManager::StoredMeshLayoutDesc> CAssimpManager::GetShouldStoredMeshLayoutDescriptions(const RShaderSemanticType* InLayouts, const UINT InLayoutNum)
	{
		TArray<StoredMeshLayoutDesc> Result;
		TArray<UINT> BlendIndicesSlots, BlendWeightsSlots;
		for (UINT LayoutIndex = 0u; LayoutIndex < InLayoutNum; LayoutIndex++)
		{
			RShaderSemanticType SemanticBaseType = GetShaderSemanticBaseType(InLayouts[LayoutIndex]);
			if (SemanticBaseType == RShaderSemanticType::SHADER_SEMANTIC_NONE)
			{
				continue;
			}
			//We calculate skin data after this part.
			if (SemanticBaseType == RShaderSemanticType::SHADER_SEMANTIC_BLENDINDICES)
			{
				BlendIndicesSlots.Add(GetShaderSemanticTypeSlot(InLayouts[LayoutIndex]));
				continue;
			}
			if (SemanticBaseType == RShaderSemanticType::SHADER_SEMANTIC_BLENDWEIGHT)
			{
				BlendWeightsSlots.Add(GetShaderSemanticTypeSlot(InLayouts[LayoutIndex]));
				continue;
			}
			UINT SemanticTypeSlot = GetShaderSemanticTypeSlot(InLayouts[LayoutIndex]);
			if (SemanticTypeSlot >= EMesh::MeshVertexLayoutPartMaxNum)
			{
				continue;
			}
			Result.Add(StoredMeshLayoutDesc());
			StoredMeshLayoutDesc& CurrentDesc = Result[Result.Length() - 1u];
			CurrentDesc.TryStoredLayoutNum += 1u;
			CurrentDesc.TryStoredLayoutSlot[CurrentDesc.TryStoredLayoutNum - 1u] = SemanticTypeSlot;

			//TODO We hard coding to finding VertexBaseType for now.
			CurrentDesc.BaseVertexLayout = TranslateSemanticBaseTypeToVertexBaseLayout(SemanticBaseType);
		}
#if !(_TEST_MODE || _SHIPPING_MODE)
		if ((BlendIndicesSlots.Length()) > 0u)
#else
		if (((BlendIndicesSlots.Length()) > 0u) && ((BlendIndicesSlots.Length()) == (BlendWeightsSlots.Length())))
#endif
		{
#if !(_TEST_MODE || _SHIPPING_MODE)
			Check((ENGINE_ASSET_ERROR), ("number of [BlendIndices] and number of [BlendWeight] is not matched."), ((BlendIndicesSlots.Length()) == (BlendWeightsSlots.Length())));
#endif
			for (UINT BlendIndex = 0u, BlendNum = BlendIndicesSlots.Length(); BlendIndex < BlendNum; BlendIndex++)
			{
				UINT BlendIndicesSlot = BlendIndicesSlots[BlendIndex];
				UINT BlendWeightsSlot = BlendWeightsSlots[BlendIndex];
#if !(_TEST_MODE || _SHIPPING_MODE)
				Check((ENGINE_ASSET_ERROR), ("slot of [BlendIndices] and slot of [BlendWeight] is not matched."), (BlendIndicesSlot == BlendWeightsSlot));
#else
				if (BlendIndicesSlot != BlendWeightsSlot)
				{
					continue;
				}
#endif
				Result.Add(StoredMeshLayoutDesc());
				StoredMeshLayoutDesc& CurrentDesc = Result[Result.Length() - 1u];
				CurrentDesc.BaseVertexLayout = EVertexLayoutType::MESH_SKIN;
				CurrentDesc.TryStoredLayoutNum += 1u;
				CurrentDesc.TryStoredLayoutSlot[CurrentDesc.TryStoredLayoutNum - 1u] = BlendIndicesSlot;
			}
		}
		return Result;
	}
	void CAssimpManager::TranslateAssimpMeshToEngineMeshInternal(const RShaderSemanticType* InEngineLayouts, const UINT InEngineLayoutNum, const TArray<const aiMesh*>& InMeshes, TArray<EStaticMesh>& OutMeshes)
	{
		TArray<StoredMeshLayoutDesc> ShouldStoredLayoutDescriptions = GetShouldStoredMeshLayoutDescriptions(InEngineLayouts, InEngineLayoutNum);
		for (UINT MeshIndex = 0u, MeshNum = InMeshes.Length(); MeshIndex < MeshNum; MeshIndex++)
		{
			const aiMesh* AssimpMesh = InMeshes[MeshIndex];
			if ((!AssimpMesh) || ((AssimpMesh->mNumVertices) < 3u) || (!(AssimpMesh->HasFaces())) || ((AssimpMesh->mNumFaces) < 3u))
			{
				continue;
			}
			OutMeshes.Add(EStaticMesh(AssimpTranslateString(AssimpMesh->mName)));
			EStaticMesh& OutMesh = OutMeshes[OutMeshes.Length() - 1u];
			{
				OutMesh.SetBoundAABB(AssimpTranslateVector3(AssimpMesh->mAABB.mMin), AssimpTranslateVector3(AssimpMesh->mAABB.mMax));
			}
			for (UINT LayoutDescIndex = 0u, LayoutDescNum = ShouldStoredLayoutDescriptions.Length(); LayoutDescIndex < LayoutDescNum; LayoutDescIndex++)
			{
				StoredMeshLayoutDesc& StoredLayoutDesc = ShouldStoredLayoutDescriptions[LayoutDescIndex];
				EVertexLayoutType TempTryStoredLayoutBaseType = StoredLayoutDesc.BaseVertexLayout;
				if (TempTryStoredLayoutBaseType == EVertexLayoutType::MESH_VERTEX)
				{
					const UINT SuccessAddPositionMaxNum = 1u;
					const UINT PositionStrideIn32Bits = 4u;
					TArray<const aiVector3D*> TempPositionDatas;
					TArray<UINT> TempPositionNum;
					TempPositionDatas.Add(AssimpMesh->mVertices);
					TempPositionNum.Add(AssimpMesh->mNumVertices);
					TryAddMeshVertexPart(TempPositionDatas, TempPositionNum, TempTryStoredLayoutBaseType, StoredLayoutDesc.TryStoredLayoutSlot, StoredLayoutDesc.TryStoredLayoutNum, PositionStrideIn32Bits, SuccessAddPositionMaxNum, OutMesh);
				}
				else if (TempTryStoredLayoutBaseType == EVertexLayoutType::MESH_TEXTURECOORD)
				{
					if (AssimpMesh->GetNumUVChannels() > 0)
					{
						const UINT SuccessAddTexcoordMaxNum = 3u;
						const UINT TexcoordStrideIn32Bits = 2u;
						TArray<const aiVector3D*> TempTexcoordDatas;
						TArray<UINT> TempTexcoordNum;
						for (UINT TexcoordIndex = 0u, TexcoordMax = AI_MAX_NUMBER_OF_TEXTURECOORDS; TexcoordIndex < TexcoordMax; TexcoordIndex++)
						{
							if (!(AssimpMesh->HasTextureCoords(TexcoordIndex)))
							{
								continue;
							}
							TempTexcoordDatas.Add(AssimpMesh->mTextureCoords[TexcoordIndex]);
							TempTexcoordNum.Add(AssimpMesh->mNumVertices);
						}
						TryAddMeshVertexPart(TempTexcoordDatas, TempTexcoordNum, TempTryStoredLayoutBaseType, StoredLayoutDesc.TryStoredLayoutSlot, StoredLayoutDesc.TryStoredLayoutNum, TexcoordStrideIn32Bits, SuccessAddTexcoordMaxNum, OutMesh);
					}
				}
				else if (TempTryStoredLayoutBaseType == EVertexLayoutType::MESH_NORMAL)
				{
					if (AssimpMesh->HasNormals())
					{
						const UINT SuccessAddNormalMaxNum = 1u;
						const UINT NormalStrideIn32Bits = 4u;
						TArray<const aiVector3D*> TempNormalDatas;
						TArray<UINT> TempNormalNum;
						TempNormalDatas.Add(AssimpMesh->mNormals);
						TempNormalNum.Add(AssimpMesh->mNumVertices);
						TryAddMeshVertexPart(TempNormalDatas, TempNormalNum, TempTryStoredLayoutBaseType, StoredLayoutDesc.TryStoredLayoutSlot, StoredLayoutDesc.TryStoredLayoutNum, NormalStrideIn32Bits, SuccessAddNormalMaxNum, OutMesh);
					}
				}
				else if (TempTryStoredLayoutBaseType == EVertexLayoutType::MESH_TANGENT)
				{
					if (AssimpMesh->HasTangentsAndBitangents())
					{
						const UINT SuccessAddTangentMaxNum = 1u;
						const UINT TangentStrideIn32Bits = 4u;
						TArray<const aiVector3D*> TempTangentDatas;
						TArray<UINT> TempTangentNum;
						TempTangentDatas.Add(AssimpMesh->mTangents);
						TempTangentNum.Add(AssimpMesh->mNumVertices);
						TryAddMeshVertexPart(TempTangentDatas, TempTangentNum, TempTryStoredLayoutBaseType, StoredLayoutDesc.TryStoredLayoutSlot, StoredLayoutDesc.TryStoredLayoutNum, TangentStrideIn32Bits, SuccessAddTangentMaxNum, OutMesh);
					}
				}
				else if (TempTryStoredLayoutBaseType == EVertexLayoutType::MESH_COLOR)
				{
					if (AssimpMesh->GetNumColorChannels() > 0)
					{
						const UINT SuccessAddColorMaxNum = 1u;
						const UINT ColorStrideIn32Bits = 4u;
						TArray<const aiColor4D*> TempColorDatas;
						TArray<UINT> TempColorNum;
						for (UINT ColorIndex = 0u, ColorMax = AI_MAX_NUMBER_OF_COLOR_SETS; ColorIndex < ColorMax; ColorIndex++)
						{
							if (!(AssimpMesh->HasVertexColors(ColorIndex)))
							{
								continue;
							}
							TempColorDatas.Add(AssimpMesh->mColors[ColorIndex]);
							TempColorNum.Add(AssimpMesh->mNumVertices);
						}
						TryAddMeshVertexPart(TempColorDatas, TempColorNum, TempTryStoredLayoutBaseType, StoredLayoutDesc.TryStoredLayoutSlot, StoredLayoutDesc.TryStoredLayoutNum, ColorStrideIn32Bits, SuccessAddColorMaxNum, OutMesh);
					}
				}
				else if (TempTryStoredLayoutBaseType == EVertexLayoutType::MESH_BITANGENT)
				{
					if (AssimpMesh->HasTangentsAndBitangents())
					{
						const UINT SuccessAddBitangentMaxNum = 1u;
						const UINT BitangentStrideIn32Bits = 4u;
						TArray<const aiVector3D*> TempBitangentDatas;
						TArray<UINT> TempBitangentNum;
						TempBitangentDatas.Add(AssimpMesh->mBitangents);
						TempBitangentNum.Add(AssimpMesh->mNumVertices);
						TryAddMeshVertexPart(TempBitangentDatas, TempBitangentNum, TempTryStoredLayoutBaseType, StoredLayoutDesc.TryStoredLayoutSlot, StoredLayoutDesc.TryStoredLayoutNum, BitangentStrideIn32Bits, SuccessAddBitangentMaxNum, OutMesh);
					}
				}
			}
		}
	}
	void CAssimpManager::TranslateAssimpMeshToEngineMeshInternal(const RShaderSemanticType* InEngineLayouts, const UINT InEngineLayoutNum, const TArray<const aiMesh*>& InMeshes, TArray<ESkinnedMesh>& OutMeshes)
	{
		TArray<StoredMeshLayoutDesc> ShouldStoredLayoutDescriptions = GetShouldStoredMeshLayoutDescriptions(InEngineLayouts, InEngineLayoutNum);
		for (UINT MeshIndex = 0u, MeshNum = InMeshes.Length(); MeshIndex < MeshNum; MeshIndex++)
		{
			const aiMesh* AssimpMesh = InMeshes[MeshIndex];
			if ((!AssimpMesh) || ((AssimpMesh->mNumVertices) < 3u) || (!(AssimpMesh->HasFaces())) || ((AssimpMesh->mNumFaces) < 3u))
			{
				continue;
			}
			OutMeshes.Add(ESkinnedMesh(AssimpTranslateString(AssimpMesh->mName)));
			ESkinnedMesh& OutMesh = OutMeshes[OutMeshes.Length() - 1u];
			{
				OutMesh.SetBoundAABB(AssimpTranslateVector3(AssimpMesh->mAABB.mMin), AssimpTranslateVector3(AssimpMesh->mAABB.mMax));
			}
			for (UINT LayoutDescIndex = 0u, LayoutDescNum = ShouldStoredLayoutDescriptions.Length(); LayoutDescIndex < LayoutDescNum; LayoutDescIndex++)
			{
				StoredMeshLayoutDesc& StoredLayoutDesc = ShouldStoredLayoutDescriptions[LayoutDescIndex];
				EVertexLayoutType TempTryStoredLayoutBaseType = StoredLayoutDesc.BaseVertexLayout;
				if (TempTryStoredLayoutBaseType == EVertexLayoutType::MESH_VERTEX)
				{
					const UINT SuccessAddPositionMaxNum = 1u;
					const UINT PositionStrideIn32Bits = 4u;
					TArray<const aiVector3D*> TempPositionDatas;
					TArray<UINT> TempPositionNum;
					TempPositionDatas.Add(AssimpMesh->mVertices);
					TempPositionNum.Add(AssimpMesh->mNumVertices);
					TryAddMeshVertexPart(TempPositionDatas, TempPositionNum, TempTryStoredLayoutBaseType, StoredLayoutDesc.TryStoredLayoutSlot, StoredLayoutDesc.TryStoredLayoutNum, PositionStrideIn32Bits, SuccessAddPositionMaxNum, OutMesh);
				}
				else if (TempTryStoredLayoutBaseType == EVertexLayoutType::MESH_TEXTURECOORD)
				{
					if (AssimpMesh->GetNumUVChannels() > 0)
					{
						const UINT SuccessAddTexcoordMaxNum = 3u;
						const UINT TexcoordStrideIn32Bits = 2u;
						TArray<const aiVector3D*> TempTexcoordDatas;
						TArray<UINT> TempTexcoordNum;
						for (UINT TexcoordIndex = 0u, TexcoordMax = AI_MAX_NUMBER_OF_TEXTURECOORDS; TexcoordIndex < TexcoordMax; TexcoordIndex++)
						{
							if (!(AssimpMesh->HasTextureCoords(TexcoordIndex)))
							{
								continue;
							}
							TempTexcoordDatas.Add(AssimpMesh->mTextureCoords[TexcoordIndex]);
							TempTexcoordNum.Add(AssimpMesh->mNumVertices);
						}
						TryAddMeshVertexPart(TempTexcoordDatas, TempTexcoordNum, TempTryStoredLayoutBaseType, StoredLayoutDesc.TryStoredLayoutSlot, StoredLayoutDesc.TryStoredLayoutNum, TexcoordStrideIn32Bits, SuccessAddTexcoordMaxNum, OutMesh);
					}
				}
				else if (TempTryStoredLayoutBaseType == EVertexLayoutType::MESH_NORMAL)
				{
					if (AssimpMesh->HasNormals())
					{
						const UINT SuccessAddNormalMaxNum = 1u;
						const UINT NormalStrideIn32Bits = 4u;
						TArray<const aiVector3D*> TempNormalDatas;
						TArray<UINT> TempNormalNum;
						TempNormalDatas.Add(AssimpMesh->mNormals);
						TempNormalNum.Add(AssimpMesh->mNumVertices);
						TryAddMeshVertexPart(TempNormalDatas, TempNormalNum, TempTryStoredLayoutBaseType, StoredLayoutDesc.TryStoredLayoutSlot, StoredLayoutDesc.TryStoredLayoutNum, NormalStrideIn32Bits, SuccessAddNormalMaxNum, OutMesh);
					}
				}
				else if (TempTryStoredLayoutBaseType == EVertexLayoutType::MESH_TANGENT)
				{
					if (AssimpMesh->HasTangentsAndBitangents())
					{
						const UINT SuccessAddTangentMaxNum = 1u;
						const UINT TangentStrideIn32Bits = 4u;
						TArray<const aiVector3D*> TempTangentDatas;
						TArray<UINT> TempTangentNum;
						TempTangentDatas.Add(AssimpMesh->mTangents);
						TempTangentNum.Add(AssimpMesh->mNumVertices);
						TryAddMeshVertexPart(TempTangentDatas, TempTangentNum, TempTryStoredLayoutBaseType, StoredLayoutDesc.TryStoredLayoutSlot, StoredLayoutDesc.TryStoredLayoutNum, TangentStrideIn32Bits, SuccessAddTangentMaxNum, OutMesh);
					}
				}
				else if (TempTryStoredLayoutBaseType == EVertexLayoutType::MESH_COLOR)
				{
					if (AssimpMesh->GetNumColorChannels() > 0)
					{
						const UINT SuccessAddColorMaxNum = 1u;
						const UINT ColorStrideIn32Bits = 4u;
						TArray<const aiColor4D*> TempColorDatas;
						TArray<UINT> TempColorNum;
						for (UINT ColorIndex = 0u, ColorMax = AI_MAX_NUMBER_OF_COLOR_SETS; ColorIndex < ColorMax; ColorIndex++)
						{
							if (!(AssimpMesh->HasVertexColors(ColorIndex)))
							{
								continue;
							}
							TempColorDatas.Add(AssimpMesh->mColors[ColorIndex]);
							TempColorNum.Add(AssimpMesh->mNumVertices);
						}
						TryAddMeshVertexPart(TempColorDatas, TempColorNum, TempTryStoredLayoutBaseType, StoredLayoutDesc.TryStoredLayoutSlot, StoredLayoutDesc.TryStoredLayoutNum, ColorStrideIn32Bits, SuccessAddColorMaxNum, OutMesh);
					}
				}
				else if (TempTryStoredLayoutBaseType == EVertexLayoutType::MESH_BITANGENT)
				{
					if (AssimpMesh->HasTangentsAndBitangents())
					{
						const UINT SuccessAddBitangentMaxNum = 1u;
						const UINT BitangentStrideIn32Bits = 4u;
						TArray<const aiVector3D*> TempBitangentDatas;
						TArray<UINT> TempBitangentNum;
						TempBitangentDatas.Add(AssimpMesh->mBitangents);
						TempBitangentNum.Add(AssimpMesh->mNumVertices);
						TryAddMeshVertexPart(TempBitangentDatas, TempBitangentNum, TempTryStoredLayoutBaseType, StoredLayoutDesc.TryStoredLayoutSlot, StoredLayoutDesc.TryStoredLayoutNum, BitangentStrideIn32Bits, SuccessAddBitangentMaxNum, OutMesh);
					}
				}
				else if (TempTryStoredLayoutBaseType == EVertexLayoutType::MESH_SKIN)
				{
					if (AssimpMesh->HasBones())
					{
						const UINT SuccessAddBoneMaxNum = 1u;
						TArray<const aiBone*> TempBones;
						for (UINT BoneIndex = 0u; BoneIndex < AssimpMesh->mNumBones; BoneIndex++)
						{
							const aiBone* TempBone = AssimpMesh->mBones[BoneIndex];
							if (!TempBone)
							{
								continue;
							}
							if (TempBone->mNumWeights > 0u)
							{
								TempBones.Add(TempBone);
							}
						}
						TryAddMeshSkinPart(TempBones, AssimpMesh->mNumVertices, TempTryStoredLayoutBaseType, StoredLayoutDesc.TryStoredLayoutSlot, StoredLayoutDesc.TryStoredLayoutNum, SuccessAddBoneMaxNum, OutMesh);
					}
				}
			}
		}
	}
	BOOL CAssimpManager::GatherAllBoneNodeDatas(const aiScene* InScene, TArray<EBoneData>& OutBones, TMap<EString, SHORT>& OutBoneIndices)
	{
		if (OutBones.Length() > 0u)
		{
			OutBones.Clear();
		}

		// Read data structures of all nodes.
		TArray<const aiNode*> TempNodes; TMap<const aiNode*, EString> TempNodeNames; TMap<EString, SHORT> TempNodeIndices;
		AssimpGatherSingleNodeRecursion(InScene->mRootNode, TempNodes, TempNodeNames, TempNodeIndices);

		// Check read state.
		if ((TempNodes.Length() < 1u) || (TempNodes.Length() != TempNodeNames.Length()) || (TempNodes.Length() != TempNodeIndices.Length()))
		{
			return FALSE;
		}

		// Create engine's hierarchical bones' data.
		for (UINT NodeIndex = 0u, NodeNum = TempNodes.Length(); NodeIndex < NodeNum; NodeIndex++)
		{
			EString TempNodeName;
			if (!(TempNodeNames.FindValue(TempNodes[NodeIndex], TempNodeName)))
			{
				PE_FAILED((ENGINE_ASSET_ERROR), ("Bone node find name failed(This step must be finded)."));
				continue;
			}
			OutBones.Add(EBoneData(TempNodeName));
			EBoneData& NewBone = OutBones[OutBones.Length() - 1u];
			NewBone.Index = static_cast<SHORT>(NodeIndex);
			NewBone.Parent = -1;
			if (TempNodes[NodeIndex]->mParent != nullptr)
			{
				EString TempNodeParentName;
				if (TempNodeNames.FindValue(TempNodes[NodeIndex]->mParent, TempNodeParentName))
				{
					SHORT TempNodeParentIndex = -1;
					if (TempNodeIndices.FindValue(TempNodeParentName, TempNodeParentIndex))
					{
						NewBone.Parent = TempNodeParentIndex;
					}
				}
			}
			OutBoneIndices.Add(NewBone.Name, NewBone.Index);
		}

		// Read skeleton default transform.
		AssimpReadNodeDefaultTransformRecursion(InScene->mRootNode, TempNodeNames, OutBoneIndices, OutBones);

		// Save node's child relation.
		for (SHORT NodeIndex = 0, NodeNum = static_cast<SHORT>(OutBones.Length()); NodeIndex < NodeNum; NodeIndex++)
		{
			EBoneData& BoneData = OutBones[NodeIndex];
			if ((NodeIndex != BoneData.Index) || (BoneData.Parent < 0) || (BoneData.Parent >= NodeNum))
			{
				continue;
			}
			EBoneData& BoneParentData = OutBones[BoneData.Parent];
			BoneParentData.Children.Add(BoneData.Index);
		}

		return TRUE;
	}
	void CAssimpManager::Initialize()
	{
		if (_GAssetImporter == nullptr)
		{
			_GAssetImporter = new Assimp::Importer();
		}
	}
	void CAssimpManager::ShutDown()
	{
		if (_GAssetImporter != nullptr)
		{
			_GAssetImporter->FreeScene();
			delete (_GAssetImporter);
			_GAssetImporter = nullptr;
		}
	}
	CAssimpManager::CReadFileStateType CAssimpManager::ReadStaticMeshFile(const EString& InPath, TArray<EStaticMesh>& OutMeshes)
	{
		CReadFileStateType Result = CReadFileStateType::ASSIMP_READ_FILE_STATE_FAILED;

		if (OutMeshes.Length() > 0u)
		{
			for (UINT i = 0u, n = OutMeshes.Length(); i < n; i++)
			{
				OutMeshes[i].Release();
			}
			OutMeshes.Clear();
		}

		Assimp::Importer* AssImporter = _GAssetImporter;
		if (AssImporter == nullptr)
		{
			// TODO Do the error logging (did not create the instance of importer)
			return Result;
		}

		// And have it read the given file with some example postprocessing
		// Usually - if speed is not the most important aspect for you - you'll
		// probably to request more postprocessing than we do in this example.

		// Use SetPropertyInteger to modify config of importer
		//Assimp::Importer::SetProperty###();

		const aiScene* Scene = AssImporter->ReadFile(
			*InPath,
			aiProcess_CalcTangentSpace |
			aiProcess_JoinIdenticalVertices |
			aiProcess_MakeLeftHanded |
			aiProcess_Triangulate |
			aiProcess_RemoveComponent |
			aiProcess_GenSmoothNormals |
			aiProcess_SplitLargeMeshes |
			/*aiProcess_ImproveCacheLocality |*/
			aiProcess_RemoveRedundantMaterials |
			aiProcess_FixInfacingNormals |
			aiProcess_SortByPType |
			aiProcess_FindInvalidData |
			aiProcess_GenUVCoords |
			aiProcess_OptimizeMeshes |
			aiProcess_OptimizeGraph |
			aiProcess_FlipUVs |
			aiProcess_FlipWindingOrder |
			aiProcess_GenBoundingBoxes);

		// If the import failed, report it
		if (Scene == nullptr)
		{
			// TODO Do the error logging (importer.GetErrorString())
			AssImporter->FreeScene();
			return Result;
		}

		if (!(Scene->HasMeshes()))
		{
			Result = CReadFileStateType::ASSIMP_READ_FILE_STATE_ERROR;
			// TODO Scene does not contain meshes
			AssImporter->FreeScene();
			return Result;
		}

		// Now we can access the file's contents.
		// Only access first mesh in scene.
		TArray<const aiMesh*> Meshes;
		{
			TArray<const aiMesh*> TempMeshes; TArray<TArray<RShaderSemanticType>> TempMeshesLayouts; TArray<BOOL> TempIsSkeletonMesh;
			FindMeshesAndVertexLayouts(Scene, TempMeshes, TempMeshesLayouts, TempIsSkeletonMesh);
			Check((ENGINE_ASSET_ERROR), ("Meshes and layouts are not matched."), (TempMeshes.Length() > 0u && TempMeshes.Length() == TempMeshesLayouts.Length() && TempMeshes.Length() == TempIsSkeletonMesh.Length()));
			for (UINT i = 0u, n = TempMeshes.Length(); i < n; i++)
			{
				if (TempIsSkeletonMesh[i])
				{
					continue;
				}
				Meshes.Add(TempMeshes[i]);
			}
		}

		if (Meshes.Length() == 0u)
		{
			Result = CReadFileStateType::ASSIMP_READ_FILE_STATE_ERROR;
			// TODO Scene does not contain static meshes
			AssImporter->FreeScene();
			return Result;
		}

		Check((ENGINE_ASSET_ERROR), ("Meshes and layouts are not matched."), (Meshes.Length() > 0u));

		const RShaderSemanticType* EngineLayouts; UINT EngineLayoutNum;
		GetEngineDefaultMeshInputLayouts(EngineLayouts, EngineLayoutNum);

		TranslateAssimpMeshToEngineMeshInternal(EngineLayouts, EngineLayoutNum, Meshes, OutMeshes);

		Result = CReadFileStateType::ASSIMP_READ_FILE_STATE_SUCCEED;
		// We're done. Everything will be cleaned up by the importer destructor
		AssImporter->FreeScene();
		return Result;
	}
	CAssimpManager::CReadFileStateType CAssimpManager::ReadSkeletonFile(const EString& InPath, ESkeleton& OutSkeleton)
	{
		CReadFileStateType Result = CReadFileStateType::ASSIMP_READ_FILE_STATE_FAILED;

		if (OutSkeleton.GetBoneCount() > 0u)
		{
			OutSkeleton.Release();
		}

		Assimp::Importer* AssImporter = _GAssetImporter;
		if (AssImporter == nullptr)
		{
			// TODO Do the error logging (did not create the instance of importer)
			return Result;
		}

		// And have it read the given file with some example postprocessing
		// Usually - if speed is not the most important aspect for you - you'll
		// probably to request more postprocessing than we do in this example.

		// Use SetPropertyInteger to modify config of importer
		//Assimp::Importer::SetProperty###();

		//const aiScene* scene = impoter->ReadFile(
		//	path,
		//	aiProcess_CalcTangentSpace |
		//	aiProcess_JoinIdenticalVertices |
		//	aiProcess_MakeLeftHanded |
		//	aiProcess_Triangulate |
		//	aiProcess_RemoveComponent |
		//	aiProcess_GenSmoothNormals |
		//	aiProcess_SplitLargeMeshes |
		//	/*aiProcess_LimitBoneWeights |*/
		//	aiProcess_RemoveRedundantMaterials |
		//	aiProcess_FixInfacingNormals |
		//	aiProcess_PopulateArmatureData |
		//	aiProcess_SortByPType |
		//	aiProcess_FindInvalidData |
		//	aiProcess_GenUVCoords |
		//	aiProcess_OptimizeMeshes |
		//	aiProcess_FlipUVs |
		//	aiProcess_FlipWindingOrder |
		//	/*aiProcess_SplitByBoneCount |*/
		//	/*aiProcess_Debone |*/
		//	aiProcess_GenBoundingBoxes);
		const aiScene* Scene = AssImporter->ReadFile(*InPath, aiProcessPreset_TargetRealtime_MaxQuality | aiProcess_ConvertToLeftHanded);

		// If the import failed, report it
		if (Scene == nullptr)
		{
			// TODO Do the error logging (importer.GetErrorString())
			AssImporter->FreeScene();
			return Result;
		}

		// Now we can access the file's contents.
		TArray<EBoneData> BoneDatas; TMap<EString, SHORT> BoneIndices;
		if (!GatherAllBoneNodeDatas(Scene, BoneDatas, BoneIndices))
		{
			Result = CReadFileStateType::ASSIMP_READ_FILE_STATE_ERROR;
			// TODO Do the error logging (importer.GetErrorString())
			AssImporter->FreeScene();
			return Result;
		}

		for (UINT i = 0u, n = BoneDatas.Length(); i < n; i++)
		{
			OutSkeleton.AddBoneElement(&(BoneDatas[i]));
		}

		Result = CReadFileStateType::ASSIMP_READ_FILE_STATE_SUCCEED;
		// We're done. Everything will be cleaned up by the importer destructor
		AssImporter->FreeScene();
		return Result;
	}
	CAssimpManager::CReadFileStateType CAssimpManager::ReadSkinnedMeshFile(const EString& InPath, TArray<ESkinnedMesh>& OutMeshes)
	{
		CReadFileStateType Result = CReadFileStateType::ASSIMP_READ_FILE_STATE_FAILED;

		if (OutMeshes.Length() > 0u)
		{
			for (UINT i = 0u, n = OutMeshes.Length(); i < n; i++)
			{
				OutMeshes[i].Release();
			}
			OutMeshes.Clear();
		}

		Assimp::Importer* AssImporter = _GAssetImporter;
		if (AssImporter == nullptr)
		{
			// TODO Do the error logging (did not create the instance of importer)
			return Result;
		}

		// And have it read the given file with some example postprocessing
		// Usually - if speed is not the most important aspect for you - you'll
		// probably to request more postprocessing than we do in this example.

		// Use SetPropertyInteger to modify config of importer
		//Assimp::Importer::SetProperty###();

		//const aiScene* scene = impoter->ReadFile(
		//	path,
		//	aiProcess_CalcTangentSpace |
		//	aiProcess_JoinIdenticalVertices |
		//	aiProcess_MakeLeftHanded |
		//	aiProcess_Triangulate |
		//	aiProcess_RemoveComponent |
		//	aiProcess_GenSmoothNormals |
		//	aiProcess_SplitLargeMeshes |
		//	/*aiProcess_LimitBoneWeights |*/
		//	aiProcess_RemoveRedundantMaterials |
		//	aiProcess_FixInfacingNormals |
		//	aiProcess_PopulateArmatureData |
		//	aiProcess_SortByPType |
		//	aiProcess_FindInvalidData |
		//	aiProcess_GenUVCoords |
		//	aiProcess_OptimizeMeshes |
		//	aiProcess_FlipUVs |
		//	aiProcess_FlipWindingOrder |
		//	/*aiProcess_SplitByBoneCount |*/
		//	/*aiProcess_Debone |*/
		//	aiProcess_GenBoundingBoxes);
		const aiScene* Scene = AssImporter->ReadFile(*InPath, aiProcessPreset_TargetRealtime_MaxQuality | aiProcess_ConvertToLeftHanded);

		// If the import failed, report it
		if (Scene == nullptr)
		{
			// TODO Do the error logging (importer.GetErrorString())
			AssImporter->FreeScene();
			return Result;
		}

		if (!(Scene->HasMeshes()))
		{
			Result = CReadFileStateType::ASSIMP_READ_FILE_STATE_ERROR;
			// TODO Do the error logging (importer.GetErrorString())
			AssImporter->FreeScene();
			return Result;
		}

		// Now we can access the file's contents.
		// Read data structures of all nodes.
		TArray<const aiMesh*> Meshes;
		{
			TArray<const aiMesh*> TempMeshes; TArray<TArray<RShaderSemanticType>> TempMeshesLayouts; TArray<BOOL> TempIsSkeletonMesh;
			FindMeshesAndVertexLayouts(Scene, TempMeshes, TempMeshesLayouts, TempIsSkeletonMesh);
			Check((ENGINE_ASSET_ERROR), ("Meshes and layouts are not matched."), (TempMeshes.Length() > 0u && TempMeshes.Length() == TempMeshesLayouts.Length() && TempMeshes.Length() == TempIsSkeletonMesh.Length()));
			for (UINT i = 0u, n = TempMeshes.Length(); i < n; i++)
			{
				if (!(TempIsSkeletonMesh[i]))
				{
					continue;
				}
				Meshes.Add(TempMeshes[i]);
			}
		}

		if (Meshes.Length() == 0u)
		{
			Result = CReadFileStateType::ASSIMP_READ_FILE_STATE_ERROR;
			// TODO Scene does not contain static meshes
			AssImporter->FreeScene();
			return Result;
		}

		const RShaderSemanticType* EngineLayouts; UINT EngineLayoutNum;
		GetEngineDefaultSkeletonMeshInputLayouts(EngineLayouts, EngineLayoutNum);

		TranslateAssimpMeshToEngineMeshInternal(EngineLayouts, EngineLayoutNum, Meshes, OutMeshes);


		Result = CReadFileStateType::ASSIMP_READ_FILE_STATE_SUCCEED;
		// We're done. Everything will be cleaned up by the importer destructor
		AssImporter->FreeScene();
		return Result;
	}
	CAssimpManager::CReadFileStateType CAssimpManager::ReadSkeletonAnimationFile(const EString& path, TArray<ESkeletonAnimationClip>& OutSkeletonAnimationClips)
	{
		CReadFileStateType Result = CReadFileStateType::ASSIMP_READ_FILE_STATE_FAILED;

		if (OutSkeletonAnimationClips.Length() > 0u)
		{
			OutSkeletonAnimationClips.Clear();
		}

		Assimp::Importer* AssImporter = _GAssetImporter;
		if (AssImporter == nullptr)
		{
			// TODO Do the error logging (did not create the instance of importer)
			return Result;
		}

		// And have it read the given file with some example postprocessing
		// Usually - if speed is not the most important aspect for you - you'll
		// probably to request more postprocessing than we do in this example.

		// Use SetPropertyInteger to modify config of importer
		//Assimp::Importer::SetProperty###();

		//const aiScene* scene = impoter->ReadFile(
		//	path,
		//	aiProcess_MakeLeftHanded |
		//	aiProcess_RemoveComponent |
		//	aiProcess_RemoveRedundantMaterials |
		//	aiProcess_PopulateArmatureData);

		const aiScene* scene = AssImporter->ReadFile(*path, aiProcessPreset_TargetRealtime_MaxQuality | aiProcess_MakeLeftHanded);

		// If the import failed, report it
		if (scene == nullptr)
		{
			// TODO Do the error logging (importer.GetErrorString())
			AssImporter->FreeScene();
			return Result;
		}

		if (!scene->HasAnimations())
		{
			Result = CReadFileStateType::ASSIMP_READ_FILE_STATE_ERROR;
			// TODO Do the error logging (importer.GetErrorString())
			AssImporter->FreeScene();
			return Result;
		}

		// Now we can access the file's contents.
		auto tempInsertResult = animationDatas.insert_or_assign(path, TMap<EString, CustomStruct::CGameAnimationInfo>());
		TMap<EString, CustomStruct::CGameAnimationInfo>& tempAnimationContainer = tempInsertResult.first->second;

		Result = _GGatherAllAnimationDatas(path, scene, tempAnimationContainer) == TRUE ? CReadFileStateType::ASSIMP_READ_FILE_STATE_SUCCEED : CReadFileStateType::ASSIMP_READ_FILE_STATE_ERROR;

		// We're done. Everything will be cleaned up by the importer destructor
		AssImporter->FreeScene();
		return Result;
	}
	//CAssimpManager::CReadFileStateType CAssimpManager::ReadSkeletonMeshAndBoneFile(const EString& path, EMesh::ESubmeshPart& subMesh, UINT& vertexStride, CHAR*& vertices, UINT& numVertices, TArray<UINT>& indices, UINT& numIndices, ESkeletonInfo& skeleton, TMap<EString, SHORT>& boneIndexMap, TArray<USHORT>& boneList)
	//{
	//	if (vertices != nullptr)
	//	{
	//		delete[]vertices;
	//		vertices = nullptr;
	//	}
	//	if (indices.size() > 0)
	//	{
	//		indices.clear();
	//	}
	//	if (subMesh.size() > 0)
	//	{
	//		subMesh.clear();
	//	}
	//	vertexStride = 0u;
	//	numVertices = 0u;
	//	numIndices = 0u;

	//	if (skeleton.size() > 0)
	//	{
	//		skeleton.clear();
	//	}
	//	if (boneIndexMap.size() > 0)
	//	{
	//		boneIndexMap.clear();
	//	}
	//	if (boneList.size() > 0)
	//	{
	//		boneList.clear();
	//	}

	//	CReadFileStateType result = CReadFileStateType::ASSIMP_READ_FILE_STATE_FAILED;

	//	Assimp::Importer* impoter = _GAssetImporter;
	//	if (impoter == nullptr)
	//	{
	//		// TODO Do the error logging (did not create the instance of importer)
	//		return result;
	//	}

	//	// And have it read the given file with some example postprocessing
	//	// Usually - if speed is not the most important aspect for you - you'll
	//	// probably to request more postprocessing than we do in this example.

	//	// Use SetPropertyInteger to modify config of importer
	//	//Assimp::Importer::SetProperty###();

	//	//const aiScene* scene = impoter->ReadFile(
	//	//	path,
	//	//	aiProcess_CalcTangentSpace |
	//	//	aiProcess_JoinIdenticalVertices |
	//	//	aiProcess_MakeLeftHanded |
	//	//	aiProcess_Triangulate |
	//	//	aiProcess_RemoveComponent |
	//	//	aiProcess_GenSmoothNormals |
	//	//	aiProcess_SplitLargeMeshes |
	//	//	/*aiProcess_LimitBoneWeights |*/
	//	//	aiProcess_RemoveRedundantMaterials |
	//	//	aiProcess_FixInfacingNormals |
	//	//	aiProcess_PopulateArmatureData |
	//	//	aiProcess_SortByPType |
	//	//	aiProcess_FindInvalidData |
	//	//	aiProcess_GenUVCoords |
	//	//	aiProcess_OptimizeMeshes |
	//	//	aiProcess_FlipUVs |
	//	//	aiProcess_FlipWindingOrder |
	//	//	/*aiProcess_SplitByBoneCount |*/
	//	//	/*aiProcess_Debone |*/
	//	//	aiProcess_GenBoundingBoxes);

	//	const aiScene* scene = impoter->ReadFile(path, aiProcessPreset_TargetRealtime_MaxQuality | aiProcess_ConvertToLeftHanded);

	//	// If the import failed, report it
	//	if (scene == nullptr)
	//	{
	//		// TODO Do the error logging (importer.GetErrorString())
	//		impoter->FreeScene();
	//		return result;
	//	}

	//	if (!scene->HasMeshes())
	//	{
	//		result = CReadFileStateType::ASSIMP_READ_FILE_STATE_ERROR;
	//		// TODO Do the error logging (importer.GetErrorString())
	//		impoter->FreeScene();
	//		return result;
	//	}

	//	// Now we can access the file's contents.
	//	const CustomStruct::CRenderInputLayoutDesc* inputLayoutDesc; UINT inputLayoutNum;
	//	CustomStruct::CRenderInputLayoutDesc::GetEngineSkeletonMeshInputLayouts(inputLayoutDesc, inputLayoutNum);
	//	result = _GGatherSkeletonMeshAllNodeStructures(scene, subMesh, vertexStride, vertices, numVertices, indices, numIndices, skeleton, boneIndexMap, boneList, inputLayoutDesc, inputLayoutNum) == TRUE ? CReadFileStateType::ASSIMP_READ_FILE_STATE_SUCCEED : CReadFileStateType::ASSIMP_READ_FILE_STATE_ERROR;

	//	// We're done. Everything will be cleaned up by the importer destructor
	//	impoter->FreeScene();
	//	return result;
	//}
	//CAssimpManager::CReadFileStateType CAssimpManager::ReadSkeletonAnimationFile(const EString& path, TMap<EString, TMap<EString, CustomStruct::CGameAnimationInfo>>& animationDatas)
	//{
	//	CReadFileStateType result = CReadFileStateType::ASSIMP_READ_FILE_STATE_FAILED;

	//	if (animationDatas.find(path) != animationDatas.end())
	//	{
	//		return result;
	//	}

	//	Assimp::Importer* impoter = _GAssetImporter;
	//	if (impoter == nullptr)
	//	{
	//		// TODO Do the error logging (did not create the instance of importer)
	//		return result;
	//	}

	//	// And have it read the given file with some example postprocessing
	//	// Usually - if speed is not the most important aspect for you - you'll
	//	// probably to request more postprocessing than we do in this example.

	//	// Use SetPropertyInteger to modify config of importer
	//	//Assimp::Importer::SetProperty###();

	//	//const aiScene* scene = impoter->ReadFile(
	//	//	path,
	//	//	aiProcess_MakeLeftHanded |
	//	//	aiProcess_RemoveComponent |
	//	//	aiProcess_RemoveRedundantMaterials |
	//	//	aiProcess_PopulateArmatureData);

	//	const aiScene* scene = impoter->ReadFile(path, aiProcessPreset_TargetRealtime_MaxQuality | aiProcess_MakeLeftHanded);

	//	// If the import failed, report it
	//	if (scene == nullptr)
	//	{
	//		// TODO Do the error logging (importer.GetErrorString())
	//		impoter->FreeScene();
	//		return result;
	//	}

	//	if (!scene->HasAnimations())
	//	{
	//		result = CReadFileStateType::ASSIMP_READ_FILE_STATE_ERROR;
	//		// TODO Do the error logging (importer.GetErrorString())
	//		impoter->FreeScene();
	//		return result;
	//	}

	//	// Now we can access the file's contents.
	//	auto tempInsertResult = animationDatas.insert_or_assign(path, TMap<EString, CustomStruct::CGameAnimationInfo>());
	//	TMap<EString, CustomStruct::CGameAnimationInfo>& tempAnimationContainer = tempInsertResult.first->second;

	//	result = _GGatherAllAnimationDatas(path, scene, tempAnimationContainer) == TRUE ? CReadFileStateType::ASSIMP_READ_FILE_STATE_SUCCEED : CReadFileStateType::ASSIMP_READ_FILE_STATE_ERROR;

	//	// We're done. Everything will be cleaned up by the importer destructor
	//	impoter->FreeScene();
	//	return result;
	//}
};