#pragma once

#include "../../../../Entry/EngineMain.h"
#include "./CGameObject.h"

class CGameObjectSingleBone : public CGameObjectTransformBase
{
public:
	BOOL	IsRootBone()const;
	void	SetBoneIndex(const UINT& boneIndex);
public:
	const UINT&					GetBoneIndex()const;
	const std::string&			GetBoneName()const;
	class CGameObjectRootBone*	GetRootBone()const;
protected:
	UINT							m_BoneIndex;
	std::string						m_BoneName;
	class CGameObjectRootBone*		m_RootBone;
public:
	CGameObjectSingleBone(const BOOL& active, const class CScene* scene, class CGameObjectRootBone* rootBone, const std::string& boneName);
	virtual ~CGameObjectSingleBone();
};

class CGameObjectRootBone : public CGameObjectSingleBone
{
public:
	UINT							GetNextBoneIndex()const;
	class CGameObjectSingleBone*	GetBoneByName(const std::string& boneName)const;
	class CGameObjectSingleBone*	GetBoneByIndex(const UINT& boneIndex)const;
public:
	BOOL	AddBone(class CGameObjectRootBone* bone);
	BOOL	RemoveBone(class CGameObjectRootBone* bone);
	BOOL	RemoveBone(const std::string& boneName);
	void	RemoveAllBone();
	void	ResortBonesAndClearNull();
public:
	const static UINT	_StaticBoneMaxNum = 384u;
protected:
	Microsoft::WRL::ComPtr<ID3D11Buffer>					m_BoneGPUBuffer;
	std::map<std::string, class CGameObjectSingleBone*>		m_BoneListMap;
	std::vector<class CGameObjectSingleBone*>				m_BoneListVector;
public:
	CGameObjectRootBone(const BOOL& active, const class CScene* scene, const std::string& boneName);
	virtual ~CGameObjectRootBone();
};

class CSceneSkeletonManager
{
public:

protected:
	std::map<ULONGLONG, class CGameObjectRootBone*>		m_SkeletonRoot;
protected:
	class CScene*	m_Scene;
public:
	CSceneSkeletonManager(class CScene* scene);
	virtual ~CSceneSkeletonManager();
};