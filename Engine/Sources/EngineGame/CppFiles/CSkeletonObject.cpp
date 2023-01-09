#include "../Headers/CSkeletonObject.h"

CGameObjectSingleBone::CGameObjectSingleBone(const BOOL& active, const CScene* scene, CGameObjectRootBone* rootBone, const std::string& boneName) : CGameObjectTransformBase(active, scene)
{
	this->m_BoneIndex = 0u;
	this->m_BoneName = boneName;
	this->m_RootBone = rootBone;
}
CGameObjectSingleBone::~CGameObjectSingleBone()
{

}
BOOL CGameObjectSingleBone::IsRootBone()const
{
	return (this->m_RootBone == nullptr);
}
void CGameObjectSingleBone::SetBoneIndex(const UINT& boneIndex)
{
	this->m_BoneIndex = boneIndex;
}
const UINT& CGameObjectSingleBone::GetBoneIndex()const
{
	return (this->m_BoneIndex);
}
const std::string& CGameObjectSingleBone::GetBoneName()const
{
	return (this->m_BoneName);
}
CGameObjectRootBone* CGameObjectSingleBone::GetRootBone()const
{
	return (this->m_RootBone);
}



CGameObjectRootBone::CGameObjectRootBone(const BOOL& active, const CScene* scene, const std::string& boneName) : CGameObjectSingleBone(active, scene, nullptr, boneName)
{
	this->AddBone(this);
}
CGameObjectRootBone::~CGameObjectRootBone()
{

}
UINT CGameObjectRootBone::GetNextBoneIndex()const
{
	return (static_cast<UINT>(this->m_BoneListVector.size()));
}
CGameObjectSingleBone* CGameObjectRootBone::GetBoneByName(const std::string& boneName)const
{
	auto element = this->m_BoneListMap.find(boneName);
	if (element != this->m_BoneListMap.end())
	{
		return (element->second);
	}
	return nullptr;
}
CGameObjectSingleBone* CGameObjectRootBone::GetBoneByIndex(const UINT& boneIndex)const
{
	if (boneIndex < static_cast<UINT>(this->m_BoneListVector.size()))
	{
		return (this->m_BoneListVector[boneIndex]);
	}
	return nullptr;
}
BOOL CGameObjectRootBone::AddBone(CGameObjectRootBone* bone)
{
	if (bone == nullptr)
	{
		return FALSE;
	}
	UINT boneListSize = static_cast<UINT>(this->m_BoneListMap.size());
	if (boneListSize >= _StaticBoneMaxNum)
	{
		return FALSE;
	}
	BOOL canAddIntoList = FALSE;
	{
		BOOL isRootBone = bone->IsRootBone();
		if (isRootBone && boneListSize == 0u)
		{
			canAddIntoList = TRUE;
		}
		else if (!isRootBone && boneListSize == 1u)
		{
			canAddIntoList = TRUE;
		}
		else if (!isRootBone && boneListSize > 1u)
		{
			if (this->GetBoneByName(bone->GetBoneName()) == nullptr)
			{
				canAddIntoList = TRUE;
			}
		}
	}

	if (canAddIntoList)
	{
		bone->SetBoneIndex(this->GetNextBoneIndex());
		this->m_BoneListVector.push_back(bone);
		this->m_BoneListMap.insert_or_assign(bone->GetBoneName(), bone);
	}
	return canAddIntoList;
}
BOOL CGameObjectRootBone::RemoveBone(CGameObjectRootBone* bone)
{
	if (bone == nullptr || bone->IsRootBone())
	{
		return FALSE;
	}

	{
		UINT boneListSize = static_cast<UINT>(this->m_BoneListMap.size());
		if (boneListSize <= 1u)
		{
			return FALSE;
		}
	}

	{
		std::string boneName(bone->GetBoneName());
		auto element = this->m_BoneListMap.find(boneName);
		if (element != this->m_BoneListMap.end())
		{
			this->m_BoneListMap.erase(element->first);
		}
	}

	for (auto it = this->m_BoneListVector.begin(); it != this->m_BoneListVector.end(); it++)
	{
		if ((*it) != nullptr && (*it) == bone)
		{
			(*it) = nullptr;
			return TRUE;
		}
	}

	return FALSE;
}
BOOL CGameObjectRootBone::RemoveBone(const std::string& boneName)
{
	{
		UINT boneListSize = static_cast<UINT>(this->m_BoneListMap.size());
		if (boneListSize <= 1u)
		{
			return FALSE;
		}
	}

	{
		auto element = this->m_BoneListMap.find(boneName);
		if (element != this->m_BoneListMap.end())
		{
			if (element->second != nullptr && !element->second->IsRootBone())
			{
				this->m_BoneListMap.erase(element->first);
			}
		}
	}

	for (auto it = this->m_BoneListVector.begin(); it != this->m_BoneListVector.end(); it++)
	{
		if ((*it) != nullptr && !(*it)->IsRootBone() && (*it)->GetBoneName() == boneName)
		{
			(*it) = nullptr;
			return TRUE;
		}
	}

	return FALSE;
}
void CGameObjectRootBone::RemoveAllBone()
{
	if (this->m_BoneListVector.size() > 0)
	{
		this->m_BoneListVector.clear();
	}
	if (this->m_BoneListMap.size() > 0)
	{
		this->m_BoneListMap.clear();
	}
	this->AddBone(this);
}
void CGameObjectRootBone::ResortBonesAndClearNull()
{
	const UINT boneListSize = static_cast<UINT>(this->m_BoneListVector.size());
	if (boneListSize > 1u)
	{
		this->m_BoneListVector.erase(std::remove_if(this->m_BoneListVector.begin(), this->m_BoneListVector.end(), [](CGameObjectSingleBone* it)->BOOL { return (it == nullptr); }), this->m_BoneListVector.end());
	}
	for (UINT i = 0u; i < boneListSize; i++)
	{
		CGameObjectSingleBone*& tempBone = this->m_BoneListVector[i];
		tempBone->SetBoneIndex(i);
	}
}



CSceneSkeletonManager::CSceneSkeletonManager(CScene* scene)
{
	this->m_Scene = scene;
}
CSceneSkeletonManager::~CSceneSkeletonManager()
{

}