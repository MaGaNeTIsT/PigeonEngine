#include "../Headers/CFuxkingWhySkeletonAnimation.h"
#include "../../EngineBase/Headers/CBaseType.h"
#include "../../EngineRender/RenderBase/Headers/CRenderStructCommon.h"
#include "../../EngineRender/RenderBase/Headers/CRenderDevice.h"

void ReadSceneMeshTransform(const aiNode* node, DirectX::XMFLOAT4X4& globalMat)
{
	if (node == nullptr)
	{
		return;
	}

	{
		DirectX::XMMATRIX nodeMat = DirectX::XMMATRIX(&(node->mTransformation.a1));
		DirectX::XMMATRIX tempGlobalMat = nodeMat * DirectX::XMLoadFloat4x4(&(globalMat));
		DirectX::XMStoreFloat4x4(&globalMat, tempGlobalMat);
	}

	{
		ReadSceneMeshTransform(node->mParent, globalMat);
	}
}

void ReadSceneBoneTransform(const aiNode* node, std::map<std::string, CFuxkingWhySkeletonAnimation::CFuxkingWhyBone*>* savedBoneMap, DirectX::XMFLOAT4X4& globalMat)
{
	if (savedBoneMap->size() == 0 || node == nullptr)
	{
		return;
	}

	{
		DirectX::XMMATRIX nodeMat = DirectX::XMMATRIX(&(node->mTransformation.a1));
		DirectX::XMMATRIX tempGlobalMat = DirectX::XMLoadFloat4x4(&(globalMat)) * nodeMat;
		DirectX::XMStoreFloat4x4(&globalMat, tempGlobalMat);
	}

	if (node->mName.length > 0)
	{
		std::string nodeName(node->mName.C_Str());
		auto tempItBone = savedBoneMap->find(nodeName);
		if (tempItBone != savedBoneMap->end())
		{
			tempItBone->second->BoneMatrix = globalMat;
		}
	}

	for (UINT i = 0u; i < node->mNumChildren; i++)
	{
		ReadSceneBoneTransform(node->mChildren[i], savedBoneMap, globalMat);
	}
}

void ReadSceneMesh(const aiScene* scene, const aiNode* node,
	std::map<std::string, CFuxkingWhySkeletonAnimation::CFuxkingWhyBone*>* boneMap,
	std::vector<CFuxkingWhySkeletonAnimation::CFuxkingWhyBone>* boneList,
	std::vector<CFuxkingWhySkeletonAnimation::CFuxkingWhyMesh>* meshes)
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
				USHORT* tempMeshVexBoneNum = new USHORT[tempSceneMesh->mNumVertices];
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
						DirectX::XMStoreFloat4x4(&(tempBone.BindPoseMatrix), DirectX::XMMATRIX(&(tempSceneMeshBone->mOffsetMatrix.a1)));
						DirectX::XMStoreFloat4x4(&(tempBone.BoneMatrix), DirectX::XMMatrixIdentity());
						boneList->push_back(tempBone);
						CFuxkingWhySkeletonAnimation::CFuxkingWhyBone* tempPtrBone = &((*boneList)[tempBone.Index]);
						boneMap->insert_or_assign(tempSceneMeshBoneName, tempPtrBone);
					}
					auto tempWriteBoneData = boneMap->find(tempSceneMeshBoneName);
					for (UINT tempIndexSceneMeshBoneWeight = 0u; tempIndexSceneMeshBoneWeight < tempSceneMeshBone->mNumWeights; tempIndexSceneMeshBoneWeight++)
					{
						const aiVertexWeight& tempSceneMeshBoneWeight = tempSceneMeshBone->mWeights[tempIndexSceneMeshBoneWeight];
						USHORT tempIndexVexBone = tempMeshVexBoneNum[tempSceneMeshBoneWeight.mVertexId];
						if (tempIndexVexBone < 4u)
						{

							tempMeshVertices[tempSceneMeshBoneWeight.mVertexId].BnI[tempIndexVexBone] = static_cast<USHORT>(tempWriteBoneData->second->Index);
							tempMeshVertices[tempSceneMeshBoneWeight.mVertexId].BnW[tempIndexVexBone] = tempSceneMeshBoneWeight.mWeight;
						}
						tempMeshVexBoneNum[tempSceneMeshBoneWeight.mVertexId] += 1u;
					}
				}
				delete[]tempMeshVexBoneNum;
			}

			{
				CFuxkingWhySkeletonAnimation::CFuxkingWhyMesh mesh;
				mesh.IndexCount = tempSceneMesh->mNumFaces * 3u;
				ReadSceneMeshTransform(node, mesh.MeshTransformInvMatrix);
				DirectX::XMStoreFloat4x4(&(mesh.MeshTransformInvMatrix), DirectX::XMMatrixInverse(nullptr, DirectX::XMLoadFloat4x4(&(mesh.MeshTransformInvMatrix))));
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
			}

			delete[]tempMeshVertices;
			delete[]tempMeshIndices;
		}
	}

	for (UINT i = 0u; i < node->mNumChildren; i++)
	{
		ReadSceneMesh(scene, node->mChildren[i], boneMap, boneList, meshes);
	}
}

CFuxkingWhySkeletonAnimation::CFuxkingWhySkeletonAnimation()
{
	DirectX::XMMATRIX Identity = DirectX::XMMatrixIdentity();
	DirectX::XMStoreFloat4x4(&(this->m_PerDrawCBufferData.WorldMatrix), Identity);
	DirectX::XMStoreFloat4x4(&(this->m_PerDrawCBufferData.WorldInvMatrix), Identity);
	DirectX::XMStoreFloat4x4(&(this->m_PerDrawCBufferData.WorldInvTransposeMatrix), Identity);

	this->m_BoneCBufferData.SkeletonBoneNum = DirectX::XMFLOAT4(0.f, 0.f, 0.f, 0.f);
	for (UINT i = 0u; i < 576u; i++)
	{
		DirectX::XMStoreFloat4x4(&(this->m_BoneCBufferData.SkeletonMatrix[i]), Identity);
	}

	this->m_AssimpScene = nullptr;
}
CFuxkingWhySkeletonAnimation::~CFuxkingWhySkeletonAnimation()
{

}
void CFuxkingWhySkeletonAnimation::Init()
{
	std::string skeletonMeshPath = "./Engine/Assets/EngineModels/SceneModels/Misaki/Misaki_SchoolUniform.fbx";

	std::string skeletonAnimationPath = "./Engine/Assets/EngineModels/SceneModels/Misaki/Misaki_SchoolUniform.fbx";

	DirectX::XMFLOAT3 objPos = DirectX::XMFLOAT3(0.f, 600.f, -600.f);
	DirectX::XMFLOAT3 objRot = DirectX::XMFLOAT3(0.f, 0.f, 0.f);
	DirectX::XMFLOAT3 objScl = DirectX::XMFLOAT3(10.f, 10.f, 10.f);

	{
		if (this->m_Meshes.size() > 0)
		{
			this->m_Meshes.clear();
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
		if (this->m_AssimpScene != nullptr)
		{
			this->m_AssimpImpoter.FreeScene();
			this->m_AssimpScene = nullptr;
		}
		this->m_AssimpScene = this->m_AssimpImpoter.ReadFile(skeletonMeshPath, aiProcessPreset_TargetRealtime_MaxQuality | aiProcess_ConvertToLeftHanded);
		if (this->m_AssimpScene == nullptr || !this->m_AssimpScene->HasMeshes())
		{
			this->m_AssimpImpoter.FreeScene();
			this->m_AssimpScene = nullptr;
			return;
		}
		ReadSceneMesh(this->m_AssimpScene, this->m_AssimpScene->mRootNode, &(this->m_BoneMap), &(m_BoneList), &(m_Meshes));
		DirectX::XMFLOAT4X4 dummyMat;
		DirectX::XMStoreFloat4x4(&dummyMat, DirectX::XMMatrixIdentity());
		ReadSceneBoneTransform(this->m_AssimpScene->mRootNode, &(this->m_BoneMap), dummyMat);
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
}
void CFuxkingWhySkeletonAnimation::Update()
{

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
		for (UINT i = 0u; i < numBones && i < 576u; i++)
		{
			CFuxkingWhyBone& tempBone = this->m_BoneList[i];
			DirectX::XMMATRIX meshInvMat = DirectX::XMMATRIX(&(this->m_AssimpScene->mRootNode->mTransformation.Inverse().a1));
			//DirectX::XMLoadFloat4x4(&(tempMesh.MeshTransformInvMatrix))
			DirectX::XMMATRIX boneMat = meshInvMat * DirectX::XMLoadFloat4x4(&(tempBone.BoneMatrix)) * DirectX::XMLoadFloat4x4(&(tempBone.BindPoseMatrix));
			DirectX::XMStoreFloat4x4(&(this->m_BoneCBufferData.SkeletonMatrix[i]), boneMat);
			DirectX::XMStoreFloat4x4(&(this->m_BoneCBufferData.SkeletonMatrix[this->m_BoneList.size() + i]), DirectX::XMMatrixTranspose(DirectX::XMMatrixInverse(nullptr, boneMat)));
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
}