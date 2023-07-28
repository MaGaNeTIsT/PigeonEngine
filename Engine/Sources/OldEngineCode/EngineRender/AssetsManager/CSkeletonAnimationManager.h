/*
#pragma once

#include "../../../../../Entry/EngineMain.h"
#include "../../../EngineGame/Headers/CGameStructCommon.h"

class CSkeletonAnimationManager
{
public:
	static const CSkeletonAnimationManager* const	GetManager() { return m_SkeletonAnimationManager; }
	static void										ShutDown();
public:
	static void										ClearData();
public:
	static const CustomStruct::CGameAnimationInfo*	LoadSkeletonAnimationAsset(const std::string& path, const std::string* animationName, UINT* animationIndex = nullptr);
private:
	static void										ImportAssetSkeletonAnimation(const std::string& path);
private:
	static const CustomStruct::CGameAnimationInfo*	FindData(const std::string& path, const std::string* name, const UINT* index = nullptr);
private:
	std::map<std::string, std::map<std::string, CustomStruct::CGameAnimationInfo>>	m_Data;
private:
	CSkeletonAnimationManager() {}
	CSkeletonAnimationManager(const CSkeletonAnimationManager&) {}
	~CSkeletonAnimationManager() {}
private:
	static CSkeletonAnimationManager*	m_SkeletonAnimationManager;
};
*/