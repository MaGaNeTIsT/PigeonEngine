#include "../Headers/CassimpManager.h"
#include "../../../Sources/EngineBase/Headers/CBaseType.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#define CUSTOM_IS_QNAN(f)	(f != f)

static Assimp::Importer* _GAssetImporter = nullptr;

inline CustomType::Quaternion _GTranslateQuaternion(const aiQuaternion& v)
{
	return (CustomType::Quaternion(v.x, v.y, v.z, v.w));
}

inline CustomType::Vector3 _GTranslateVector3(const aiVector3D& v)
{
	return (CustomType::Vector3(v.x, v.y, v.z));
}

inline CustomType::Matrix4x4 _GTranslateMatrix(const aiMatrix4x4& m)
{
	aiVector3D scaling, position;
	aiVector3D rotation;
	m.Decompose(scaling, rotation, position);
	return (CustomType::Matrix4x4(
		CustomType::Vector3(position.x, position.y, position.z),
		CustomType::Quaternion(DirectX::XMQuaternionRotationRollPitchYaw(rotation.x, rotation.y, rotation.z)),
		CustomType::Vector3(scaling.x, scaling.y, scaling.z)));
}

inline void _GTranslateMatrix(const aiMatrix4x4& m, CustomType::Vector3& location, CustomType::Quaternion& rotation, CustomType::Vector3& scale)
{
	aiVector3D aiScaling, aiPosition;
	aiVector3D aiRotation;
	m.Decompose(aiScaling, aiRotation, aiPosition);

	location = CustomType::Vector3(aiPosition.x, aiPosition.y, aiPosition.z);
	rotation = CustomType::Quaternion(DirectX::XMQuaternionRotationRollPitchYaw(aiRotation.x, aiRotation.y, aiRotation.z));
	scale = CustomType::Vector3(aiScaling.x, aiScaling.y, aiScaling.z);
}

inline std::string _GTranslateString(const aiString& s)
{
	return (std::string(s.C_Str()));
}

inline aiString _GTranslateString(const std::string& s)
{
	return (aiString(s));
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
	std::string				Name;
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

BOOL _GTranslateMeshDesc(const aiScene* scene, std::vector<CustomStruct::CSubMeshInfo>& subMesh, std::vector<_GMeshAssetImporterInfo>& sceneMeshesInfo, UINT& totalNumVertices, UINT& totalNumIndices, std::vector<BOOL>& hasBones, const CustomStruct::CRenderInputLayoutDesc* inputLayoutDesc, const UINT& inputLayoutNum)
{
	const static UINT constVertexColorIndex		= 0u;
	const static UINT constVertexTexcoordIndex	= 0u;

	if (subMesh.size() > 0)
	{
		subMesh.clear();
	}
	if (sceneMeshesInfo.size() > 0)
	{
		sceneMeshesInfo.clear();
	}
	if (hasBones.size() > 0)
	{
		hasBones.clear();
	}
	totalNumVertices	= 0u;
	totalNumIndices		= 0u;

	if (scene == nullptr || !scene->HasMeshes())
	{
		return FALSE;
	}

	UINT numMeshes = scene->mNumMeshes;
	for (UINT indexMesh = 0u; indexMesh < numMeshes; indexMesh++)
	{
		const aiMesh* tempMesh = scene->mMeshes[indexMesh];
		if (tempMesh == nullptr)
		{
			continue;
		}

		hasBones.push_back(tempMesh->HasBones());

		_GMeshAssetImporterInfo tempPerMeshInfo;
		tempPerMeshInfo.Name = tempMesh->mName.C_Str();
		tempPerMeshInfo.NumVertices = tempMesh->mNumVertices;
		tempPerMeshInfo.NumIndices = tempMesh->mNumFaces * 3u;

		for (UINT i = 0u; i < inputLayoutNum; i++)
		{
			CustomStruct::CRenderInputLayoutDesc desc(inputLayoutDesc[i]);
			if (desc.SemanticName == CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_COLOR)
			{
				if (!tempMesh->HasVertexColors(constVertexColorIndex))
				{
					return FALSE;
				}
				tempPerMeshInfo.VertexColor.Exist = TRUE;
				tempPerMeshInfo.VertexColor.Offset = tempPerMeshInfo.VertexStride;
			}
			else if (desc.SemanticName == CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_POSITION)
			{
				if (!tempMesh->HasPositions())
				{
					return FALSE;
				}
				tempPerMeshInfo.VertexPosition.Exist = TRUE;
				tempPerMeshInfo.VertexPosition.Offset = tempPerMeshInfo.VertexStride;
			}
			else if (desc.SemanticName == CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_NORMAL)
			{
				if (!tempMesh->HasNormals())
				{
					return FALSE;
				}
				tempPerMeshInfo.VertexNormal.Exist = TRUE;
				tempPerMeshInfo.VertexNormal.Offset = tempPerMeshInfo.VertexStride;
			}
			else if (desc.SemanticName == CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_TANGENT)
			{
				if (!tempMesh->HasTangentsAndBitangents())
				{
					return FALSE;
				}
				tempPerMeshInfo.VertexTangent.Exist = TRUE;
				tempPerMeshInfo.VertexTangent.Offset = tempPerMeshInfo.VertexStride;
			}
			else if (desc.SemanticName == CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_TEXCOORD)
			{
				if (!tempMesh->HasTextureCoords(constVertexTexcoordIndex))
				{
					return FALSE;
				}
				tempPerMeshInfo.VertexUV.Exist = TRUE;
				tempPerMeshInfo.VertexUV.Offset = tempPerMeshInfo.VertexStride;
			}
			else if (desc.SemanticName == CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_BLENDINDICES)
			{
				tempPerMeshInfo.VertexBoneIndices.Exist = TRUE;
				tempPerMeshInfo.VertexBoneIndices.Offset = tempPerMeshInfo.VertexStride;
			}
			else if (desc.SemanticName == CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_BLENDWEIGHT)
			{
				tempPerMeshInfo.VertexBoneWeight.Exist = TRUE;
				tempPerMeshInfo.VertexBoneWeight.Offset = tempPerMeshInfo.VertexStride;
			}
			tempPerMeshInfo.VertexStride += desc.GetSemanticSizeByByte();
		}

		sceneMeshesInfo.push_back(tempPerMeshInfo);
	}

	subMesh.resize(sceneMeshesInfo.size());
	UINT indexSubMesh = 0u;
	for (UINT indexMesh = 0u; indexMesh < numMeshes; indexMesh++)
	{
		if (scene->mMeshes[indexMesh] == nullptr)
		{
			continue;
		}

		CustomStruct::CSubMeshInfo& subMeshInfo = subMesh[indexSubMesh];
		_GMeshAssetImporterInfo& sceneMeshInfo = sceneMeshesInfo[indexSubMesh];

		subMeshInfo.IndexStart = totalNumIndices;
		subMeshInfo.IndexCount = sceneMeshInfo.NumIndices;
		subMeshInfo.VertexStart = totalNumVertices;
		subMeshInfo.VertexCount = sceneMeshInfo.NumVertices;

		totalNumVertices += sceneMeshInfo.NumVertices;
		totalNumIndices += sceneMeshInfo.NumIndices;

		indexSubMesh += 1u;
	}

	return TRUE;
}

void _GTranslateMeshVertexData(const aiMesh* mesh, const _GMeshAssetImporterInfo& assetInfo, const CustomStruct::CSubMeshInfo& subMeshInfo, CHAR*& verticesData, std::vector<UINT>& indicesData, const std::vector<CustomStruct::CGameBoneNodeInfo>& skeletonInput, const std::vector<UINT>& boneListInput)
{
	const static UINT constVertexColorIndex		= 0u;
	const static UINT constVertexTexcoordIndex	= 0u;

	{
		UINT numFaces = mesh->mNumFaces;
		aiFace* faces = mesh->mFaces;
		for (UINT indexFace = 0u; indexFace < numFaces; indexFace++)
		{
			aiFace& tempFace = faces[indexFace];
			for (UINT i = 0u; i < 3u; i++)
			{
				indicesData[subMeshInfo.IndexStart + (indexFace * 3u + i)] = tempFace.mIndices[i];
			}
		}
	}

	{
		UINT		numVertices		= mesh->mNumVertices;

		aiColor4D*	colors			= mesh->mColors[constVertexColorIndex];
		aiVector3D*	vertices		= mesh->mVertices;
		aiVector3D*	normals			= mesh->mNormals;
		aiVector3D*	tangents		= mesh->mTangents;
		aiVector3D*	texcoords		= mesh->mTextureCoords[constVertexTexcoordIndex];

		std::vector<std::vector<std::pair<USHORT, FLOAT>>> blendIndicesWeights;
		UINT* blendWriteIndex = nullptr;

		if (skeletonInput.size() != 0 && boneListInput.size() != 0)
		{
			blendWriteIndex = new UINT[numVertices];
			blendIndicesWeights.resize(numVertices);

			auto _FindBoneListIndex = [](const std::string& boneName, const std::vector<CustomStruct::CGameBoneNodeInfo>& skeleton, const std::vector<UINT>& boneList)->UINT {
				UINT skeletonNum = static_cast<UINT>(skeleton.size());
				UINT boneListNum = static_cast<UINT>(boneList.size());
				for (UINT findBoneIndex = 0u; findBoneIndex < boneListNum; findBoneIndex++)
				{
					const UINT& tempBoneIndex = boneList[findBoneIndex];
					const CustomStruct::CGameBoneNodeInfo& tempBoneInSkeleton = skeleton[tempBoneIndex];
					if (tempBoneInSkeleton.Name == boneName)
					{
						return findBoneIndex;
					}
				}
				return 0u; };
			auto _SortBoneIndicesAndWeight = [](const UINT& num, std::vector<std::pair<USHORT, FLOAT>>& inputIndicesWeights) {
				BOOL needLoop = TRUE;
				while (needLoop == TRUE)
				{
					BOOL isModify = FALSE;
					for (UINT index = 1u; index < num; index++)
					{
						if (inputIndicesWeights[index].second > inputIndicesWeights[index - 1].second)
						{
							USHORT indexOfSmaller = inputIndicesWeights[index - 1].first;
							FLOAT weightOfSmaller = inputIndicesWeights[index - 1].second;
							inputIndicesWeights[index - 1] = std::pair<USHORT, FLOAT>(inputIndicesWeights[index].first, inputIndicesWeights[index].second);
							inputIndicesWeights[index] = std::pair<USHORT, FLOAT>(indexOfSmaller, weightOfSmaller);
							isModify |= TRUE;
						}
					}
					needLoop = isModify;
				}};

			for (UINT indexVertex = 0u; indexVertex < numVertices; indexVertex++)
			{
				blendWriteIndex[indexVertex] = 0u;
			}
			UINT boneNum = mesh->mNumBones;
			for (UINT boneIndex = 0u; boneIndex < boneNum; boneIndex++)
			{
				const aiBone* bone = mesh->mBones[boneIndex];
				if (bone->mNumWeights == 0u)
				{
					continue;
				}
				UINT tempBoneIndex = _FindBoneListIndex(_GTranslateString(bone->mName), skeletonInput, boneListInput);
				for (UINT boneWeightIndex = 0u; boneWeightIndex < bone->mNumWeights; boneWeightIndex++)
				{
					const aiVertexWeight& tempBoneWeight = bone->mWeights[boneWeightIndex];
					blendWriteIndex[tempBoneWeight.mVertexId] += 1u;
					blendIndicesWeights[tempBoneWeight.mVertexId].push_back(std::pair<USHORT, FLOAT>(static_cast<USHORT>(tempBoneIndex), tempBoneWeight.mWeight));
				}
			}
			for (UINT vertexIndex = 0u; vertexIndex < numVertices; vertexIndex++)
			{
				if (blendWriteIndex[vertexIndex] <= 4u)
				{
					continue;
				}
				_SortBoneIndicesAndWeight(blendWriteIndex[vertexIndex], blendIndicesWeights[vertexIndex]);
			}
		}

		for (UINT indexVertex = 0u; indexVertex < numVertices; indexVertex++)
		{
			CHAR* tempVertex = &(verticesData[assetInfo.VertexStride * (indexVertex + subMeshInfo.VertexStart)]);
			if (assetInfo.VertexColor.Exist)
			{
				FLOAT* tempColor = (FLOAT*)(&(tempVertex[assetInfo.VertexColor.Offset]));
				aiColor4D& tempSceneMeshColor = colors[indexVertex];
				tempColor[0] = tempSceneMeshColor.r;
				tempColor[1] = tempSceneMeshColor.g;
				tempColor[2] = tempSceneMeshColor.b;
				tempColor[3] = tempSceneMeshColor.a;
			}
			if (assetInfo.VertexPosition.Exist)
			{
				FLOAT* tempPosition = (FLOAT*)(&(tempVertex[assetInfo.VertexPosition.Offset]));
				aiVector3D& tempSceneMeshPos = vertices[indexVertex];
				tempPosition[0] = tempSceneMeshPos.x;
				tempPosition[1] = tempSceneMeshPos.y;
				tempPosition[2] = tempSceneMeshPos.z;
				tempPosition[3] = 1.f;
			}
			if (assetInfo.VertexNormal.Exist)
			{
				FLOAT* tempNormal = (FLOAT*)(&(tempVertex[assetInfo.VertexNormal.Offset]));
				aiVector3D& tempSceneMeshNormal = normals[indexVertex];
				tempNormal[0] = tempSceneMeshNormal.x;
				tempNormal[1] = tempSceneMeshNormal.y;
				tempNormal[2] = tempSceneMeshNormal.z;
				tempNormal[3] = 0.f;
			}
			if (assetInfo.VertexTangent.Exist)
			{
				FLOAT* tempTangent = (FLOAT*)(&(tempVertex[assetInfo.VertexTangent.Offset]));
				aiVector3D& tempSceneMeshTangent = tangents[indexVertex];
				tempTangent[0] = tempSceneMeshTangent.x;
				tempTangent[1] = tempSceneMeshTangent.y;
				tempTangent[2] = tempSceneMeshTangent.z;
				tempTangent[3] = 0.f;
			}
			if (assetInfo.VertexUV.Exist)
			{
				FLOAT* tempUV = (FLOAT*)(&(tempVertex[assetInfo.VertexUV.Offset]));
				aiVector3D& tempSceneMeshUV = texcoords[indexVertex];
				tempUV[0] = tempSceneMeshUV.x;
				tempUV[1] = tempSceneMeshUV.y;
			}
			if (assetInfo.VertexBoneIndices.Exist)
			{
				USHORT* tempBoneIndices = (USHORT*)(&(tempVertex[assetInfo.VertexBoneIndices.Offset]));
				if (blendWriteIndex != nullptr && blendIndicesWeights.size() > 0)
				{
					std::vector<std::pair<USHORT, FLOAT>>& tempSceneMeshBoneIndices = blendIndicesWeights[indexVertex];
					UINT& tempSceneMeshBoneWriteIndex = blendWriteIndex[indexVertex];
					tempBoneIndices[0] = tempSceneMeshBoneWriteIndex > 0u ? tempSceneMeshBoneIndices[0].first : 0u;
					tempBoneIndices[1] = tempSceneMeshBoneWriteIndex > 1u ? tempSceneMeshBoneIndices[1].first : 0u;
					tempBoneIndices[2] = tempSceneMeshBoneWriteIndex > 2u ? tempSceneMeshBoneIndices[2].first : 0u;
					tempBoneIndices[3] = tempSceneMeshBoneWriteIndex > 3u ? tempSceneMeshBoneIndices[3].first : 0u;
				}
				else
				{
					tempBoneIndices[0] = 0u;
					tempBoneIndices[1] = 0u;
					tempBoneIndices[2] = 0u;
					tempBoneIndices[3] = 0u;
				}
			}
			if (assetInfo.VertexBoneWeight.Exist)
			{
				FLOAT* tempBoneWeights = (FLOAT*)(&(tempVertex[assetInfo.VertexBoneWeight.Offset]));
				if (blendWriteIndex != nullptr && blendIndicesWeights.size() > 0)
				{
					std::vector<std::pair<USHORT, FLOAT>>& tempSceneMeshBoneWeights = blendIndicesWeights[indexVertex];
					UINT& tempSceneMeshBoneWriteIndex = blendWriteIndex[indexVertex];
					tempBoneWeights[0] = tempSceneMeshBoneWriteIndex > 0u ? tempSceneMeshBoneWeights[0].second : 1.f;
					tempBoneWeights[1] = tempSceneMeshBoneWriteIndex > 1u ? tempSceneMeshBoneWeights[1].second : 0.f;
					tempBoneWeights[2] = tempSceneMeshBoneWriteIndex > 2u ? tempSceneMeshBoneWeights[2].second : 0.f;
					tempBoneWeights[3] = tempSceneMeshBoneWriteIndex > 3u ? tempSceneMeshBoneWeights[3].second : 0.f;
				}
				else
				{
					tempBoneWeights[0] = 1.f;
					tempBoneWeights[1] = 0.f;
					tempBoneWeights[2] = 0.f;
					tempBoneWeights[3] = 0.f;
				}
			}
		}

		if (blendWriteIndex != nullptr)
		{
			delete[]blendWriteIndex;
		}
	}
}

BOOL _GTranslateDefaultMeshData(const aiScene* scene, std::vector<CustomStruct::CSubMeshInfo>& subMesh, UINT& vertexStride, CHAR*& vertices, UINT& numVertices, std::vector<UINT>& indices, UINT& numIndices, const CustomStruct::CRenderInputLayoutDesc* inputLayoutDesc, const UINT& inputLayoutNum)
{
	vertexStride	= 0u;
	numVertices		= 0u;
	numIndices		= 0u;
	if (vertices != nullptr)
	{
		delete[]vertices;
		vertices = nullptr;
	}
	if (indices.size() > 0)
	{
		indices.clear();
	}

	std::vector<BOOL> sceneMeshesHasBones;
	std::vector<_GMeshAssetImporterInfo> sceneMeshesInfo;
	UINT totalNumVertices, totalNumIndices;
	if (!_GTranslateMeshDesc(scene, subMesh, sceneMeshesInfo, totalNumVertices, totalNumIndices, sceneMeshesHasBones, inputLayoutDesc, inputLayoutNum))
	{
		return FALSE;
	}

	{
		vertexStride = sceneMeshesInfo[0].VertexStride;
		numVertices = totalNumVertices;
		numIndices = totalNumIndices;
		vertices = new CHAR[totalNumVertices * vertexStride];
		indices.resize(totalNumIndices);
	}

	std::vector<CustomStruct::CGameBoneNodeInfo> tempSkeleton;
	std::vector<UINT> tempBoneList;

	UINT numMeshes = scene->mNumMeshes;
	UINT indexSubMesh = 0u;
	for (UINT indexMesh = 0u; indexMesh < numMeshes; indexMesh++)
	{
		const aiMesh* tempMesh = scene->mMeshes[indexMesh];
		if (tempMesh == nullptr)
		{
			continue;
		}
		_GMeshAssetImporterInfo& sceneMeshInfo = sceneMeshesInfo[indexSubMesh];
		CustomStruct::CSubMeshInfo& subMeshInfo = subMesh[indexSubMesh];
		_GTranslateMeshVertexData(tempMesh, sceneMeshInfo, subMeshInfo, vertices, indices, tempSkeleton, tempBoneList);
		indexSubMesh += 1u;
	}

	return TRUE;
}

BOOL _GTranslateSkeletonMeshData(const aiScene* scene, std::vector<CustomStruct::CSubMeshInfo>& subMesh, UINT& vertexStride, CHAR*& vertices, UINT& numVertices, std::vector<UINT>& indices, UINT& numIndices, const std::vector<CustomStruct::CGameBoneNodeInfo>& skeletonInput, const std::vector<UINT>& boneListInput, const CustomStruct::CRenderInputLayoutDesc* inputLayoutDesc, const UINT& inputLayoutNum)
{
	vertexStride	= 0u;
	numVertices		= 0u;
	numIndices		= 0u;
	if (vertices != nullptr)
	{
		delete[]vertices;
		vertices = nullptr;
	}
	if (indices.size() > 0)
	{
		indices.clear();
	}

	std::vector<BOOL> sceneMeshesHasBones;
	std::vector<_GMeshAssetImporterInfo> sceneMeshesInfo;
	UINT totalNumVertices, totalNumIndices;
	if (!_GTranslateMeshDesc(scene, subMesh, sceneMeshesInfo, totalNumVertices, totalNumIndices, sceneMeshesHasBones, inputLayoutDesc, inputLayoutNum))
	{
		return FALSE;
	}

	{
		vertexStride = sceneMeshesInfo[0].VertexStride;
		numVertices = totalNumVertices;
		numIndices = totalNumIndices;
		vertices = new CHAR[totalNumVertices * vertexStride];
		indices.resize(totalNumIndices);
	}

	UINT numMeshes = scene->mNumMeshes;
	UINT indexSubMesh = 0u;
	for (UINT indexMesh = 0u; indexMesh < numMeshes; indexMesh++)
	{
		const aiMesh* tempMesh = scene->mMeshes[indexMesh];
		if (tempMesh == nullptr)
		{
			continue;
		}
		_GMeshAssetImporterInfo& sceneMeshInfo = sceneMeshesInfo[indexSubMesh];
		CustomStruct::CSubMeshInfo& subMeshInfo = subMesh[indexSubMesh];
		_GTranslateMeshVertexData(tempMesh, sceneMeshInfo, subMeshInfo, vertices, indices, skeletonInput, boneListInput);
		indexSubMesh += 1u;
	}

	return TRUE;
}

const static std::string _GConstImporterNodeEmptyName	= "_ConstImporterNodeEmptyName";
const static std::string _GConstDummyName				= "_ConstDummyName";

struct _GImporterBoneNodeData
{
	_GImporterBoneNodeData()
	{
		this->Node			= nullptr;
		this->Bone			= nullptr;
		this->Index			= -2;
		this->Parent		= nullptr;
	}
	_GImporterBoneNodeData(const _GImporterBoneNodeData& v)
	{
		this->Node			= v.Node;
		this->Bone			= v.Bone;
		this->Index			= v.Index;
		this->Parent		= v.Parent;
		this->Name			= v.Name;
	}
	const aiNode*							Node;
	const aiBone*							Bone;
	INT										Index;
	_GImporterBoneNodeData*					Parent;
	std::string								Name;
};

void _GGatherSingleNodeRecursion(const aiNode* node, std::vector<const aiNode*>& outputNode, std::map<const aiNode*, INT>& outputParent)
{
	if (node == nullptr)
	{
		return;
	}

	outputNode.push_back(node);
	outputParent.insert_or_assign(node, -2);

	for (UINT indexChild = 0u; indexChild < node->mNumChildren; indexChild++)
	{
		_GGatherSingleNodeRecursion(node->mChildren[indexChild], outputNode, outputParent);
	}
}

BOOL _GGatherAllNodes(const aiNode* rootNode, std::vector<const aiNode*>& outputAllNodes, std::map<const aiNode*, INT>& outputNodeParent, std::map<std::string, _GImporterBoneNodeData>& output)
{
	if (outputAllNodes.size() > 0)
	{
		outputAllNodes.clear();
	}
	if (outputNodeParent.size() > 0)
	{
		outputNodeParent.clear();
	}
	if (output.size() > 0)
	{
		output.clear();
	}
	if (rootNode == nullptr)
	{
		return FALSE;
	}

	_GGatherSingleNodeRecursion(rootNode, outputAllNodes, outputNodeParent);

	if (outputAllNodes.size() == 0 || outputAllNodes.size() != outputNodeParent.size())
	{
		return FALSE;
	}

	const UINT numNodes = static_cast<UINT>(outputAllNodes.size());
	{
		auto findParentIndex = [](const aiNode* node, const std::vector<const aiNode*>& allNodes, const UINT& numNodes)->INT {
			if (node == nullptr)
			{
				return -2;
			}
			if (node->mParent == nullptr)
			{
				return -1;
			}
			for (UINT indexNode = 0u; indexNode < numNodes; indexNode++)
			{
				const aiNode* tempNode = allNodes[indexNode];
				if (tempNode == nullptr)
				{
					continue;
				}
				if (tempNode == node->mParent)
				{
					return static_cast<INT>(indexNode);
				}
			}
			return -2; };

		for (UINT indexNode = 0u; indexNode < numNodes; indexNode++)
		{
			const aiNode* tempNode = outputAllNodes[indexNode];
			if (tempNode == nullptr)
			{
				continue;
			}
			auto itNodeParent = outputNodeParent.find(tempNode);
			if (itNodeParent == outputNodeParent.end())
			{
				continue;
			}
			itNodeParent->second = findParentIndex(tempNode, outputAllNodes, numNodes);
		}
	}

	for (UINT indexNode = 0u; indexNode < numNodes; indexNode++)
	{
		const aiNode* tempNode = outputAllNodes[indexNode];
		if (tempNode == nullptr)
		{
			continue;
		}

		_GImporterBoneNodeData tempBoneNodeData;
		tempBoneNodeData.Node = tempNode;
		tempBoneNodeData.Bone = nullptr;
		tempBoneNodeData.Index = static_cast<INT>(indexNode);
		tempBoneNodeData.Parent = nullptr;

		std::string tempBoneNodeName;
		if (tempNode->mName.length < 1)
		{
			tempBoneNodeName = _GConstImporterNodeEmptyName;
		}
		else
		{
			tempBoneNodeName = _GTranslateString(tempNode->mName);
		}

		tempBoneNodeData.Name = tempBoneNodeName;

		if (output.find(tempBoneNodeData.Name) != output.end())
		{
			UINT tempDummyIndex = 0u;
			tempBoneNodeData.Name = tempBoneNodeName + "_" + std::to_string(tempDummyIndex);
			tempDummyIndex += 1u;
			while (output.find(tempBoneNodeData.Name) != output.end())
			{
				tempBoneNodeData.Name = tempBoneNodeName + "_" + std::to_string(tempDummyIndex);
				tempDummyIndex += 1u;
			}
		}

		output.insert_or_assign(tempBoneNodeData.Name, tempBoneNodeData);
	}

	for (auto itNode = output.begin(); itNode != output.end(); itNode++)
	{
		if (itNode->second.Node == nullptr)
		{
			continue;
		}
		auto itNodeParent = outputNodeParent.find(itNode->second.Node);
		if (itNodeParent == outputNodeParent.end())
		{
			continue;
		}
		if (itNodeParent->second == -2 || itNodeParent->second == -1)
		{
			continue;
		}
		for (auto findNodeParent = output.begin(); findNodeParent != output.end(); findNodeParent++)
		{
			if (findNodeParent->second.Index == itNodeParent->second)
			{
				itNode->second.Parent = &(findNodeParent->second);
			}
		}
	}

	return TRUE;
}

void _GGatherSingleNodeRecursion(const aiNode* node, _GImporterBoneNodeData* parentNode, std::map<std::string, std::vector<_GImporterBoneNodeData>>& output)
{
	if (node == nullptr)
	{
		return;
	}
	_GImporterBoneNodeData* currentNode = nullptr;
	{
		std::string nodeName = _GTranslateString(node->mName);
		{
			if (node->mName.length < 1)
			{
				nodeName = _GConstImporterNodeEmptyName;
			}
			std::map<std::string, std::vector<_GImporterBoneNodeData>>::iterator result = output.end();
			{
				result = output.find(nodeName);
				if (result == output.end())
				{
					std::pair<std::map<std::string, std::vector<_GImporterBoneNodeData>>::iterator, bool> tempResult = output.insert_or_assign(nodeName, std::vector<_GImporterBoneNodeData>());
					result = tempResult.first;
				}
			}
			if (result != output.end())
			{
				result->second.push_back(_GImporterBoneNodeData());
				currentNode = &(result->second[result->second.size() - 1]);
			}
		}
		if (currentNode != nullptr)
		{
			currentNode->Name = nodeName;
			currentNode->Node = node;
			currentNode->Parent = parentNode;
		}
	}
	for (UINT childIndex = 0u; childIndex < node->mNumChildren; childIndex++)
	{
		_GGatherSingleNodeRecursion(node->mChildren[childIndex], currentNode, output);
	}
}

void _GResortAllNodeRecursion(std::map<std::string, std::vector<_GImporterBoneNodeData>>& input, std::map<std::string, _GImporterBoneNodeData>& output)
{
	if (output.size() != 0)
	{
		output.clear();
	}

	{
		// Add into a new list that delete same name node and add dummy node.
		UINT dummyNodeIndex = 0u;
		for (auto it = input.begin(); it != input.end(); it++)
		{
			if (it->second.size() == 1)
			{
				_GImporterBoneNodeData& tempNodeData = (it->second)[0];
				output.insert_or_assign(tempNodeData.Name, _GImporterBoneNodeData(tempNodeData));
			}
			else if (it->second.size() > 1)
			{
				for (UINT nodeIndex = 0u; nodeIndex < static_cast<UINT>(it->second.size()); nodeIndex++)
				{
					_GImporterBoneNodeData& tempNodeData = (it->second)[nodeIndex];
					tempNodeData.Name = (_GConstDummyName + '_' + std::to_string(dummyNodeIndex));
					dummyNodeIndex += 1u;
					_GImporterBoneNodeData newNodeData(tempNodeData);
					output.insert_or_assign(newNodeData.Name, newNodeData);
				}
			}
		}
	}

	// Restore parent and child connection.
	for (auto it = output.begin(); it != output.end(); it++)
	{
		if (it->second.Parent != nullptr)
		{
			auto parentIt = output.find(it->second.Parent->Name);
			if (parentIt != output.end())
			{
				it->second.Parent = &(parentIt->second);
			}
			else
			{
				it->second.Parent = nullptr;
			}
		}
	}
}

void _GGatherAllNodes(const aiNode* root, std::map<std::string, _GImporterBoneNodeData>& output)
{
	std::map<std::string, std::vector<_GImporterBoneNodeData>> nodeDatas;
	_GGatherSingleNodeRecursion(root, nullptr, nodeDatas);
	_GResortAllNodeRecursion(nodeDatas, output);
}

void _GGatherAllBones(const aiScene* scene, std::map<std::string, const aiBone*>& output)
{
	if (output.size() != 0)
	{
		output.clear();
	}
	UINT meshNum = scene->mNumMeshes;
	for (UINT meshIndex = 0u; meshIndex < meshNum; meshIndex++)
	{
		const aiMesh* mesh = scene->mMeshes[meshIndex];
		if (mesh == nullptr || !mesh->HasBones())
		{
			continue;
		}
		UINT boneNum = mesh->mNumBones;
		for (UINT boneIndex = 0u; boneIndex < boneNum; boneIndex++)
		{
			const aiBone* bone = mesh->mBones[boneIndex];
			if (bone == nullptr)
			{
				continue;
			}
			output.insert_or_assign(_GTranslateString(bone->mName), bone);
		}
	}
}

BOOL _GGatherBoneDatas(const aiScene* scene, std::vector<CustomStruct::CGameBoneNodeInfo>& skeletonOutput, std::vector<UINT>& boneList, INT& skeletonRootNode)
{
	if (skeletonOutput.size() != 0)
	{
		skeletonOutput.clear();
	}
	if (boneList.size() != 0)
	{
		boneList.clear();
	}
	skeletonRootNode = -1;

	std::map<std::string, const aiBone*> boneMap;
	_GGatherAllBones(scene, boneMap);

	if (boneMap.size() < 1)
	{
		return FALSE;
	}

	std::map<std::string, _GImporterBoneNodeData> nodeMap;

#if 1

	{
		std::vector<const aiNode*> allNodes;
		std::map<const aiNode*, INT> allNodeParents;
		if (!_GGatherAllNodes(scene->mRootNode, allNodes, allNodeParents, nodeMap))
		{
			return FALSE;
		}
	}

#else

	_GGatherAllNodes(scene->mRootNode, nodeMap);

#endif

	for (auto it = nodeMap.begin(); it != nodeMap.end(); it++)
	{
		auto boneIt = boneMap.find(it->second.Name);
		if (boneIt != boneMap.end())
		{
			it->second.Bone = boneIt->second;
		}
	}

	{
		UINT boneIndex = 0u;
		for (auto it = nodeMap.begin(); it != nodeMap.end(); it++)
		{
			skeletonOutput.push_back(CustomStruct::CGameBoneNodeInfo(it->second.Name));
			if (it->second.Bone != nullptr)
			{
				boneList.push_back(boneIndex);
			}
			boneIndex += 1u;
		}
	}

	{
		UINT skeletonNodeNum = static_cast<UINT>(skeletonOutput.size());
		for (UINT nodeIndex = 0u; nodeIndex < skeletonNodeNum; nodeIndex++)
		{
			CustomStruct::CGameBoneNodeInfo& node = skeletonOutput[nodeIndex];

			auto it = nodeMap.find(node.Name);
			if (it == nodeMap.end())
			{
				continue;
			}

			_GTranslateMatrix(it->second.Node->mTransformation, node.Location, node.Rotation, node.Scale);

			if (it->second.Bone != nullptr)
			{
				node.Offset = _GTranslateMatrix(it->second.Bone->mOffsetMatrix);
			}

			if (it->second.Parent != nullptr)
			{
				for (UINT parentIndex = 0u; parentIndex < skeletonNodeNum; parentIndex++)
				{
					if (skeletonOutput[parentIndex].Name == it->second.Parent->Name)
					{
						node.Parent = &(skeletonOutput[parentIndex]);
						break;
					}
				}
			}
		}
		std::string rawRootNodeName(scene->mRootNode->mName.C_Str());
		for (UINT nodeIndex = 0u; nodeIndex < skeletonNodeNum; nodeIndex++)
		{
			CustomStruct::CGameBoneNodeInfo& node = skeletonOutput[nodeIndex];
			node.Index = static_cast<INT>(nodeIndex);
			if (skeletonRootNode == -1 && node.Name == rawRootNodeName)
			{
				skeletonRootNode = static_cast<INT>(nodeIndex);
			}
			if (node.Parent != nullptr)
			{
				node.Parent->Children.push_back(&(skeletonOutput[nodeIndex]));
			}
		}
	}

	return (boneList.size() > 0 && skeletonRootNode >= 0);
}

void _GTranslateAnimationBehaviour(const aiAnimBehaviour& input, CustomStruct::CGameAnimationBehaviour& output)
{
	if (input == aiAnimBehaviour::aiAnimBehaviour_DEFAULT)
	{
		output = CustomStruct::CGameAnimationBehaviour::ANIMATION_BEHAVIOUR_DEFAULT;
	}
	else if (input == aiAnimBehaviour::aiAnimBehaviour_CONSTANT)
	{
		output = CustomStruct::CGameAnimationBehaviour::ANIMATION_BEHAVIOUR_CONSTANT;
	}
	else if (input == aiAnimBehaviour::aiAnimBehaviour_LINEAR)
	{
		output = CustomStruct::CGameAnimationBehaviour::ANIMATION_BEHAVIOUR_LINEAR;
	}
	else if (input == aiAnimBehaviour::aiAnimBehaviour_REPEAT)
	{
		output = CustomStruct::CGameAnimationBehaviour::ANIMATION_BEHAVIOUR_REPEAT;
	}
}

void _GGatherSingleAnimationDatas(const aiAnimation* animationNode, CustomStruct::CGameAnimationInfo& animationData)
{
	if (animationData.AnimationNodes.size() > 0)
	{
		animationData.AnimationNodes.clear();
	}

	UINT numChannels = animationNode->mNumChannels;
	for (UINT indexChannel = 0u; indexChannel < numChannels; indexChannel++)
	{
		const aiNodeAnim* tempChannel = animationNode->mChannels[indexChannel];
		if (tempChannel == nullptr || tempChannel->mNodeName.length < 1)
		{
			continue;
		}

		CustomStruct::CGameAnimationNodeInfo tempAnimationNodeInfo(_GTranslateString(tempChannel->mNodeName));

		for (UINT indexPosKey = 0u; indexPosKey < tempChannel->mNumPositionKeys && tempChannel->mPositionKeys != nullptr; indexPosKey++)
		{
			const aiVectorKey& tempChannelPosKey = tempChannel->mPositionKeys[indexPosKey];

			CustomStruct::CGameAnimationKey<CustomType::Vector3> tempPosKey;
			tempPosKey.Time = tempChannelPosKey.mTime;
			tempPosKey.Value = _GTranslateVector3(tempChannelPosKey.mValue);

			tempAnimationNodeInfo.PositionKeys.push_back(tempPosKey);
		}

		for (UINT indexRotKey = 0u; indexRotKey < tempChannel->mNumRotationKeys && tempChannel->mRotationKeys != nullptr; indexRotKey++)
		{
			const aiQuatKey& tempChannelRotKey = tempChannel->mRotationKeys[indexRotKey];

			CustomStruct::CGameAnimationKey<CustomType::Quaternion> tempRotKey;
			tempRotKey.Time = tempChannelRotKey.mTime;
			tempRotKey.Value = _GTranslateQuaternion(tempChannelRotKey.mValue);

			tempAnimationNodeInfo.RotationKeys.push_back(tempRotKey);
		}

		for (UINT indexSclKey = 0u; indexSclKey < tempChannel->mNumScalingKeys && tempChannel->mScalingKeys != nullptr; indexSclKey++)
		{
			const aiVectorKey& tempChannelSclKey = tempChannel->mScalingKeys[indexSclKey];

			CustomStruct::CGameAnimationKey<CustomType::Vector3> tempSclKey;
			tempSclKey.Time = tempChannelSclKey.mTime;
			tempSclKey.Value = _GTranslateVector3(tempChannelSclKey.mValue);

			tempAnimationNodeInfo.ScalingKeys.push_back(tempSclKey);
		}

		_GTranslateAnimationBehaviour(tempChannel->mPreState, tempAnimationNodeInfo.PreState);
		_GTranslateAnimationBehaviour(tempChannel->mPostState, tempAnimationNodeInfo.PostState);

		// TODO is this necessary?
		{
			std::sort(tempAnimationNodeInfo.PositionKeys.begin(), tempAnimationNodeInfo.PositionKeys.end());
			std::sort(tempAnimationNodeInfo.RotationKeys.begin(), tempAnimationNodeInfo.RotationKeys.end());
			std::sort(tempAnimationNodeInfo.ScalingKeys.begin(), tempAnimationNodeInfo.ScalingKeys.end());
		}
		
		animationData.AnimationNodes.push_back(tempAnimationNodeInfo);
	}
}

BOOL _GGatherAllAnimationDatas(const std::string& path, const aiScene* scene, std::map<std::string, CustomStruct::CGameAnimationInfo>& animationDatas)
{
	if (animationDatas.size() > 0)
	{
		animationDatas.clear();
	}

	BOOL result = TRUE;

	UINT animationDummyIndex = 0u;
	UINT numAnimationNodes = scene->mNumAnimations;
	for (UINT indexAnimationNode = 0u; indexAnimationNode < numAnimationNodes; indexAnimationNode++)
	{
		const aiAnimation* tempAnimationNode = scene->mAnimations[indexAnimationNode];
		if (tempAnimationNode == nullptr)
		{
			continue;
		}

		std::string tempAnimationNodeName;

		if (tempAnimationNode->mName.length > 0)
		{
			tempAnimationNodeName = _GTranslateString(tempAnimationNode->mName);
		}
		else
		{
			tempAnimationNodeName = std::string("DummyAnimation_") + std::to_string(animationDummyIndex);
			animationDummyIndex += 1u;
		}

		{
			auto tempFindAnimation = animationDatas.find(tempAnimationNodeName);
			if (tempFindAnimation != animationDatas.end())
			{
				result = FALSE;
				continue;
			}
		}

		{
			auto tempCurrentAnimation = animationDatas.insert_or_assign(tempAnimationNodeName, CustomStruct::CGameAnimationInfo());
			CustomStruct::CGameAnimationInfo& tempAnimationData = tempCurrentAnimation.first->second;
			tempAnimationData.Name				= path + "_" + tempAnimationNodeName;
			tempAnimationData.Duration			= tempAnimationNode->mDuration;
			tempAnimationData.TicksPerSecond	= tempAnimationNode->mTicksPerSecond;
			_GGatherSingleAnimationDatas(tempAnimationNode, tempAnimationData);
		}
	}

	return result;
}

CassimpManager* CassimpManager::m_AssimpManager = nullptr;
void CassimpManager::Initialize()
{
	if (CassimpManager::m_AssimpManager == nullptr)
	{
		CassimpManager::m_AssimpManager = new CassimpManager();
	}
	if (_GAssetImporter == nullptr)
	{
		_GAssetImporter = new Assimp::Importer();
	}
}
void CassimpManager::ShutDown()
{
	if (_GAssetImporter != nullptr)
	{
		_GAssetImporter->FreeScene();
		delete (_GAssetImporter);
		_GAssetImporter = nullptr;
	}
	if (CassimpManager::m_AssimpManager != nullptr)
	{
		delete (CassimpManager::m_AssimpManager);
		CassimpManager::m_AssimpManager = nullptr;
	}
}
CassimpManager::CassimpReadFileState CassimpManager::ReadDefaultMeshFile(const std::string& path, std::vector<CustomStruct::CSubMeshInfo>& subMesh, UINT& vertexStride, CHAR*& vertices, UINT& numVertices, std::vector<UINT>& indices, UINT& numIndices)
{
	if (vertices != nullptr)
	{
		delete[]vertices;
		vertices = nullptr;
	}
	if (indices.size() > 0)
	{
		indices.clear();
	}
	if (subMesh.size() > 0)
	{
		subMesh.clear();
	}
	vertexStride	= 0u;
	numVertices		= 0u;
	numIndices		= 0u;

	CassimpReadFileState result = CassimpReadFileState::ASSIMP_READ_FILE_STATE_FAILED;

	Assimp::Importer* impoter = _GAssetImporter;
	if (impoter == nullptr)
	{
		// TODO Do the error logging (did not create the instance of importer)
		return result;
	}

	// And have it read the given file with some example postprocessing
	// Usually - if speed is not the most important aspect for you - you'll
	// probably to request more postprocessing than we do in this example.

	// Use SetPropertyInteger to modify config of importer
	//Assimp::Importer::SetProperty###();

	const aiScene* scene = impoter->ReadFile(
		path,
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
	if (scene == nullptr)
	{
		// TODO Do the error logging (importer.GetErrorString())
		impoter->FreeScene();
		return result;
	}

	if (!scene->HasMeshes())
	{
		result = CassimpReadFileState::ASSIMP_READ_FILE_STATE_ERROR;
		// TODO Scene does not contain meshes
		impoter->FreeScene();
		return result;
	}

	// Now we can access the file's contents.
	// Only access first mesh in scene.
	const CustomStruct::CRenderInputLayoutDesc* inputLayoutDesc; UINT inputLayoutNum;
	CustomStruct::CRenderInputLayoutDesc::GetEngineDefaultMeshInputLayouts(inputLayoutDesc, inputLayoutNum);
	if (!_GTranslateDefaultMeshData(scene, subMesh, vertexStride, vertices, numVertices, indices, numIndices, inputLayoutDesc, inputLayoutNum))
	{
		result = CassimpReadFileState::ASSIMP_READ_FILE_STATE_ERROR;
		impoter->FreeScene();
		return result;
	}

	result = CassimpReadFileState::ASSIMP_READ_FILE_STATE_SUCCEED;
	// We're done. Everything will be cleaned up by the importer destructor
	impoter->FreeScene();
	return result;
}
//void CopyNodesWithMeshes(aiNode node, SceneObject targetParent, Matrix4x4 accTransform)
//{
//	SceneObject parent;
//	Matrix4x4 transform;
//
//	// if node has meshes, create a new scene object for it
//	if (node.mNumMeshes > 0) {
//		SceneObjekt newObject = new SceneObject;
//		targetParent.addChild(newObject);
//		// copy the meshes
//		CopyMeshes(node, newObject);
//
//		// the new object is the parent for all child nodes
//		parent = newObject;
//		transform.SetUnity();
//	}
//	else {
//		// if no meshes, skip the node, but keep its transformation
//		parent = targetParent;
//		transform = node.mTransformation * accTransform;
//	}
//
//	// continue for all child nodes
//	for (all node.mChildren) {
//		CopyNodesWithMeshes(node.mChildren[a], parent, transform);
//	}
//}
CassimpManager::CassimpReadFileState CassimpManager::ReadSkeletonMeshAndBoneFile(const std::string& path, std::vector<CustomStruct::CSubMeshInfo>& subMesh, UINT& vertexStride, CHAR*& vertices, UINT& numVertices, std::vector<UINT>& indices, UINT& numIndices, std::vector<CustomStruct::CGameBoneNodeInfo>& skeleton, std::vector<UINT>& boneList, UINT& rootNode)
{
	if (vertices != nullptr)
	{
		delete[]vertices;
		vertices = nullptr;
	}
	if (indices.size() > 0)
	{
		indices.clear();
	}
	if (subMesh.size() > 0)
	{
		subMesh.clear();
	}
	vertexStride = 0u;
	numVertices = 0u;
	numIndices = 0u;

	if (skeleton.size() > 0)
	{
		skeleton.clear();
	}
	if (boneList.size() > 0)
	{
		boneList.clear();
	}
	rootNode = 0u;

	CassimpReadFileState result = CassimpReadFileState::ASSIMP_READ_FILE_STATE_FAILED;

	Assimp::Importer* impoter = _GAssetImporter;
	if (impoter == nullptr)
	{
		// TODO Do the error logging (did not create the instance of importer)
		return result;
	}

	// And have it read the given file with some example postprocessing
	// Usually - if speed is not the most important aspect for you - you'll
	// probably to request more postprocessing than we do in this example.

	// Use SetPropertyInteger to modify config of importer
	//Assimp::Importer::SetProperty###();

	const aiScene* scene = impoter->ReadFile(
		path,
		aiProcess_CalcTangentSpace |
		aiProcess_JoinIdenticalVertices |
		aiProcess_MakeLeftHanded |
		aiProcess_Triangulate |
		aiProcess_RemoveComponent |
		aiProcess_GenSmoothNormals |
		aiProcess_SplitLargeMeshes |
		/*aiProcess_LimitBoneWeights |*/
		aiProcess_RemoveRedundantMaterials |
		aiProcess_FixInfacingNormals |
		aiProcess_PopulateArmatureData |
		aiProcess_SortByPType |
		aiProcess_FindInvalidData |
		aiProcess_GenUVCoords |
		aiProcess_OptimizeMeshes |
		aiProcess_FlipUVs |
		aiProcess_FlipWindingOrder |
		/*aiProcess_SplitByBoneCount |*/
		/*aiProcess_Debone |*/
		aiProcess_GenBoundingBoxes);

	// If the import failed, report it
	if (scene == nullptr)
	{
		// TODO Do the error logging (importer.GetErrorString())
		impoter->FreeScene();
		return result;
	}

	if (!scene->HasMeshes())
	{
		result = CassimpReadFileState::ASSIMP_READ_FILE_STATE_ERROR;
		// TODO Do the error logging (importer.GetErrorString())
		impoter->FreeScene();
		return result;
	}

	// Now we can access the file's contents.
	INT tempRootNode = -1;
	result = _GGatherBoneDatas(scene, skeleton, boneList, tempRootNode) == TRUE ? CassimpReadFileState::ASSIMP_READ_FILE_STATE_SUCCEED : CassimpReadFileState::ASSIMP_READ_FILE_STATE_ERROR;

	if (result == CassimpReadFileState::ASSIMP_READ_FILE_STATE_SUCCEED)
	{
		rootNode = static_cast<UINT>(tempRootNode);
		const CustomStruct::CRenderInputLayoutDesc* inputLayoutDesc; UINT inputLayoutNum;
		CustomStruct::CRenderInputLayoutDesc::GetEngineSkeletonMeshInputLayouts(inputLayoutDesc, inputLayoutNum);
		result = _GTranslateSkeletonMeshData(scene, subMesh, vertexStride, vertices, numVertices, indices, numIndices, skeleton, boneList, inputLayoutDesc, inputLayoutNum) == TRUE ? CassimpReadFileState::ASSIMP_READ_FILE_STATE_SUCCEED : CassimpReadFileState::ASSIMP_READ_FILE_STATE_ERROR;
	}

	// We're done. Everything will be cleaned up by the importer destructor
	impoter->FreeScene();
	return result;
}
CassimpManager::CassimpReadFileState CassimpManager::ReadSkeletonAnimationFile(const std::string& path, std::map<std::string, std::map<std::string, CustomStruct::CGameAnimationInfo>>& animationDatas)
{
	CassimpReadFileState result = CassimpReadFileState::ASSIMP_READ_FILE_STATE_FAILED;

	if (animationDatas.find(path) != animationDatas.end())
	{
		return result;
	}

	Assimp::Importer* impoter = _GAssetImporter;
	if (impoter == nullptr)
	{
		// TODO Do the error logging (did not create the instance of importer)
		return result;
	}

	// And have it read the given file with some example postprocessing
	// Usually - if speed is not the most important aspect for you - you'll
	// probably to request more postprocessing than we do in this example.

	// Use SetPropertyInteger to modify config of importer
	//Assimp::Importer::SetProperty###();

	const aiScene* scene = impoter->ReadFile(
		path,
		aiProcess_MakeLeftHanded |
		aiProcess_RemoveComponent |
		aiProcess_RemoveRedundantMaterials |
		aiProcess_PopulateArmatureData);

	// If the import failed, report it
	if (scene == nullptr)
	{
		// TODO Do the error logging (importer.GetErrorString())
		impoter->FreeScene();
		return result;
	}

	if (!scene->HasAnimations())
	{
		result = CassimpReadFileState::ASSIMP_READ_FILE_STATE_ERROR;
		// TODO Do the error logging (importer.GetErrorString())
		impoter->FreeScene();
		return result;
	}

	// Skeleton reader
	//std::vector<CustomStruct::CGameBoneNodeInfo> sceneSkeleton;
	//std::vector<UINT> sceneBoneList;
	//UINT rootNode = 0u;
	//INT tempRootNode = -1;
	//result = _GGatherBoneDatas(scene, sceneSkeleton, sceneBoneList, tempRootNode);
	//rootNode = static_cast<UINT>(tempRootNode);

	// Now we can access the file's contents.
	auto tempInsertResult = animationDatas.insert_or_assign(path, std::map<std::string, CustomStruct::CGameAnimationInfo>());
	std::map<std::string, CustomStruct::CGameAnimationInfo>& tempAnimationContainer = tempInsertResult.first->second;

	result = _GGatherAllAnimationDatas(path, scene, tempAnimationContainer) == TRUE ? CassimpReadFileState::ASSIMP_READ_FILE_STATE_SUCCEED : CassimpReadFileState::ASSIMP_READ_FILE_STATE_ERROR;

	// We're done. Everything will be cleaned up by the importer destructor
	impoter->FreeScene();
	return result;
}