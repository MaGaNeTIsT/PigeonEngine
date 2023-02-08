#include "../Headers/CFuxkingWhySkeletonAnimation.h"
#include "../../EngineBase/Headers/CBaseType.h"
#include "../../EngineRender/RenderBase/Headers/CRenderDevice.h"

void ReadFindNodeByName(const aiNode* currentNode, const aiString& findName, BOOL& successFind, const aiNode*& outputNode)
{
	if (currentNode == nullptr || successFind == TRUE)
	{
		return;
	}

	if (currentNode->mName == findName)
	{
		outputNode = currentNode;
		successFind = TRUE;
	}

	for (UINT i = 0u; i < currentNode->mNumChildren; i++)
	{
		ReadFindNodeByName(currentNode->mChildren[i], findName, successFind, outputNode);
	}
}

void ReadNodeToRootTransform(const aiNode* node, aiMatrix4x4& globalMat)
{
	if (node == nullptr)
	{
		return;
	}

	{
		aiMatrix4x4 nodeTransformation = node->mTransformation;
		globalMat = nodeTransformation * globalMat;
	}

	ReadNodeToRootTransform(node->mParent, globalMat);
}

void ReadSceneBoneTransform(const aiNode* node, std::map<std::string, UINT>* savedBoneMap, std::vector<CFuxkingWhySkeletonAnimation::CFuxkingWhyBone>* savedBoneList, DirectX::XMMATRIX globalMat)
{
	if (savedBoneMap->size() == 0 || node == nullptr)
	{
		return;
	}

	DirectX::XMMATRIX tempGlobalMat = DirectX::XMMATRIX(&(node->mTransformation.a1));
	tempGlobalMat = globalMat * tempGlobalMat;

	if (node->mName.length > 0)
	{
		std::string nodeName(node->mName.C_Str());
		auto tempItBone = savedBoneMap->find(nodeName);
		if (tempItBone != savedBoneMap->end())
		{
			DirectX::XMStoreFloat4x4(&((*savedBoneList)[tempItBone->second].BoneMatrix), tempGlobalMat);
		}
	}

	for (UINT i = 0u; i < node->mNumChildren; i++)
	{
		ReadSceneBoneTransform(node->mChildren[i], savedBoneMap, savedBoneList, tempGlobalMat);
	}
}

void ReadSceneAnimTransform(const aiNode* node, const aiAnimation* anim, std::map<std::string, UINT>* savedBoneMap, std::vector<CFuxkingWhySkeletonAnimation::CFuxkingWhyBone>* savedBoneList, DirectX::XMMATRIX globalMat, FLOAT& t)
{
	if (savedBoneMap->size() == 0 || node == nullptr || anim == nullptr)
	{
		return;
	}

	DirectX::XMMATRIX tempGlobalMatrix = globalMat;

	if (node->mName.length > 0)
	{
		DirectX::XMMATRIX nodeMat = DirectX::XMMATRIX(&(node->mTransformation.a1));
		{
			const aiNodeAnim* nodeAnim = nullptr;
			for (UINT i = 0; i < anim->mNumChannels; i++)
			{
				const aiNodeAnim* tempNodeAnim = anim->mChannels[i];
				if (tempNodeAnim->mNodeName == node->mName)
				{
					nodeAnim = tempNodeAnim;
					break;
				}
			}
			if (nodeAnim != nullptr)
			{
				aiVector3D scl(1.f, 1.f, 1.f), pos(0.f, 0.f, 0.f);
				aiQuaternion rot(1.f, 0.f, 0.f, 0.f);

				BOOL successFind;
				DOUBLE current_t = static_cast<DOUBLE>(t) * anim->mDuration;
				if (nodeAnim->mNumPositionKeys > 0u)
				{
					successFind = FALSE;
					for (UINT iKey = 0u; iKey < (nodeAnim->mNumPositionKeys - 1u); iKey++)
					{
						const aiVectorKey& curKey = nodeAnim->mPositionKeys[iKey];
						const aiVectorKey& nexKey = nodeAnim->mPositionKeys[iKey + 1u];
						if (curKey.mTime <= current_t && current_t < nexKey.mTime)
						{
							DOUBLE temp_t = nexKey.mTime - curKey.mTime;
							temp_t = (current_t - curKey.mTime) / temp_t;
							{
								aiVector3D tempPos = nexKey.mValue;
								tempPos *= static_cast<FLOAT>(temp_t);
								pos = curKey.mValue;
								pos *= static_cast<FLOAT>(static_cast<DOUBLE>(1) - temp_t);
								pos += tempPos;
							}
							successFind = TRUE;
							break;
						}
					}
					if (successFind == FALSE)
					{
						pos = nodeAnim->mPositionKeys[nodeAnim->mNumPositionKeys - 1u].mValue;
					}
				}
				if (nodeAnim->mNumRotationKeys > 0u)
				{
					successFind = FALSE;
					for (UINT iKey = 0u; iKey < (nodeAnim->mNumRotationKeys - 1u); iKey++)
					{
						const aiQuatKey& curKey = nodeAnim->mRotationKeys[iKey];
						const aiQuatKey& nexKey = nodeAnim->mRotationKeys[iKey + 1u];
						if (curKey.mTime <= current_t && current_t < nexKey.mTime)
						{
							DOUBLE temp_t = nexKey.mTime - curKey.mTime;
							temp_t = (current_t - curKey.mTime) / temp_t;
							aiQuaternion::Interpolate(rot, curKey.mValue, nexKey.mValue, static_cast<FLOAT>(temp_t));
							successFind = TRUE;
							break;
						}
					}
					if (successFind == FALSE)
					{
						rot = nodeAnim->mRotationKeys[nodeAnim->mNumRotationKeys - 1u].mValue;
					}
				}
				if (nodeAnim->mNumScalingKeys > 0u)
				{
					successFind = FALSE;
					for (UINT iKey = 0u; iKey < (nodeAnim->mNumScalingKeys - 1u); iKey++)
					{
						const aiVectorKey& curKey = nodeAnim->mScalingKeys[iKey];
						const aiVectorKey& nexKey = nodeAnim->mScalingKeys[iKey + 1u];
						if (curKey.mTime <= current_t && current_t < nexKey.mTime)
						{
							DOUBLE temp_t = nexKey.mTime - curKey.mTime;
							temp_t = (current_t - curKey.mTime) / temp_t;
							{
								aiVector3D tempScl = nexKey.mValue;
								tempScl *= static_cast<FLOAT>(temp_t);
								scl = curKey.mValue;
								scl *= static_cast<FLOAT>(static_cast<DOUBLE>(1) - temp_t);
								scl += tempScl;
							}
							successFind = TRUE;
							break;
						}
					}
					if (successFind == FALSE)
					{
						scl = nodeAnim->mScalingKeys[nodeAnim->mNumScalingKeys - 1u].mValue;
					}
				}

				nodeMat = DirectX::XMMatrixAffineTransformation(
					DirectX::XMVectorSet(scl.x, scl.y, scl.z, 1.f),
					DirectX::XMVectorSet(0.f, 0.f, 0.f, 0.f),
					DirectX::XMVectorSet(rot.x, rot.y, rot.z, rot.w),
					DirectX::XMVectorSet(pos.x, pos.y, pos.z, 1.f));
				nodeMat = DirectX::XMMatrixTranspose(nodeMat);
			}
		}

		tempGlobalMatrix = globalMat * nodeMat;

		{
			std::string nodeName(node->mName.C_Str());
			auto tempItBone = savedBoneMap->find(nodeName);
			if (tempItBone != savedBoneMap->end())
			{
				DirectX::XMStoreFloat4x4(&((*savedBoneList)[tempItBone->second].BoneMatrix), tempGlobalMatrix);
			}
		}
	}

	for (UINT i = 0u; i < node->mNumChildren; i++)
	{
		ReadSceneAnimTransform(node->mChildren[i], anim, savedBoneMap, savedBoneList, tempGlobalMatrix, t);
	}
}

void ReadSceneMesh(const aiScene* scene, const aiNode* node, std::map<std::string, UINT>* boneMap, std::vector<CFuxkingWhySkeletonAnimation::CFuxkingWhyBone>* boneList, std::vector<CFuxkingWhySkeletonAnimation::CFuxkingWhyMesh>* meshes, std::vector<const aiMesh*>* assMeshes)
{
	if (node == nullptr)
	{
		return;
	}

	if (node->mNumMeshes > 0u && node->mMeshes != nullptr)
	{
		for (UINT indexMesh = 0u; indexMesh < node->mNumMeshes; indexMesh++)
		{
			const aiMesh* tempSceneMesh = scene->mMeshes[node->mMeshes[indexMesh]];
			if (tempSceneMesh == nullptr || !tempSceneMesh->HasFaces() || !tempSceneMesh->HasPositions())
			{
				continue;
			}
			UINT* tempMeshIndices = nullptr;
			if (tempSceneMesh->mNumFaces > 0 && tempSceneMesh->mFaces != nullptr)
			{
				tempMeshIndices = new UINT[tempSceneMesh->mNumFaces * 3u];
				const aiFace* tempFaces = tempSceneMesh->mFaces;
				for (UINT tempIndexFace = 0u; tempIndexFace < tempSceneMesh->mNumFaces; tempIndexFace++)
				{
					const aiFace& tempFace = tempFaces[tempIndexFace];
					for (UINT i = 0u; i < 3u; i++)
					{
						tempMeshIndices[tempIndexFace * 3u + i] = tempFace.mIndices[i];
					}
				}
			}
			CFuxkingWhySkeletonAnimation::CFuxkingWhyMeshVertex* tempMeshVertices = nullptr;
			if (tempMeshIndices != nullptr)
			{
				tempMeshVertices = new CFuxkingWhySkeletonAnimation::CFuxkingWhyMeshVertex[tempSceneMesh->mNumVertices];
				for (UINT tempIndexVex = 0u; tempIndexVex < tempSceneMesh->mNumVertices; tempIndexVex++)
				{
					tempMeshVertices[tempIndexVex].Pos[0] = tempSceneMesh->mVertices[tempIndexVex].x;
					tempMeshVertices[tempIndexVex].Pos[1] = tempSceneMesh->mVertices[tempIndexVex].y;
					tempMeshVertices[tempIndexVex].Pos[2] = tempSceneMesh->mVertices[tempIndexVex].z;
					tempMeshVertices[tempIndexVex].Pos[3] = 1.f;
					if (tempSceneMesh->HasNormals())
					{
						tempMeshVertices[tempIndexVex].Nml[0] = tempSceneMesh->mNormals[tempIndexVex].x;
						tempMeshVertices[tempIndexVex].Nml[1] = tempSceneMesh->mNormals[tempIndexVex].y;
						tempMeshVertices[tempIndexVex].Nml[2] = tempSceneMesh->mNormals[tempIndexVex].z;
						tempMeshVertices[tempIndexVex].Nml[3] = 0.f;
					}
					else
					{
						tempMeshVertices[tempIndexVex].Nml[0] = 0.f;
						tempMeshVertices[tempIndexVex].Nml[1] = 1.f;
						tempMeshVertices[tempIndexVex].Nml[2] = 0.f;
						tempMeshVertices[tempIndexVex].Nml[3] = 0.f;
					}
					if (tempSceneMesh->HasNormals())
					{
						tempMeshVertices[tempIndexVex].Nml[0] = tempSceneMesh->mNormals[tempIndexVex].x;
						tempMeshVertices[tempIndexVex].Nml[1] = tempSceneMesh->mNormals[tempIndexVex].y;
						tempMeshVertices[tempIndexVex].Nml[2] = tempSceneMesh->mNormals[tempIndexVex].z;
						tempMeshVertices[tempIndexVex].Nml[3] = 0.f;
					}
					else
					{
						tempMeshVertices[tempIndexVex].Nml[0] = 0.f;
						tempMeshVertices[tempIndexVex].Nml[1] = 1.f;
						tempMeshVertices[tempIndexVex].Nml[2] = 0.f;
						tempMeshVertices[tempIndexVex].Nml[3] = 0.f;
					}
					tempMeshVertices[tempIndexVex].BnI[0] = 0u;
					tempMeshVertices[tempIndexVex].BnI[1] = 0u;
					tempMeshVertices[tempIndexVex].BnI[2] = 0u;
					tempMeshVertices[tempIndexVex].BnI[3] = 0u;
					tempMeshVertices[tempIndexVex].BnW[0] = 1.f;
					tempMeshVertices[tempIndexVex].BnW[1] = 0.f;
					tempMeshVertices[tempIndexVex].BnW[2] = 0.f;
					tempMeshVertices[tempIndexVex].BnW[3] = 0.f;
				}
			}
			if (tempSceneMesh->HasBones() && tempMeshVertices != nullptr)
			{
				std::vector<USHORT> tempMeshVexBoneNum;
				tempMeshVexBoneNum.resize(tempSceneMesh->mNumVertices);
				for (UINT i = 0u; i < tempSceneMesh->mNumVertices; i++)
				{
					tempMeshVexBoneNum[i] = 0u;
				}
				for (UINT tempIndexSceneMeshBone = 0u; tempIndexSceneMeshBone < tempSceneMesh->mNumBones; tempIndexSceneMeshBone++)
				{
					const aiBone* tempSceneMeshBone = tempSceneMesh->mBones[tempIndexSceneMeshBone];
					if (tempSceneMeshBone == nullptr || tempSceneMeshBone->mName.length == 0)
					{
						continue;
					}
					std::string tempSceneMeshBoneName(tempSceneMeshBone->mName.C_Str());
					if (boneMap->find(tempSceneMeshBoneName) == boneMap->end())
					{
						CFuxkingWhySkeletonAnimation::CFuxkingWhyBone tempBone;
						tempBone.Index = static_cast<UINT>(boneList->size());
						tempBone.Name = tempSceneMeshBoneName;
						aiMatrix4x4 tempOffsetMatrix = tempSceneMeshBone->mOffsetMatrix;
#if 1
						DirectX::XMStoreFloat4x4(&(tempBone.BindPoseMatrix), DirectX::XMMATRIX(&(tempOffsetMatrix.a1)));
#else
						{
							aiVector3D scl, pos; aiQuaternion rot;
							tempOffsetMatrix.Decompose(scl, rot, pos);
							DirectX::XMMATRIX tempM = DirectX::XMMatrixAffineTransformation(
								DirectX::XMVectorSet(scl.x, scl.y, scl.z, 1.f),
								DirectX::XMVectorSet(0.f, 0.f, 0.f, 0.f),
								DirectX::XMVectorSet(rot.x, rot.y, rot.z, rot.w),
								DirectX::XMVectorSet(pos.x, pos.y, pos.z, 1.f));
							tempM = DirectX::XMMatrixTranspose(tempM);
							DirectX::XMStoreFloat4x4(&(tempBone.BindPoseMatrix), tempM);
						}
#endif
						DirectX::XMStoreFloat4x4(&(tempBone.BoneMatrix), DirectX::XMMatrixIdentity());
						boneList->push_back(tempBone);
						CFuxkingWhySkeletonAnimation::CFuxkingWhyBone* tempPtrBone = &((*boneList)[tempBone.Index]);
						boneMap->insert_or_assign(tempSceneMeshBoneName, tempBone.Index);
					}
					auto tempWriteBoneData = boneMap->find(tempSceneMeshBoneName);
#if 0
					{
						const aiNode* tempBoneNode = nullptr; BOOL successFind = FALSE;
						ReadFindNodeByName(scene->mRootNode, aiString(tempWriteBoneData->first), successFind, tempBoneNode);
						aiMatrix4x4 dummyMat;
						ReadNodeToRootTransform(tempBoneNode, dummyMat);
						dummyMat.Inverse();
					}
#endif
					for (UINT tempIndexSceneMeshBoneWeight = 0u; tempIndexSceneMeshBoneWeight < tempSceneMeshBone->mNumWeights; tempIndexSceneMeshBoneWeight++)
					{
						const aiVertexWeight& tempSceneMeshBoneWeight = tempSceneMeshBone->mWeights[tempIndexSceneMeshBoneWeight];
						USHORT tempIndexVexBone = tempMeshVexBoneNum[tempSceneMeshBoneWeight.mVertexId];
						if (tempIndexVexBone < 4u)
						{

							tempMeshVertices[tempSceneMeshBoneWeight.mVertexId].BnI[tempIndexVexBone] = static_cast<USHORT>(tempWriteBoneData->second);
							tempMeshVertices[tempSceneMeshBoneWeight.mVertexId].BnW[tempIndexVexBone] = tempSceneMeshBoneWeight.mWeight;
						}
						tempMeshVexBoneNum[tempSceneMeshBoneWeight.mVertexId] += 1u;
					}
				}
			}

			{
				CFuxkingWhySkeletonAnimation::CFuxkingWhyMesh mesh;
				mesh.IndexCount = tempSceneMesh->mNumFaces * 3u;
				BOOL successVB = CRenderDevice::CreateBuffer(mesh.VertexBuffer,
					CustomStruct::CRenderBufferDesc(
						static_cast<UINT>(sizeof(CFuxkingWhySkeletonAnimation::CFuxkingWhyMeshVertex) * tempSceneMesh->mNumVertices),
						CustomStruct::CRenderBindFlag::BIND_VERTEX_BUFFER, 0u),
					&CustomStruct::CRenderSubresourceData(static_cast<const void*>(tempMeshVertices), 0u, 0u));
				BOOL successIB = CRenderDevice::CreateBuffer(mesh.IndexBuffer,
					CustomStruct::CRenderBufferDesc(
						static_cast<UINT>(sizeof(UINT) * tempSceneMesh->mNumFaces * 3u),
						CustomStruct::CRenderBindFlag::BIND_INDEX_BUFFER, 0u),
					&CustomStruct::CRenderSubresourceData(static_cast<const void*>(tempMeshIndices), 0u, 0u));
				meshes->push_back(mesh);
				assMeshes->push_back(tempSceneMesh);
			}

			delete[]tempMeshVertices;
			delete[]tempMeshIndices;
		}
	}

	for (UINT i = 0u; i < node->mNumChildren; i++)
	{
		ReadSceneMesh(scene, node->mChildren[i], boneMap, boneList, meshes, assMeshes);
	}
}

CFuxkingWhySkeletonAnimation::CFuxkingWhySkeletonAnimation()
{
	DirectX::XMMATRIX Identity = DirectX::XMMatrixIdentity();
	DirectX::XMStoreFloat4x4(&(this->m_PerDrawCBufferData.WorldMatrix), Identity);
	DirectX::XMStoreFloat4x4(&(this->m_PerDrawCBufferData.WorldInvMatrix), Identity);
	DirectX::XMStoreFloat4x4(&(this->m_PerDrawCBufferData.WorldInvTransposeMatrix), Identity);

	this->m_BoneCBufferData.SkeletonBoneNum = DirectX::XMFLOAT4(0.f, 0.f, 0.f, 0.f);
	for (UINT i = 0u; i < (CustomStruct::CRenderBaseSetting::RenderBoneMaxNum * 2u); i++)
	{
		DirectX::XMStoreFloat4x4(&(this->m_BoneCBufferData.SkeletonMatrix[i]), Identity);
	}

	this->m_AssimpScene = nullptr;
	this->m_AssimpAnimScene = nullptr;
	this->m_AnimationT = 0.f;
}
CFuxkingWhySkeletonAnimation::~CFuxkingWhySkeletonAnimation()
{

}
void CFuxkingWhySkeletonAnimation::Init()
{
	std::string skeletonMeshPath = "./Engine/Assets/EngineModels/SceneModels/UnrealCharacter/SK_Mannequin_UE4_WithWeapon.FBX";

	UINT skeletonMeshReadState = aiProcessPreset_TargetRealtime_MaxQuality | aiProcess_ConvertToLeftHanded;

	std::string skeletonAnimationPath = "./Engine/Assets/EngineModels/SceneModels/UnrealCharacter/Boss_Idle.FBX";

	UINT skeletonAnimationReadState = aiProcessPreset_TargetRealtime_MaxQuality | aiProcess_MakeLeftHanded;

	DirectX::XMFLOAT3 objPos = DirectX::XMFLOAT3(0.f, 600.f, -600.f);
	DirectX::XMFLOAT3 objRot = DirectX::XMFLOAT3(90.f, 0.f, 0.f);
	DirectX::XMFLOAT3 objScl = DirectX::XMFLOAT3(0.5f, 0.5f, 0.5f);

	{
		if (this->m_Meshes.size() > 0)
		{
			this->m_Meshes.clear();
		}
		if (this->m_BoneMap.size() > 0)
		{
			this->m_BoneMap.clear();
		}
		if (this->m_BoneList.size() > 0)
		{
			this->m_BoneList.clear();
		}
	}

	{
		CustomStruct::CRenderInputLayoutDesc inputLayoutDesc[4] =
		{
			CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_POSITION),
			CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_NORMAL),
			CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_BLENDINDICES),
			CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_BLENDWEIGHT)
		};
		BOOL successVS = CRenderDevice::LoadVertexShader("./Engine/Assets/EngineShaders/TestVS.cso", this->m_VertexShader, this->m_InputLayout, inputLayoutDesc, 4u);
		BOOL successPS = CRenderDevice::LoadPixelShader("./Engine/Assets/EngineShaders/TestPS.cso", this->m_PixelShader);
	}

	{
		if (this->m_AssimpMeshes.size() > 0)
		{
			this->m_AssimpMeshes.clear();
		}
		if (this->m_AssimpScene != nullptr)
		{
			this->m_AssimpImpoter.FreeScene();
			this->m_AssimpScene = nullptr;
		}
		this->m_AssimpScene = this->m_AssimpImpoter.ReadFile(skeletonMeshPath, skeletonMeshReadState);
		if (this->m_AssimpScene == nullptr || !this->m_AssimpScene->HasMeshes())
		{
			this->m_AssimpImpoter.FreeScene();
			this->m_AssimpScene = nullptr;
			return;
		}

		{
			CFuxkingWhyBone rootDummyBone;
			rootDummyBone.Index = 0u;
			rootDummyBone.Name = "_RootDummyBone";
			DirectX::XMStoreFloat4x4(&(rootDummyBone.BindPoseMatrix), DirectX::XMMatrixIdentity());
			DirectX::XMStoreFloat4x4(&(rootDummyBone.BoneMatrix), DirectX::XMMatrixIdentity());
			this->m_BoneList.push_back(rootDummyBone);
			this->m_BoneMap.insert_or_assign(rootDummyBone.Name, rootDummyBone.Index);
		}
		ReadSceneMesh(this->m_AssimpScene, this->m_AssimpScene->mRootNode, &(this->m_BoneMap), &(this->m_BoneList), &(this->m_Meshes), &(this->m_AssimpMeshes));
		DirectX::XMMATRIX dummyMat = DirectX::XMMatrixIdentity();
		ReadSceneBoneTransform(this->m_AssimpScene->mRootNode, &(this->m_BoneMap), &(this->m_BoneList), dummyMat);
	}

	{
		DirectX::XMMATRIX worldMat = DirectX::XMMatrixScaling(objScl.x, objScl.y, objScl.z) * DirectX::XMMatrixRotationRollPitchYaw(objRot.x * CustomType::CMath::GetDegToRad(), objRot.y * CustomType::CMath::GetDegToRad(), objRot.z * CustomType::CMath::GetDegToRad()) * DirectX::XMMatrixTranslation(objPos.x, objPos.y, objPos.z);
		DirectX::XMMATRIX worldInvMat = DirectX::XMMatrixInverse(nullptr, worldMat);
		DirectX::XMStoreFloat4x4(&(this->m_PerDrawCBufferData.WorldMatrix), DirectX::XMMatrixTranspose(worldMat));
		DirectX::XMStoreFloat4x4(&(this->m_PerDrawCBufferData.WorldInvMatrix), DirectX::XMMatrixTranspose(worldInvMat));
		DirectX::XMStoreFloat4x4(&(this->m_PerDrawCBufferData.WorldInvTransposeMatrix), worldInvMat);

		this->m_BoneCBufferData.SkeletonBoneNum = DirectX::XMFLOAT4(static_cast<FLOAT>(this->m_BoneList.size()), 0.f, 0.f, 0.f);

		BOOL successPerDrarw = CRenderDevice::CreateBuffer(this->m_PerDrawCBuffer, CustomStruct::CRenderBufferDesc(sizeof(CFuxkingWhyPerDrawCBuffer), CustomStruct::CRenderBindFlag::BIND_CONSTANT_BUFFER, sizeof(FLOAT)));
		BOOL successBone = CRenderDevice::CreateBuffer(this->m_BoneCBuffer, CustomStruct::CRenderBufferDesc(sizeof(CFuxkingWhyBoneCBuffer), CustomStruct::CRenderBindFlag::BIND_CONSTANT_BUFFER, sizeof(FLOAT)));
	}

	{
		if (this->m_AssimpAnims.size() > 0)
		{
			this->m_AssimpAnims.clear();
		}
		if (this->m_AssimpAnimScene != nullptr)
		{
			this->m_AssimpAnimImpoter.FreeScene();
			this->m_AssimpAnimScene = nullptr;
		}
		this->m_AssimpAnimScene = this->m_AssimpAnimImpoter.ReadFile(skeletonAnimationPath, skeletonAnimationReadState);
		if (this->m_AssimpAnimScene == nullptr || !this->m_AssimpAnimScene->HasAnimations())
		{
			this->m_AssimpAnimImpoter.FreeScene();
			this->m_AssimpAnimScene = nullptr;
			return;
		}
		for (UINT indexSceneAnim = 0u; indexSceneAnim < this->m_AssimpAnimScene->mNumAnimations; indexSceneAnim++)
		{
			this->m_AssimpAnims.push_back(this->m_AssimpAnimScene->mAnimations[indexSceneAnim]);
		}
	}
}
void CFuxkingWhySkeletonAnimation::Update()
{
#if 1
	if (this->m_AssimpAnims.size() > 0 && this->m_BoneMap.size() > 0)
	{
		{
			FLOAT t = this->m_AnimationT;
			ImGui::Begin("FuxkingWhySkeletonAnimation");
			ImGui::SliderFloat("AnimationTime", &t, 0.f, 1.f, "%.2f", ImGuiSliderFlags_::ImGuiSliderFlags_AlwaysClamp);
			ImGui::End();
			this->m_AnimationT = t;
		}
		FLOAT t = this->m_AnimationT;
		DirectX::XMMATRIX dummyMat = DirectX::XMMatrixIdentity();
		ReadSceneAnimTransform(this->m_AssimpAnimScene->mRootNode, nullptr, &(this->m_BoneMap), &(this->m_BoneList), dummyMat, t);//this->m_AssimpAnims[0]
	}
#endif
}
void CFuxkingWhySkeletonAnimation::Draw()
{
	UINT vertexStride = sizeof(CFuxkingWhySkeletonAnimation::CFuxkingWhyMeshVertex);
	CRenderDevice::SetInputLayout(this->m_InputLayout);
	CRenderDevice::SetVSShader(this->m_VertexShader);
	CRenderDevice::SetPSShader(this->m_PixelShader);
	CRenderDevice::UploadBuffer(this->m_PerDrawCBuffer, static_cast<const void*>(&(this->m_PerDrawCBufferData)));
	CRenderDevice::BindVSConstantBuffer(this->m_PerDrawCBuffer, 2u);
	CRenderDevice::BindPSConstantBuffer(this->m_PerDrawCBuffer, 2u);
	const UINT numMeshes = static_cast<UINT>(this->m_Meshes.size());
	for (UINT indexMesh = 0u; indexMesh < numMeshes; indexMesh++)
	{
		CFuxkingWhyMesh& tempMesh = this->m_Meshes[indexMesh];
		CRenderDevice::SetVertexBuffer(tempMesh.VertexBuffer, vertexStride);
		CRenderDevice::SetIndexBuffer(tempMesh.IndexBuffer);
		const UINT numBones = static_cast<UINT>(this->m_BoneList.size());
		for (UINT i = 0u; i < numBones && (numBones + i) < (CustomStruct::CRenderBaseSetting::RenderBoneMaxNum * 2u); i++)
		{
			CFuxkingWhyBone& tempBone = this->m_BoneList[i];

			//DirectX::XMMATRIX meshInvMat = DirectX::XMLoadFloat4x4(&(tempMesh.MeshTransformInvMatrix));
			DirectX::XMMATRIX meshInvMat = DirectX::XMMATRIX(&(this->m_AssimpScene->mRootNode->mTransformation.Inverse().a1));
			//DirectX::XMMATRIX meshInvMat = DirectX::XMMatrixInverse(nullptr, DirectX::XMMatrixRotationAxis(DirectX::XMVectorSet(1.f, 0.f, 0.f, 0.f), 90.f * CustomType::CMath::GetDegToRad()));
			//DirectX::XMMATRIX meshInvMat = DirectX::XMMatrixRotationAxis(DirectX::XMVectorSet(1.f, 0.f, 0.f, 0.f), 90.f * CustomType::CMath::GetDegToRad());
			
			//aiMatrix4x4 tempBP = tempBone.BindPoseMatrix; aiMatrix4x4 tempB = tempBone.BoneMatrix;
			//tempBP = tempB * tempBP;
			//DirectX::XMMATRIX boneMat = DirectX::XMMATRIX(&(tempBP.a1));
			//DirectX::XMMATRIX boneMat = DirectX::XMLoadFloat4x4(&(tempBone.BindPoseMatrix)) * DirectX::XMLoadFloat4x4(&(tempBone.BoneMatrix)) * meshInvMat;
			//DirectX::XMMATRIX boneMat = meshInvMat * DirectX::XMMATRIX(&(tempBone.BoneMatrix.a1)) * DirectX::XMMATRIX(&(tempBone.BindPoseMatrix.a1));
			//DirectX::XMMATRIX boneMat = meshInvMat * DirectX::XMLoadFloat4x4(&(tempBone.BindPoseMatrix)) * DirectX::XMLoadFloat4x4(&(tempBone.BoneMatrix));
			DirectX::XMMATRIX boneMat = meshInvMat * DirectX::XMLoadFloat4x4(&(tempBone.BoneMatrix)) * DirectX::XMLoadFloat4x4(&(tempBone.BindPoseMatrix));

			DirectX::XMStoreFloat4x4(&(this->m_BoneCBufferData.SkeletonMatrix[i]), boneMat);
			DirectX::XMStoreFloat4x4(&(this->m_BoneCBufferData.SkeletonMatrix[numBones + i]), DirectX::XMMatrixTranspose(DirectX::XMMatrixInverse(nullptr, boneMat)));
			//DirectX::XMStoreFloat4x4(&(this->m_BoneCBufferData.SkeletonMatrix[i]), DirectX::XMMatrixTranspose(boneMat));
			//DirectX::XMStoreFloat4x4(&(this->m_BoneCBufferData.SkeletonMatrix[this->m_BoneList.size() + i]), DirectX::XMMatrixInverse(nullptr, boneMat));
		}
		CRenderDevice::UploadBuffer(this->m_BoneCBuffer, static_cast<const void*>(&(this->m_BoneCBufferData)));
		CRenderDevice::BindVSConstantBuffer(this->m_BoneCBuffer, 4u);
		CRenderDevice::DrawIndexed(tempMesh.IndexCount);
	}
}
void CFuxkingWhySkeletonAnimation::Uninit()
{
	if (this->m_AssimpMeshes.size() > 0)
	{
		this->m_AssimpMeshes.clear();
	}
	if (this->m_AssimpScene != nullptr)
	{
		this->m_AssimpImpoter.FreeScene();
		this->m_AssimpScene = nullptr;
	}
	if (this->m_AssimpAnimScene != nullptr)
	{
		this->m_AssimpAnimImpoter.FreeScene();
		this->m_AssimpAnimScene = nullptr;
	}
}