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

inline void _GTranslateMeshData(const aiMesh* mesh, const _GMeshAssetImporterInfo& assetInfo, const CustomStruct::CSubMeshInfo& subMeshInfo, CHAR*& verticesData, std::vector<UINT>& indicesData)
{
	const static UINT constVertexColorIndex = 0u;
	const static UINT constVertexTexcoordIndex = 0u;

	{
		UINT numFaces = mesh->mNumFaces;
		aiFace* faces = mesh->mFaces;
		for (UINT indexFace = 0; indexFace < numFaces; indexFace++)
		{
			aiFace& tempFace = faces[indexFace];
			for (UINT i = 0; i < 3; i++)
			{
				indicesData[subMeshInfo.IndexStart + (indexFace * 3 + i)] = tempFace.mIndices[i];
			}
		}
	}

	{
		UINT numVertices = mesh->mNumVertices;

		aiColor4D* colors = mesh->mColors[constVertexColorIndex];
		aiVector3D* vertices = mesh->mVertices;
		aiVector3D* normals = mesh->mNormals;
		aiVector3D* tangents = mesh->mTangents;
		aiVector3D* texcoords = mesh->mTextureCoords[constVertexTexcoordIndex];

		aiBone** bones = mesh->mBones;

		for (UINT indexVertex = 0; indexVertex < numVertices; indexVertex++)
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
				//TODO
			}
			if (assetInfo.VertexBoneWeight.Exist)
			{
				//TODO
			}
		}
	}
}

inline BOOL _GTranslateMeshDesc(const aiScene* scene, std::vector<CustomStruct::CSubMeshInfo>& subMesh, UINT& vertexStride, CHAR*& vertices, UINT& numVertices, std::vector<UINT>& indices, UINT& numIndices, const CustomStruct::CRenderInputLayoutDesc* inputLayoutDesc, const UINT& inputLayoutNum)
{
	const static UINT constVertexColorIndex		= 0u;
	const static UINT constVertexTexcoordIndex	= 0u;

	vertexStride	= 0u;
	numVertices		= 0u;
	numIndices		= 0u;
	if (vertices != nullptr)
	{
		delete vertices;
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

	if (scene == nullptr || !scene->HasMeshes())
	{
		return FALSE;
	}

	UINT numMeshes = scene->mNumMeshes;

	subMesh.resize(numMeshes);
	std::vector<_GMeshAssetImporterInfo> sceneMeshesInfo;

	for (UINT indexMesh = 0; indexMesh < numMeshes; indexMesh++)
	{
		const aiMesh* tempMesh = scene->mMeshes[indexMesh];

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

	{
		vertexStride = sceneMeshesInfo[0].VertexStride;
		numVertices = totalNumVertices;
		numIndices = totalNumIndices;
		vertices = new CHAR[totalNumVertices * vertexStride];
		indices.resize(totalNumIndices);
	}

	for (UINT indexMesh = 0; indexMesh < numMeshes; indexMesh++)
	{
		const aiMesh* tempMesh = scene->mMeshes[indexMesh];
		_GMeshAssetImporterInfo& sceneMeshInfo = sceneMeshesInfo[indexMesh];
		CustomStruct::CSubMeshInfo& subMeshInfo = subMesh[indexMesh];
		_GTranslateMeshData(tempMesh, sceneMeshInfo, subMeshInfo, vertices, indices);
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
BOOL CassimpManager::ReadDefaultMeshFile(const std::string& path, std::vector<CustomStruct::CSubMeshInfo>& subMesh, UINT& vertexStride, CHAR*& vertices, UINT& numVertices, std::vector<UINT>& indices, UINT& numIndices)
{
	if (vertices != nullptr)
	{
		delete vertices;
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
	if (!_GTranslateMeshDesc(scene, subMesh, vertexStride, vertices, numVertices, indices, numIndices, inputLayoutDesc, inputLayoutNum))
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
