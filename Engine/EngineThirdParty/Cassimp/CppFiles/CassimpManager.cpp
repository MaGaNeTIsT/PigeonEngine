#include "../Headers/CassimpManager.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#define CUSTOM_IS_QNAN(f)	(f != f)

CassimpManager*				CassimpManager::m_AssimpManager	= nullptr;
static Assimp::Importer*	_GAssetImporter					= nullptr;
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
	if (!CassimpManager::TranslateMeshData<aiMesh>((scene->mMeshes[0u]), inputLayoutDesc, inputLayoutNum, meshVertices, numVertices, meshIndices, numIndices))
	{
		impoter->FreeScene();
		return FALSE;
	}

	// We're done. Everything will be cleaned up by the importer destructor
	impoter->FreeScene();
	return TRUE;
}
template<typename T>
BOOL CassimpManager::TranslateMeshData(const T* sceneMesh, const CustomStruct::CRenderInputLayoutDesc* inputLayoutDesc, const UINT& inputLayoutNum, CHAR*& verticesData, UINT& numVertices, std::vector<UINT>& indicesData, UINT& numIndices)
{
	const aiMesh* tempMesh = dynamic_cast<const aiMesh*>(sceneMesh);
	if (tempMesh == nullptr || !tempMesh->HasFaces())
	{
		return FALSE;
	}

	const static UINT constVertexColorIndex		= 0u;
	const static UINT constVertexTexcoordIndex	= 0u;

	BOOL hasColor, hasPosition, hasNormal, hasTangent, hasUV, hasBlendIndices, hasBlendWeight;
	hasColor = hasPosition = hasNormal = hasTangent = hasUV = hasBlendIndices = hasBlendWeight = FALSE;
	UINT stride, offsetColor, offsetPosition, offsetNormal, offsetTangent, offsetUV, offsetBlendIndices, offsetBlendWeight;
	stride = offsetColor = offsetPosition = offsetNormal = offsetTangent = offsetUV = offsetBlendIndices = offsetBlendWeight = 0u;

	for (UINT i = 0u; i < inputLayoutNum; i++)
	{
		CustomStruct::CRenderInputLayoutDesc desc(inputLayoutDesc[i]);
		if (desc.SemanticName == CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_COLOR)
		{
			if (!tempMesh->HasVertexColors(constVertexColorIndex))
			{
				return FALSE;
			}
			hasColor = TRUE;
			offsetColor = stride;
		}
		else if (desc.SemanticName == CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_POSITION)
		{
			if (!tempMesh->HasPositions())
			{
				return FALSE;
			}
			hasPosition = TRUE;
			offsetPosition = stride;
		}
		else if (desc.SemanticName == CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_NORMAL)
		{
			if (!tempMesh->HasNormals())
			{
				return FALSE;
			}
			hasNormal = TRUE;
			offsetNormal = stride;
		}
		else if (desc.SemanticName == CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_TANGENT)
		{
			if (!tempMesh->HasTangentsAndBitangents())
			{
				return FALSE;
			}
			hasTangent = TRUE;
			offsetTangent = stride;
		}
		else if (desc.SemanticName == CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_TEXCOORD)
		{
			if (!tempMesh->HasTextureCoords(constVertexTexcoordIndex))
			{
				return FALSE;
			}
			hasUV = TRUE;
			offsetUV = stride;
		}
		else if (desc.SemanticName == CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_BLENDINDICES)
		{
			if (!tempMesh->HasBones())
			{
				return FALSE;
			}
			hasBlendIndices = TRUE;
			offsetBlendIndices = stride;
		}
		else if (desc.SemanticName == CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_BLENDWEIGHT)
		{
			if (!tempMesh->HasBones())
			{
				return FALSE;
			}
			hasBlendWeight = TRUE;
			offsetBlendWeight = stride;
		}
		stride += desc.GetSemanticSizeByByte();
	}

	numVertices = tempMesh->mNumVertices;
	verticesData = new CHAR[stride * tempMesh->mNumVertices];
	numIndices = tempMesh->mNumFaces * 3u;
	indicesData.resize(tempMesh->mNumFaces * 3u);

	{
		UINT numFaces = tempMesh->mNumFaces;
		aiFace* faces = tempMesh->mFaces;
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
		aiString sceneMeshName = tempMesh->mName;
		aiColor4D* colors = tempMesh->mColors[constVertexColorIndex];
		aiVector3D* vertices = tempMesh->mVertices;
		aiVector3D* normals = tempMesh->mNormals;
		aiVector3D* tangents = tempMesh->mTangents;
		aiVector3D* texcoords = tempMesh->mTextureCoords[constVertexTexcoordIndex];
		aiBone** bones = tempMesh->mBones;
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