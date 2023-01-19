#include "../Headers/CSkeletonComponent.h"
#include "../../EngineRender/RenderBase/Headers/CRenderStructCommon.h"
#include "../../EngineRender/RenderBase/Headers/CRenderDevice.h"
#include "../Headers/CGameObject.h"

CSkeletonSingleBone::CSkeletonSingleBone()
{
	this->m_BindPose	= CustomType::Matrix4x4::Identity();
	this->m_NodeIndex	= 0u;
	this->m_RootBone	= nullptr;
	this->m_ParentBone	= nullptr;
}
CSkeletonSingleBone::~CSkeletonSingleBone()
{

}
BOOL CSkeletonSingleBone::IsRootBone()const
{
	return (this->m_RootBone == nullptr);
}
BOOL CSkeletonSingleBone::HasParentBone()const
{
	return (this->m_ParentBone != nullptr);
}
BOOL CSkeletonSingleBone::HasChildrenBone()const
{
	return (this->m_ChildrenBone.size() > 0);
}
const UINT& CSkeletonSingleBone::GetNodeIndex()const
{
	return (this->m_NodeIndex);
}
const CustomType::Matrix4x4& CSkeletonSingleBone::GetBindPose()const
{
	return (this->m_BindPose);
}
const std::string& CSkeletonSingleBone::GetBoneName()const
{
	return (this->m_BoneName);
}
const CSkeletonSingleBone* CSkeletonSingleBone::GetRootBone()const
{
	return (this->m_RootBone);
}
void CSkeletonSingleBone::SetNodeIndex(const UINT& nodeIndex)
{
	this->m_NodeIndex = nodeIndex;
}
void CSkeletonSingleBone::SetBindPose(const CustomType::Matrix4x4& m)
{
	this->m_BindPose = m;
}
void CSkeletonSingleBone::SetBoneName(const std::string& boneName)
{
	this->m_BoneName = boneName;
}
void CSkeletonSingleBone::SetRootBone(CSkeletonSingleBone* rootBone)
{
	this->m_RootBone = rootBone;
}
const CSkeletonSingleBone* CSkeletonSingleBone::GetParentBone()const
{
	return (this->m_ParentBone);
}
const CSkeletonSingleBone* CSkeletonSingleBone::GetChildBoneByName(const std::string& boneName)const
{
	UINT childNum = static_cast<UINT>(this->m_ChildrenBone.size());
	for (UINT childIndex = 0u; childIndex < childNum; childIndex++)
	{
		if (this->m_ChildrenBone[childIndex]->GetBoneName() == boneName)
		{
			return (this->m_ChildrenBone[childIndex]);
		}
	}
	return nullptr;
}
const CSkeletonSingleBone* CSkeletonSingleBone::GetChildBoneByIndex(const UINT& childIndex)const
{
	UINT childNum = static_cast<UINT>(this->m_ChildrenBone.size());
	if (childIndex < childNum)
	{
		return (this->m_ChildrenBone[childIndex]);
	}
	return nullptr;
}
const std::vector<CSkeletonSingleBone*>& CSkeletonSingleBone::GetChildBoneList()const
{
	return (this->m_ChildrenBone);
}
void CSkeletonSingleBone::SetParentBone(CSkeletonSingleBone* bone)
{
	this->m_ParentBone = bone;
}
void CSkeletonSingleBone::AddChildBone(CSkeletonSingleBone* bone)
{
	if (bone != nullptr)
	{
		BOOL needAddChild = TRUE;
		UINT childrenNum = static_cast<UINT>(this->m_ChildrenBone.size());
		for (UINT i = 0u; i < childrenNum; i++)
		{
			if (this->m_ChildrenBone[i]->GetBoneName() == bone->GetBoneName())
			{
				needAddChild = FALSE;
				break;
			}
		}
		if (needAddChild == TRUE)
		{
			this->m_ChildrenBone.push_back(bone);
		}
	}
}
void CSkeletonSingleBone::ResetChildrenBone()
{
	if (this->m_ChildrenBone.size() > 0)
	{
		this->m_ChildrenBone.clear();
	}
}
BOOL CSkeletonSingleBone::operator==(const CSkeletonSingleBone* v)
{
	return (this->m_BoneName == v->m_BoneName);
}
BOOL CSkeletonSingleBone::operator!=(const CSkeletonSingleBone* v)
{
	return (this->m_BoneName != v->m_BoneName);
}
const CTransform* CSkeletonSingleBone::GetTransform()const
{
	return (&(this->m_Transform));
}
CTransform* CSkeletonSingleBone::GetTransformNoConst()
{
	return (&(this->m_Transform));
}



CSkeletonComponent::CSkeletonComponent(const std::string& skeletonName) : CBaseComponent(TRUE, TRUE, FALSE)
{
	this->m_SkeletonPerFrameUpload	= TRUE;
	this->m_SkeletonName			= skeletonName;
	this->m_RootBone				= nullptr;
	this->m_SkeletonGPUCBuffer		= nullptr;
}
CSkeletonComponent::~CSkeletonComponent()
{
	this->m_RootBone->GetTransformNoConst()->RemoveParent();
}
void CSkeletonComponent::Init()
{
	this->m_SkeletonPerFrameUpload = TRUE;
}
void CSkeletonComponent::Uninit()
{

}
void CSkeletonComponent::Update()
{
	// TODO Check(!this->IsValid())
	this->m_SkeletonPerFrameUpload = TRUE;
	this->UpdateSkeletonTransformInfoForGPU();
}
BOOL CSkeletonComponent::IsValid()const
{
	return (
		this->GetGameObject() != nullptr &&
		this->m_RootBone != nullptr &&
		this->m_SkeletonGPUCBuffer != nullptr &&
		this->m_NodeListVector.size() > 0 &&
		this->m_BoneListVector.size() > 0 &&
		this->m_NodeListVector.size() == this->m_NodeListMap.size());
}
const CSkeletonSingleBone* CSkeletonComponent::GetBoneByName(const std::string& boneName)const
{
	auto element = this->m_NodeListMap.find(boneName);
	if (element != this->m_NodeListMap.end())
	{
		return (element->second);
	}
	return nullptr;
}
const CSkeletonSingleBone* CSkeletonComponent::GetBoneByBoneIndex(const UINT& boneIndex)const
{
	if (boneIndex < static_cast<UINT>(this->m_BoneListVector.size()))
	{
		return (&(this->m_NodeListVector[this->m_BoneListVector[boneIndex]]));
	}
	return nullptr;
}
void CSkeletonComponent::SetSkeleton(const std::vector<CustomStruct::CGameBoneNodeInfo>& skeletonData, const std::vector<UINT>& boneList, const UINT& rootNode)
{
	const UINT nodeNum = static_cast<UINT>(skeletonData.size());
	const UINT boneNum = static_cast<UINT>(boneList.size());

	if (nodeNum < 1u || nodeNum > CSkeletonComponent::_StaticBoneMaxNum || boneList.size() < 1 || rootNode >= nodeNum)
	{
		return;
	}

	{
		this->m_RootBone = nullptr;
		if (this->m_SkeletonGPUCBufferData.size() != (boneList.size() * 2u))
		{
			this->m_SkeletonGPUCBufferData.resize((boneList.size() * 2u));
		}
		if (this->m_BoneListVector.size() != boneList.size())
		{
			this->m_BoneListVector.resize(boneList.size());
		}
		if (this->m_NodeListVector.size() != skeletonData.size())
		{
			this->m_NodeListVector.resize(skeletonData.size());
		}
		if (this->m_NodeListMap.size() > 0)
		{
			this->m_NodeListMap.clear();
		}
	}

	{
		this->m_RootBone = &(this->m_NodeListVector[rootNode]);

		for (UINT nodeIndex = 0u; nodeIndex < nodeNum; nodeIndex++)
		{
			const CustomStruct::CGameBoneNodeInfo& tempInputNode = skeletonData[nodeIndex];
			CSkeletonSingleBone& tempNode = this->m_NodeListVector[nodeIndex];

			tempNode.SetRootBone(rootNode != nodeIndex ? this->m_RootBone : nullptr);
			tempNode.SetNodeIndex(nodeIndex);
			tempNode.SetBindPose(tempInputNode.Offset);
			tempNode.SetBoneName(tempInputNode.Name);

			{
				tempNode.ResetChildrenBone();
				if (tempInputNode.Parent != nullptr)
				{
					tempNode.SetParentBone((&(this->m_NodeListVector[tempInputNode.Parent->Index])));
				}
				else
				{
					tempNode.SetParentBone(nullptr);
				}
				for (size_t childIndex = 0u; childIndex < tempInputNode.Children.size(); childIndex++)
				{
					tempNode.AddChildBone((&(this->m_NodeListVector[tempInputNode.Children[childIndex]->Index])));
				}
			}

			{
				CTransform* tempNodeTransform = tempNode.GetTransformNoConst();
				tempNodeTransform->RemoveParent();
				tempNodeTransform->RemoveAllChildren();
			}
		}

		for (UINT nodeIndex = 0u; nodeIndex < nodeNum; nodeIndex++)
		{
			CSkeletonSingleBone& tempNode = this->m_NodeListVector[nodeIndex];
			if (tempNode.IsRootBone())
			{
				const CGameObject* tempGameObject = this->GetGameObject();
				if (tempGameObject != nullptr && tempGameObject->HasTransform())
				{
					tempNode.GetTransformNoConst()->SetParent(tempGameObject->GetTransform());
				}
			}
			else if (tempNode.HasParentBone())
			{
				tempNode.GetTransformNoConst()->SetParent(tempNode.m_ParentBone->GetTransformNoConst());
			}
		}

		for (UINT nodeIndex = 0u; nodeIndex < nodeNum; nodeIndex++)
		{
			const CustomStruct::CGameBoneNodeInfo& tempInputNode = skeletonData[nodeIndex];
			CSkeletonSingleBone& tempNode = this->m_NodeListVector[nodeIndex];
			CTransform* tempNodeTransform = tempNode.GetTransformNoConst();
			tempNodeTransform->SetLocalPosition(tempInputNode.Location);
			tempNodeTransform->SetLocalRotation(tempInputNode.Rotation);
			tempNodeTransform->SetLocalScale(tempInputNode.Scale);

			this->m_NodeListMap.insert_or_assign(tempNode.GetBoneName(), (&(this->m_NodeListVector[nodeIndex])));
		}

		for (UINT boneIndex = 0u; boneIndex < boneNum; boneIndex++)
		{
			// TODO Check(boneList[boneIndex] > CSkeletonComponent::_StaticBoneMaxNum);

			USHORT tempBoneIndex = static_cast<USHORT>(boneList[boneIndex]);
			this->m_BoneListVector[boneIndex] = tempBoneIndex;

			//CSkeletonSingleBone& tempNode = this->m_NodeListVector[tempBoneIndex];
			//CustomType::Matrix4x4 tempBindPose(tempNode.GetBindPose());
			//CustomType::Matrix4x4 tempLocation(tempNode.GetTransformNoConst()->GetLocalToWorldMatrix());
			//this->m_SkeletonGPUCBufferData[boneIndex] = (tempBindPose * tempLocation).GetGPUUploadFloat4x4();
		}
	}

	CRenderDevice::CreateBuffer(this->m_SkeletonGPUCBuffer, CustomStruct::CRenderBufferDesc(
		static_cast<UINT>(this->m_SkeletonGPUCBufferData.size()) * sizeof(DirectX::XMFLOAT4X4), CustomStruct::CRenderBindFlag::BIND_CONSTANT_BUFFER, sizeof(FLOAT)));
}
void CSkeletonComponent::BindVSSkeletonGPUConstantBuffer(const UINT& startSlot)const
{
	CRenderDevice::BindVSConstantBuffer(this->m_SkeletonGPUCBuffer, startSlot);
}
void CSkeletonComponent::BindPSSkeletonGPUConstantBuffer(const UINT& startSlot)const
{
	CRenderDevice::BindPSConstantBuffer(this->m_SkeletonGPUCBuffer, startSlot);
}
void CSkeletonComponent::BindCSSkeletonGPUConstantBuffer(const UINT& startSlot)const
{
	CRenderDevice::BindCSConstantBuffer(this->m_SkeletonGPUCBuffer, startSlot);
}
void CSkeletonComponent::UpdateSkeletonTransformInfoForGPU()
{
	const UINT boneNum = static_cast<UINT>(this->m_BoneListVector.size());
	for (UINT boneIndex = 0u; boneIndex < boneNum; boneIndex++)
	{
		const USHORT& tempBoneIndex = this->m_BoneListVector[boneIndex];
		const CSkeletonSingleBone& tempNode = this->m_NodeListVector[tempBoneIndex];

		CustomType::Matrix4x4 tempBindPose(tempNode.GetBindPose());
		CustomType::Matrix4x4 tempLocation(tempNode.GetTransform()->GetLocalToWorldMatrix());
		this->m_SkeletonGPUCBufferData[boneIndex] = (tempBindPose * tempLocation).GetGPUUploadFloat4x4();
		this->m_SkeletonGPUCBufferData[boneNum + boneIndex] = (tempBindPose * tempLocation).Inverse().GetXMFLOAT4X4();
	}
}
void CSkeletonComponent::UploadSkeletonGPUConstantBuffer()const
{
	if (this->m_SkeletonPerFrameUpload == TRUE)
	{
		CRenderDevice::UploadBuffer(this->m_SkeletonGPUCBuffer, (void*)(this->m_SkeletonGPUCBufferData.data()));
		this->m_SkeletonPerFrameUpload = FALSE;
	}
}