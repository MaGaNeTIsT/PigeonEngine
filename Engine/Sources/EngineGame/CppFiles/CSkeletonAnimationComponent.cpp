#include "../Headers/CSkeletonAnimationComponent.h"
#include "../../EngineBase/Headers/CManager.h"
#include "../../EngineBase/Headers/CTimer.h"
#include "../Headers/CSkeletonComponent.h"

template<typename KeyValueType, typename KeyLerpFunc>
BOOL LerpAnimationKey(const DOUBLE& keyTime, KeyValueType& output, const std::vector<CustomStruct::CGameAnimationKey<KeyValueType>>& keys, KeyLerpFunc func)
{
	if (keys.size() == 0u)
	{
		return FALSE;
	}
	UINT numKey = static_cast<UINT>(keys.size()) - 1u;
	for (UINT indexKey = 0u; indexKey < numKey; indexKey++)
	{
		if (keyTime >= keys[indexKey].Time && keyTime < keys[indexKey + 1u].Time)
		{
			FLOAT t = static_cast<FLOAT>((keyTime - keys[indexKey].Time) / (keys[indexKey + 1u].Time - keys[indexKey].Time));
			output = func(keys[indexKey].Value, keys[indexKey + 1u].Value, t);
			return TRUE;
		}
	}
	output = keys[numKey].Value;
	return TRUE;
}

CSkeletonAnimationComponent::CSkeletonAnimationComponent() : CBaseComponent(TRUE, TRUE, FALSE)
{
	this->m_GameTimer				= nullptr;
	this->m_PlayState				= SkeletonAnimationPlayState::SkeletonAnimationPlayState_None;
	this->m_LastState				= SkeletonAnimationPlayState::SkeletonAnimationPlayState_None;
	this->m_PlayTime				= static_cast<DOUBLE>(0);
	this->m_CurrentPlayAnimation	= nullptr;
	this->m_SkeletonComponent		= nullptr;
}
CSkeletonAnimationComponent::~CSkeletonAnimationComponent()
{

}
void CSkeletonAnimationComponent::Init()
{
	this->m_GameTimer	= CManager::GetGameTimer();
	this->m_PlayState	= SkeletonAnimationPlayState::SkeletonAnimationPlayState_None;
	this->m_LastState	= SkeletonAnimationPlayState::SkeletonAnimationPlayState_None;
	this->m_PlayTime	= static_cast<DOUBLE>(0);
}
void CSkeletonAnimationComponent::Uninit()
{
	this->m_GameTimer = nullptr;
}
void CSkeletonAnimationComponent::Update()
{
	if (this->m_SkeletonComponent == nullptr || this->m_CurrentPlayAnimation == nullptr ||
		this->m_PlayState == SkeletonAnimationPlayState::SkeletonAnimationPlayState_None ||
		this->m_PlayState == SkeletonAnimationPlayState::SkeletonAnimationPlayState_Pause)
	{
		return;
	}

	this->m_PlayTime += this->m_GameTimer->GetDeltaTime();
	
	DOUBLE currentPlayTime = this->m_PlayTime;
	if (this->m_PlayState & SkeletonAnimationPlayState::SkeletonAnimationPlayState_Repeat)
	{
		currentPlayTime = CustomType::CMath::Mod(currentPlayTime, this->m_CurrentPlayAnimation->Duration);
	}

	{
		auto vec3Lerp = [](const CustomType::Vector3& v1, const CustomType::Vector3& v2, const FLOAT& t)->CustomType::Vector3 { return CustomType::Vector3::Lerp(v1, v2, t); };
		auto quatLerp = [](const CustomType::Quaternion& v1, const CustomType::Quaternion& v2, const FLOAT& t)->CustomType::Quaternion { return CustomType::Quaternion::SLerp(v1, v2, t); };

		UINT numAnimationNode = static_cast<UINT>(this->m_CurrentPlayAnimation->AnimationNodes.size());
		for (UINT indexAnimationNode = 0u; indexAnimationNode < numAnimationNode; indexAnimationNode++)
		{
			const CustomStruct::CGameAnimationNodeInfo& tempNode = this->m_CurrentPlayAnimation->AnimationNodes[indexAnimationNode];
			CSkeletonSingleBone* tempBone = this->m_SkeletonComponent->GetBoneByNameNoConst(tempNode.Name);
			if (tempBone == nullptr)
			{
				continue;
			}

			CustomType::Vector3 tempPos, tempScl;
			CustomType::Quaternion tempRot;

			LerpAnimationKey<CustomType::Vector3>(currentPlayTime, tempPos, tempNode.PositionKeys, vec3Lerp);
			LerpAnimationKey<CustomType::Quaternion>(currentPlayTime, tempRot, tempNode.RotationKeys, quatLerp);
			LerpAnimationKey<CustomType::Vector3>(currentPlayTime, tempScl, tempNode.ScalingKeys, vec3Lerp);

			tempBone->GetTransformNoConst()
		}
	}
}
void CSkeletonAnimationComponent::Play(const BOOL& repeat)
{
	if (this->m_PlayState == SkeletonAnimationPlayState::SkeletonAnimationPlayState_Pause)
	{
		this->m_PlayState = this->m_LastState;
	}
	else
	{
		this->m_PlayState = repeat ? SkeletonAnimationPlayState::SkeletonAnimationPlayState_PlayAndRepeat : SkeletonAnimationPlayState::SkeletonAnimationPlayState_Play;
	}
}
void CSkeletonAnimationComponent::Pause()
{
	if (this->m_PlayState == SkeletonAnimationPlayState::SkeletonAnimationPlayState_None)
	{
		return;
	}
	if (this->m_PlayState == SkeletonAnimationPlayState::SkeletonAnimationPlayState_Pause)
	{
		this->m_PlayState = this->m_LastState;
	}
	else
	{
		this->m_LastState = this->m_PlayState;
		this->m_PlayState = SkeletonAnimationPlayState::SkeletonAnimationPlayState_Pause;
	}
}
void CSkeletonAnimationComponent::Reset()
{
	this->m_PlayState	= SkeletonAnimationPlayState::SkeletonAnimationPlayState_None;
	this->m_PlayTime	= static_cast<DOUBLE>(0);
}
void CSkeletonAnimationComponent::SetCurrentAnimation(const std::string& name)
{
	this->m_CurrentPlayAnimation = this->FindAnimation(name);
}
void CSkeletonAnimationComponent::SetCurrentAnimation(const UINT& index)
{
	if (this->m_AnimationList.size() > 0)
	{
		UINT tempIndex = 0u;
		for (auto indexFind = this->m_AnimationList.begin(); indexFind != this->m_AnimationList.end(); indexFind++)
		{
			if (tempIndex == index)
			{
				this->m_CurrentPlayAnimation = indexFind->second;
				return;
			}
		}
	}
	this->m_CurrentPlayAnimation = nullptr;
}
void CSkeletonAnimationComponent::SetSkeletonComponent(CSkeletonComponent* skeletonComponent)
{
	if (skeletonComponent != nullptr)
	{
		this->m_SkeletonComponent = skeletonComponent;
	}
}
const CSkeletonComponent* CSkeletonAnimationComponent::GetSkeletonComponent()const
{
	return (this->m_SkeletonComponent);
}
void CSkeletonAnimationComponent::AddAnimation(const std::string& name, const CustomStruct::CGameAnimationInfo* animation)
{
	if (animation != nullptr)
	{
		BOOL needAssign = FALSE;
		if (this->m_AnimationList.size() == 0)
		{
			needAssign = TRUE;
		}
		else if (this->m_AnimationList.find(name) == this->m_AnimationList.end())
		{
			needAssign = TRUE;
		}
		if (needAssign)
		{
			this->m_AnimationList.insert_or_assign(name, animation);
		}
	}
}
void CSkeletonAnimationComponent::RemoveAnimation(const std::string& name)
{
	if (this->m_AnimationList.size() > 0)
	{
		if (this->m_AnimationList.find(name) != this->m_AnimationList.end())
		{
			this->m_AnimationList.erase(name);
		}
	}
}
void CSkeletonAnimationComponent::RemoveAllAnimation()
{
	if (this->m_AnimationList.size() > 0)
	{
		this->m_AnimationList.clear();
	}
}
const CustomStruct::CGameAnimationInfo* CSkeletonAnimationComponent::FindAnimation(const std::string& name)const
{
	if (this->m_AnimationList.size() > 0)
	{
		auto result = this->m_AnimationList.find(name);
		if (result != this->m_AnimationList.end())
		{
			return (result->second);
		}
	}
	return nullptr;
}