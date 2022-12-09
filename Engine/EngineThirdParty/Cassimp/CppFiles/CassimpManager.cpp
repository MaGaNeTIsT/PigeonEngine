#include "../Headers/CassimpManager.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#define CUSTOM_IS_QNAN(f)	(f != f)

static Assimp::Importer* _GAssetImporter = nullptr;

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

inline BOOL _GTranslateMeshDesc(const aiScene* sceneMesh, std::vector<CustomStruct::CSubMeshInfo>& subMesh, const CustomStruct::CRenderInputLayoutDesc* inputLayoutDesc, const UINT& inputLayoutNum)
{
	const static UINT constVertexColorIndex		= 0u;
	const static UINT constVertexTexcoordIndex	= 0u;

	if (subMesh.size() > 0)
	{
		subMesh.clear();
	}

	if (sceneMesh == nullptr || !sceneMesh->HasMeshes())
	{
		return FALSE;
	}

	UINT numMeshes = sceneMesh->mNumMeshes;

	subMesh.resize(numMeshes);
	std::vector<_GMeshAssetImporterInfo> sceneMeshesInfo;

	for (UINT indexMesh = 0; indexMesh < numMeshes; indexMesh++)
	{
		const aiMesh* tempMesh = sceneMesh->mMeshes[indexMesh];

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
				if (!tempMesh->HasBones())
				{
					return FALSE;
				}
				tempPerMeshInfo.VertexBoneIndices.Exist = TRUE;
				tempPerMeshInfo.VertexBoneIndices.Offset = tempPerMeshInfo.VertexStride;
			}
			else if (desc.SemanticName == CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_BLENDWEIGHT)
			{
				if (!tempMesh->HasBones())
				{
					return FALSE;
				}
				tempPerMeshInfo.VertexBoneWeight.Exist = TRUE;
				tempPerMeshInfo.VertexBoneWeight.Offset = tempPerMeshInfo.VertexStride;
			}
			tempPerMeshInfo.VertexStride += desc.GetSemanticSizeByByte();
		}

		sceneMeshesInfo.push_back(tempPerMeshInfo);
	}

	UINT totalNumVertices = 0u;
	UINT totalNumIndices = 0u;

	for (UINT indexSubMesh = 0; indexSubMesh < numMeshes; indexSubMesh++)
	{
		CustomStruct::CSubMeshInfo& subMeshInfo		= subMesh[indexSubMesh];
		_GMeshAssetImporterInfo&	sceneMeshInfo	= sceneMeshesInfo[indexSubMesh];

		subMeshInfo.IndexStart	= totalNumIndices;
		subMeshInfo.IndexCount	= sceneMeshInfo.NumIndices;
		subMeshInfo.VertexStart = totalNumVertices;
		subMeshInfo.VertexCount = sceneMeshInfo.NumVertices;

		totalNumVertices	+= sceneMeshInfo.NumVertices;
		totalNumIndices		+= sceneMeshInfo.NumIndices;
	}



	return TRUE;
}

inline BOOL _GTranslateMeshData(const aiMesh* sceneMesh, const _GMeshAssetImporterInfo& assetInfo, CHAR*& startVerticesData, std::vector<UINT>& startIndicesData)
{
	const static UINT constVertexColorIndex		= 0u;
	const static UINT constVertexTexcoordIndex	= 0u;

	if (sceneMesh == nullptr || !sceneMesh->HasFaces())
	{
		return FALSE;
	}

	BOOL hasColor, hasPosition, hasNormal, hasTangent, hasUV, hasBlendIndices, hasBlendWeight;
	hasColor = hasPosition = hasNormal = hasTangent = hasUV = hasBlendIndices = hasBlendWeight = FALSE;
	UINT stride, offsetColor, offsetPosition, offsetNormal, offsetTangent, offsetUV, offsetBlendIndices, offsetBlendWeight;
	stride = offsetColor = offsetPosition = offsetNormal = offsetTangent = offsetUV = offsetBlendIndices = offsetBlendWeight = 0u;

	for (UINT i = 0u; i < inputLayoutNum; i++)
	{
		CustomStruct::CRenderInputLayoutDesc desc(inputLayoutDesc[i]);
		if (desc.SemanticName == CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_COLOR)
		{
			if (!sceneMesh->HasVertexColors(constVertexColorIndex))
			{
				return FALSE;
			}
			hasColor = TRUE;
			offsetColor = stride;
		}
		else if (desc.SemanticName == CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_POSITION)
		{
			if (!sceneMesh->HasPositions())
			{
				return FALSE;
			}
			hasPosition = TRUE;
			offsetPosition = stride;
		}
		else if (desc.SemanticName == CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_NORMAL)
		{
			if (!sceneMesh->HasNormals())
			{
				return FALSE;
			}
			hasNormal = TRUE;
			offsetNormal = stride;
		}
		else if (desc.SemanticName == CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_TANGENT)
		{
			if (!sceneMesh->HasTangentsAndBitangents())
			{
				return FALSE;
			}
			hasTangent = TRUE;
			offsetTangent = stride;
		}
		else if (desc.SemanticName == CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_TEXCOORD)
		{
			if (!sceneMesh->HasTextureCoords(constVertexTexcoordIndex))
			{
				return FALSE;
			}
			hasUV = TRUE;
			offsetUV = stride;
		}
		else if (desc.SemanticName == CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_BLENDINDICES)
		{
			if (!sceneMesh->HasBones())
			{
				return FALSE;
			}
			hasBlendIndices = TRUE;
			offsetBlendIndices = stride;
		}
		else if (desc.SemanticName == CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_BLENDWEIGHT)
		{
			if (!sceneMesh->HasBones())
			{
				return FALSE;
			}
			hasBlendWeight = TRUE;
			offsetBlendWeight = stride;
		}
		stride += desc.GetSemanticSizeByByte();
	}

	numVertices = sceneMesh->mNumVertices;
	verticesData = new CHAR[stride * sceneMesh->mNumVertices];
	numIndices = sceneMesh->mNumFaces * 3u;
	indicesData.resize(sceneMesh->mNumFaces * 3u);

	{
		UINT numFaces = sceneMesh->mNumFaces;
		aiFace* faces = sceneMesh->mFaces;
		for (UINT indexFace = 0; indexFace < numFaces; indexFace++)
		{
			aiFace& tempFace = faces[indexFace];
			for (UINT i = 0; i < 3; i++)
			{
				indicesData[indexFace * 3 + i] = tempFace.mIndices[i];
			}
		}
	}

	{
		aiString sceneMeshName = sceneMesh->mName;
		aiColor4D* colors = sceneMesh->mColors[constVertexColorIndex];
		aiVector3D* vertices = sceneMesh->mVertices;
		aiVector3D* normals = sceneMesh->mNormals;
		aiVector3D* tangents = sceneMesh->mTangents;
		aiVector3D* texcoords = sceneMesh->mTextureCoords[constVertexTexcoordIndex];
		aiBone** bones = sceneMesh->mBones;
		for (UINT indexVertex = 0; indexVertex < numVertices; indexVertex++)
		{
			CHAR* tempVertex = &(verticesData[stride * indexVertex]);
			if (hasColor)
			{
				FLOAT* tempColor = (FLOAT*)(&(tempVertex[offsetColor]));
				aiColor4D& tempSceneMeshColor = colors[indexVertex];
				tempColor[0] = tempSceneMeshColor.r;
				tempColor[1] = tempSceneMeshColor.g;
				tempColor[2] = tempSceneMeshColor.b;
				tempColor[3] = tempSceneMeshColor.a;
			}
			if (hasPosition)
			{
				FLOAT* tempPosition = (FLOAT*)(&(tempVertex[offsetPosition]));
				aiVector3D& tempSceneMeshPos = vertices[indexVertex];
				tempPosition[0] = tempSceneMeshPos.x;
				tempPosition[1] = tempSceneMeshPos.y;
				tempPosition[2] = tempSceneMeshPos.z;
				tempPosition[3] = 1.f;
			}
			if (hasNormal)
			{
				FLOAT* tempNormal = (FLOAT*)(&(tempVertex[offsetNormal]));
				aiVector3D& tempSceneMeshNormal = normals[indexVertex];
				tempNormal[0] = tempSceneMeshNormal.x;
				tempNormal[1] = tempSceneMeshNormal.y;
				tempNormal[2] = tempSceneMeshNormal.z;
				tempNormal[3] = 0.f;
			}
			if (hasTangent)
			{
				FLOAT* tempTangent = (FLOAT*)(&(tempVertex[offsetTangent]));
				aiVector3D& tempSceneMeshTangent = tangents[indexVertex];
				tempTangent[0] = tempSceneMeshTangent.x;
				tempTangent[1] = tempSceneMeshTangent.y;
				tempTangent[2] = tempSceneMeshTangent.z;
				tempTangent[3] = 0.f;
			}
			if (hasUV)
			{
				FLOAT* tempUV = (FLOAT*)(&(tempVertex[offsetUV]));
				aiVector3D& tempSceneMeshUV = texcoords[indexVertex];
				tempUV[0] = tempSceneMeshUV.x;
				tempUV[1] = tempSceneMeshUV.y;
			}
			if (hasBlendIndices)
			{
				//TODO
			}
			if (hasBlendWeight)
			{
				//TODO
			}
		}
	}
	return TRUE;
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
		_GAssetImporter = new  Assimp::Importer();
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
BOOL CassimpManager::ReadDefaultMeshFile(const std::string& path, CHAR*& meshVertices, UINT& numVertices, std::vector<UINT>& meshIndices, UINT& numIndices)
{
	if (meshVertices != nullptr)
	{
		delete meshVertices;
		meshVertices = nullptr;
	}
	if (meshIndices.size() > 0)
	{
		meshIndices.clear();
	}
	numVertices		= 0u;
	numIndices		= 0u;

	Assimp::Importer* impoter = _GAssetImporter;
	if (impoter == nullptr)
	{
		// TODO Do the error logging (did not create the instance of importer)
		return FALSE;
	}

	// And have it read the given file with some example postprocessing
	// Usually - if speed is not the most important aspect for you - you'll
	// probably to request more postprocessing than we do in this example.

	// Use SetPropertyInteger to modify config of importer
	//Assimp::Importer::SetProperty###();

	const aiScene* scene = impoter->ReadFile(
		path,
		aiProcess_Triangulate |
		aiProcess_CalcTangentSpace |
		aiProcess_MakeLeftHanded |
		aiProcess_FlipWindingOrder |
		aiProcess_FlipUVs |
		aiProcess_JoinIdenticalVertices |
		aiProcess_RemoveComponent |
		aiProcess_GenSmoothNormals |
		aiProcess_SplitLargeMeshes |
		aiProcess_ImproveCacheLocality |
		aiProcess_FixInfacingNormals |
		aiProcess_FindInvalidData |
		aiProcess_GenUVCoords |
		aiProcess_SortByPType |
		aiProcess_OptimizeMeshes |
		aiProcess_OptimizeGraph);

	// If the import failed, report it
	if (scene == nullptr)
	{
		// TODO Do the error logging (importer.GetErrorString())
		return FALSE;
	}

	if (!scene->HasMeshes())
	{
		impoter->FreeScene();
		// TODO Scene does not contain meshes
		return FALSE;
	}

	// Now we can access the file's contents.
	// Only access first mesh in scene.
	const CustomStruct::CRenderInputLayoutDesc* inputLayoutDesc; UINT inputLayoutNum;
	CustomStruct::CRenderInputLayoutDesc::GetEngineDefaultMeshInputLayouts(inputLayoutDesc, inputLayoutNum);
	if (TRUE)
	{
		impoter->FreeScene();
		return FALSE;
	}

	// We're done. Everything will be cleaned up by the importer destructor
	impoter->FreeScene();
	return TRUE;
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
