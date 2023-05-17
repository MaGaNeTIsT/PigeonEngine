#include "../Headers/CSkeletonComponent.h"
#include "../../EngineRender/RenderBase/Headers/CRenderDevice.h"
#include "../Headers/CGameObject.h"

CSkeletonSingleNode::CSkeletonSingleNode()
{
	this->m_BoneSpaceGlobalMatrix	= CustomType::Matrix4x4::Identity();
	this->m_BindPoseMatrix			= CustomType::Matrix4x4::Identity();
	this->m_DefaultPosition			= CustomType::Vector3::Zero();
	this->m_DefaultRotation			= CustomType::Quaternion::Identity();
	this->m_DefaultScaling			= CustomType::Vector3::One();
	this->m_RootIndex				= -1;
	this->m_BoneIndex				= -1;
	this->m_NodeIndex				= -1;
	this->m_ParentIndex				= -1;
	if (this->m_ChildrenIndex.size() > 0)
	{
		this->m_ChildrenIndex.clear();
	}

	this->m_Transform.InitTransform(CustomType::Vector3::Zero(), CustomType::Quaternion::Identity(), CustomType::Vector3::One());
	this->m_Transform.RemoveParent();
	this->m_Transform.RemoveAllChildren();
}
CSkeletonSingleNode::~CSkeletonSingleNode()
{
	this->m_Transform.RemoveParent();
	this->m_Transform.RemoveAllChildren();
}
BOOL CSkeletonSingleNode::IsRootNode()const
{
	return (this->m_RootIndex == -1);
}
BOOL CSkeletonSingleNode::IsBoneNode()const
{
	return (this->m_BoneIndex >= 0);
}
BOOL CSkeletonSingleNode::HasParentNode()const
{
	return (this->m_ParentIndex >= 0);
}
BOOL CSkeletonSingleNode::HasChildrenNode()const
{
	return (this->m_ChildrenIndex.size() > 0);
}
const CustomType::Matrix4x4& CSkeletonSingleNode::GetBoneSpaceGlobalMatrix()const
{
	return (this->m_BoneSpaceGlobalMatrix);
}
const CustomType::Matrix4x4& CSkeletonSingleNode::GetBindPoseMatrix()const
{
	return (this->m_BindPoseMatrix);
}
const CustomType::Vector3& CSkeletonSingleNode::GetDefaultPosition()const
{
	return (this->m_DefaultPosition);
}
const CustomType::Quaternion& CSkeletonSingleNode::GetDefaultRotation()const
{
	return (this->m_DefaultRotation);
}
const CustomType::Vector3& CSkeletonSingleNode::GetDefaultScaling()const
{
	return (this->m_DefaultScaling);
}
const std::string& CSkeletonSingleNode::GetNodeName()const
{
	return (this->m_NodeName);
}
const SHORT& CSkeletonSingleNode::GetRootIndex()const
{
	return (this->m_RootIndex);
}
const SHORT& CSkeletonSingleNode::GetBoneIndex()const
{
	return (this->m_BoneIndex);
}
const SHORT& CSkeletonSingleNode::GetNodeIndex()const
{
	return (this->m_NodeIndex);
}
const SHORT& CSkeletonSingleNode::GetParentIndex()const
{
	return (this->m_ParentIndex);
}
USHORT CSkeletonSingleNode::GetChildrenNum()const
{
	return (static_cast<USHORT>(this->m_ChildrenIndex.size()));
}
const std::vector<SHORT>& CSkeletonSingleNode::GetChildrenList()const
{
	return (this->m_ChildrenIndex);
}
void CSkeletonSingleNode::SetBindPoseMatrix(const CustomType::Matrix4x4& m)
{
	this->m_BindPoseMatrix = m;
}
void CSkeletonSingleNode::SetBoneSpaceGlobalMatrix(const CustomType::Matrix4x4& m)
{
	this->m_BoneSpaceGlobalMatrix = m;
}
void CSkeletonSingleNode::SetDefaultPosition(const CustomType::Vector3& v)
{
	this->m_DefaultPosition = v;
}
void CSkeletonSingleNode::SetDefaultRotation(const CustomType::Quaternion& v)
{
	this->m_DefaultRotation = v;
}
void CSkeletonSingleNode::SetDefaultScaling(const CustomType::Vector3& v)
{
	this->m_DefaultScaling = v;
}
void CSkeletonSingleNode::SetNodeName(const std::string& name)
{
	this->m_NodeName = name;
}
void CSkeletonSingleNode::SetRootIndex(const SHORT& i)
{
	this->m_RootIndex = i;
}
void CSkeletonSingleNode::SetBoneIndex(const SHORT& i)
{
	this->m_BoneIndex = i;
}
void CSkeletonSingleNode::SetNodeIndex(const SHORT& i)
{
	this->m_NodeIndex = i;
}
void CSkeletonSingleNode::SetParentIndex(const SHORT& i)
{
	this->m_ParentIndex = i;
}
void CSkeletonSingleNode::AddChildIndex(const SHORT& i)
{
	if (i < 0)
	{
		return;
	}
	USHORT numChildren = static_cast<USHORT>(this->m_ChildrenIndex.size());
	for (USHORT index = 0u; index < numChildren; index++)
	{
		if (i == this->m_ChildrenIndex[index])
		{
			return;
		}
	}
	this->m_ChildrenIndex.push_back(i);
}
void CSkeletonSingleNode::SetAnimationPosition(const CustomType::Vector3& v)
{
	this->m_Transform.SetLocalPosition(v);
}
void CSkeletonSingleNode::SetAnimationRotation(const CustomType::Quaternion& v)
{
	this->m_Transform.SetLocalRotation(v);
}
void CSkeletonSingleNode::SetAnimationScaling(const CustomType::Vector3& v)
{
	this->m_Transform.SetLocalScale(v);
}
const CTransform* CSkeletonSingleNode::GetTransform()const
{
	return (&(this->m_Transform));
}
CTransform* CSkeletonSingleNode::GetTransformNoConst()
{
	return (&(this->m_Transform));
}



static CSkeletonSingleNode _GSkeletonSingleErrorNode;
CSkeletonComponent::CSkeletonComponent(const std::string& skeletonName) : CBaseComponent(TRUE, TRUE, FALSE)
{
	this->m_SkeletonPerFrameUpload = TRUE;
	this->m_SkeletonGPUCBufferData.SkeletonBoneNum = DirectX::XMFLOAT4(0.f, 0.f, 0.f, 0.f);
	this->m_SkeletonGPUCBuffer = nullptr;

	this->m_SkeletonName = skeletonName;
	this->m_RootTransformInverse = CustomType::Matrix4x4::Identity();
	this->m_RootNodeIndex = -1;
	if (this->m_BoneListVector.size() > 0)
	{
		this->m_BoneListVector.clear();
	}
	if (this->m_NodeListMap.size() > 0)
	{
		this->m_NodeListMap.clear();
	}
	if (this->m_NodeListVector.size() > 0)
	{
		this->m_NodeListVector.clear();
	}
}
CSkeletonComponent::~CSkeletonComponent()
{

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
	this->UpdateSkeletonTransformInfo();
}
BOOL CSkeletonComponent::IsValid()const
{
	return (
		this->GetGameObject() != nullptr &&
		this->m_RootNodeIndex >= 0 &&
		this->m_SkeletonGPUCBuffer != nullptr &&
		this->m_NodeListVector.size() > 0 &&
		this->m_BoneListVector.size() > 0 &&
		this->m_NodeListVector.size() == this->m_NodeListMap.size());
}
const std::string& CSkeletonComponent::GetSkeletonName()const
{
	return (this->m_SkeletonName);
}
const CSkeletonSingleNode& CSkeletonComponent::GetNodeByName(const std::string& name)const
{
	if (this->m_NodeListMap.size() > 0 && this->m_NodeListVector.size() > 0 && this->m_NodeListMap.size() == this->m_NodeListVector.size())
	{
		auto itFindName = this->m_NodeListMap.find(name);
		if (itFindName != this->m_NodeListMap.end())
		{
			return (this->m_NodeListVector[itFindName->second]);
		}
	}
	return (_GSkeletonSingleErrorNode);
}
const CSkeletonSingleNode& CSkeletonComponent::GetNodeByNodeIndex(const USHORT& i)const
{
	if (this->m_NodeListVector.size() > 0 && this->m_NodeListMap.size() > 0 && this->m_NodeListMap.size() == this->m_NodeListVector.size())
	{
		USHORT numNodes = static_cast<USHORT>(this->m_NodeListVector.size());
		if (i < numNodes)
		{
			return (this->m_NodeListVector[i]);
		}
	}
	return (_GSkeletonSingleErrorNode);
}
CSkeletonSingleNode& CSkeletonComponent::GetNodeByNameNoConst(const std::string& name)
{
	if (this->m_NodeListMap.size() > 0 && this->m_NodeListVector.size() > 0 && this->m_NodeListMap.size() == this->m_NodeListVector.size())
	{
		auto itFindName = this->m_NodeListMap.find(name);
		if (itFindName != this->m_NodeListMap.end())
		{
			return (this->m_NodeListVector[itFindName->second]);
		}
	}
	return (_GSkeletonSingleErrorNode);
}
CSkeletonSingleNode& CSkeletonComponent::GetNodeByNodeIndexNoConst(const USHORT& i)
{
	if (this->m_NodeListVector.size() > 0 && this->m_NodeListMap.size() > 0 && this->m_NodeListMap.size() == this->m_NodeListVector.size())
	{
		USHORT numNodes = static_cast<USHORT>(this->m_NodeListVector.size());
		if (i < numNodes)
		{
			return (this->m_NodeListVector[i]);
		}
	}
	return (_GSkeletonSingleErrorNode);
}
void CSkeletonComponent::SetSkeleton(const std::vector<CustomStruct::CGameBoneNodeInfo>& skeleton, const std::map<std::string, SHORT>& boneIndexMap, const std::vector<USHORT>& boneList)
{
	const USHORT nodeNum = static_cast<USHORT>(skeleton.size());
	const USHORT boneNum = static_cast<USHORT>(boneList.size());

	if (nodeNum < 1u || nodeNum > CustomStruct::CRenderBaseSetting::RenderBoneMaxNum || boneNum < 1u || skeleton.size() != boneIndexMap.size())
	{
		return;
	}

	{
		this->m_RootNodeIndex = 0;

		//TODO we do not need this matrix for now.
		this->m_RootTransformInverse = CustomType::Matrix4x4::Identity();

		if (this->m_NodeListVector.size() > 0)
		{
			this->m_NodeListVector.clear();
		}
		if (this->m_NodeListMap.size() > 0)
		{
			this->m_NodeListMap.clear();
		}
		if (this->m_BoneListVector.size() > 0)
		{
			this->m_BoneListVector.clear();
		}
	}

	{
		for (USHORT indexInputNode = 0u; indexInputNode < nodeNum; indexInputNode++)
		{
			const CustomStruct::CGameBoneNodeInfo& inputNodeInfo = skeleton[indexInputNode];
			CSkeletonSingleNode saveSingleNode;
			saveSingleNode.SetRootIndex(this->m_RootNodeIndex);
			saveSingleNode.SetBindPoseMatrix(inputNodeInfo.BindPoseMatrix);
			saveSingleNode.SetDefaultPosition(inputNodeInfo.DefaultPosition);
			saveSingleNode.SetDefaultRotation(inputNodeInfo.DefaultRotation);
			saveSingleNode.SetDefaultScaling(inputNodeInfo.DefaultScaling);
			saveSingleNode.SetNodeName(inputNodeInfo.Name);
			saveSingleNode.SetNodeIndex(indexInputNode);
			saveSingleNode.SetParentIndex(inputNodeInfo.Parent);
			USHORT numChildren = static_cast<USHORT>(inputNodeInfo.Children.size());
			for (USHORT indexNodeChild = 0u; indexNodeChild < numChildren; indexNodeChild++)
			{
				saveSingleNode.AddChildIndex(inputNodeInfo.Children[indexNodeChild]);
			}
			CTransform* nodeTransform = saveSingleNode.GetTransformNoConst();
			nodeTransform->SetLocalPosition(inputNodeInfo.DefaultPosition);
			nodeTransform->SetLocalRotation(inputNodeInfo.DefaultRotation);
			nodeTransform->SetLocalScale(inputNodeInfo.DefaultScaling);
			this->m_NodeListVector.push_back(saveSingleNode);
			this->m_NodeListMap.insert_or_assign(inputNodeInfo.Name, indexInputNode);
		}

		// We keep first bone as identity dummy matrix.
		this->m_BoneListVector.push_back(0u);
		for (USHORT indexInputBone = 1u; indexInputBone < boneNum; indexInputBone++)
		{
			this->m_NodeListVector[boneList[indexInputBone]].SetBoneIndex(static_cast<SHORT>(indexInputBone));
			this->m_BoneListVector.push_back(boneList[indexInputBone]);
		}
	}

	//TODO
	this->m_SkeletonGPUCBufferData.SkeletonBoneNum = DirectX::XMFLOAT4(static_cast<FLOAT>(this->m_BoneListVector.size()), 0.f, 0.f, 0.f);
	this->m_SkeletonGPUCBufferData.SkeletonMatrix[0] = CustomType::Matrix4x4::Identity().GetXMFLOAT4X4();
	this->m_SkeletonGPUCBufferData.SkeletonMatrix[this->m_BoneListVector.size()] = CustomType::Matrix4x4::Identity().GetXMFLOAT4X4();

	CRenderDevice::CreateBuffer(this->m_SkeletonGPUCBuffer, CustomStruct::CRenderBufferDesc(sizeof(CustomStruct::CShaderSkeletonMatrix), CustomStruct::CRenderBindFlag::BIND_CONSTANT_BUFFER, sizeof(FLOAT)));
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
void CSkeletonComponent::UpdateSkeletonTransformInfo()
{
	// First we update the whole node.
	CustomType::Matrix4x4 dummyMatrix(CustomType::Matrix4x4::Identity());
	this->UpdateBoneTransformMatrix(0, dummyMatrix);

	const USHORT boneNum = static_cast<USHORT>(this->m_BoneListVector.size());
	for (USHORT boneIndex = 1u; boneIndex < boneNum; boneIndex++)
	{
		const USHORT& tempBoneIndex = this->m_BoneListVector[boneIndex];
		const CSkeletonSingleNode& tempNode = this->m_NodeListVector[tempBoneIndex];

		CustomType::Matrix4x4 finalMatrix(tempNode.GetBoneSpaceGlobalMatrix());
		finalMatrix = finalMatrix * tempNode.GetBindPoseMatrix();

		this->m_SkeletonGPUCBufferData.SkeletonMatrix[boneIndex] = (finalMatrix).GetXMFLOAT4X4();
		this->m_SkeletonGPUCBufferData.SkeletonMatrix[boneNum + boneIndex] = (finalMatrix).Inverse().Transpose().GetXMFLOAT4X4();
	}
}
void CSkeletonComponent::UploadSkeletonGPUConstantBuffer()const
{
	if (this->m_SkeletonPerFrameUpload == TRUE)
	{
		CRenderDevice::UploadBuffer(this->m_SkeletonGPUCBuffer, static_cast<const void*>(&(this->m_SkeletonGPUCBufferData)));
		this->m_SkeletonPerFrameUpload = FALSE;
	}
}
void CSkeletonComponent::UpdateBoneTransformMatrix(SHORT nodeIndex, CustomType::Matrix4x4 globalTransform)
{
	if (nodeIndex < 0)
	{
		return;
	}

	CSkeletonSingleNode& currentNode = this->m_NodeListVector[nodeIndex];
	CustomType::Matrix4x4 tempGlobalTransform = currentNode.GetTransform()->GetLocalToWorldMatrix();
	tempGlobalTransform = tempGlobalTransform.Transpose();
	tempGlobalTransform = globalTransform * tempGlobalTransform;
	currentNode.SetBoneSpaceGlobalMatrix(tempGlobalTransform);

	{
		USHORT numChildren = currentNode.GetChildrenNum();
		if (numChildren > 0u)
		{
			const std::vector<SHORT>& currentNodeChildren = currentNode.GetChildrenList();
			for (USHORT indexChild = 0u; indexChild < numChildren; indexChild++)
			{
				this->UpdateBoneTransformMatrix(currentNodeChildren[indexChild], tempGlobalTransform);
			}
		}
	}
}