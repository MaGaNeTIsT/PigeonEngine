/*
#include "../Headers/CSkeletonAnimationManager.h"
#include "../../../../EngineThirdParty/Cassimp/Headers/CassimpManager.h"

CSkeletonAnimationManager* CSkeletonAnimationManager::m_SkeletonAnimationManager = new CSkeletonAnimationManager();
void CSkeletonAnimationManager::ShutDown()
{
	CSkeletonAnimationManager::ClearData();
	delete m_SkeletonAnimationManager;
}
void CSkeletonAnimationManager::ClearData()
{
	if (m_SkeletonAnimationManager->m_Data.size() > 0u)
	{
		m_SkeletonAnimationManager->m_Data.clear();
	}
}
const CustomStruct::CGameAnimationInfo* CSkeletonAnimationManager::LoadSkeletonAnimationAsset(const std::string& path, const std::string* animationName, UINT32* animationIndex)
{
	const CustomStruct::CGameAnimationInfo* result = nullptr;
	result = CSkeletonAnimationManager::FindData(path, animationName, animationIndex);
	if (result != nullptr)
	{
		return result;
	}

	CSkeletonAnimationManager::ImportAssetSkeletonAnimation(path);

	result = CSkeletonAnimationManager::FindData(path, animationName, animationIndex);
	return result;
}
void CSkeletonAnimationManager::ImportAssetSkeletonAnimation(const std::string& path)
{
	CassimpManager::CassimpReadFileState readState = CassimpManager::ReadSkeletonAnimationFile(path, m_SkeletonAnimationManager->m_Data);
}
const CustomStruct::CGameAnimationInfo* CSkeletonAnimationManager::FindData(const std::string& path, const std::string* name, const UINT32* index)
{
	if (m_SkeletonAnimationManager->m_Data.size() > 0)
	{
		auto element = m_SkeletonAnimationManager->m_Data.find(path);
		if (element != m_SkeletonAnimationManager->m_Data.end())
		{
			if (element->second.size() > 0)
			{
				if (element->second.size() > 1)
				{
					if (name != nullptr)
					{
						auto resultAnimation = element->second.find(*name);
						if (resultAnimation != element->second.end())
						{
							return (&(resultAnimation->second));
						}
					}
					else if (index != nullptr)
					{
						UINT32 tempIndex = 0u;
						for (auto resultAnimation = element->second.begin(); resultAnimation != element->second.end(); resultAnimation++)
						{
							if (tempIndex == (*index))
							{
								return (&(resultAnimation->second));
							}
						}
					}
				}
				else
				{
					auto resultAnimation = element->second.begin();
					return (&(resultAnimation->second));
				}
			}
		}
	}
	return nullptr;
}
*/