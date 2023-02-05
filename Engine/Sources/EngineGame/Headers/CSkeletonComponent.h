#pragma once

#include "../../../../Entry/EngineMain.h"
#include "../../EngineBase/Headers/CBaseType.h"
#include "./CGameStructCommon.h"
#include "./CComponent.h"

class CSkeletonSingleBone : public CObjectBase
{
public:
	BOOL	IsRootBone()const;
	BOOL	HasParentBone()const;
	BOOL	HasChildrenBone()const;
public:
	const UINT&						GetNodeIndex()const;
	const CustomType::Matrix4x4&	GetBindPose()const;
	const std::string&				GetBoneName()const;
	const CSkeletonSingleBone*		GetRootBone()const;
public:
	void	SetNodeIndex(const UINT& nodeIndex);
	void	SetBindPose(const CustomType::Matrix4x4& m);
	void	SetBoneName(const std::string& boneName);
	void	SetRootBone(CSkeletonSingleBone* rootBone);
public:
	const CSkeletonSingleBone*					GetParentBone()const;
	const CSkeletonSingleBone*					GetChildBoneByName(const std::string& boneName)const;
	const CSkeletonSingleBone*					GetChildBoneByIndex(const UINT& childIndex)const;
	const std::vector<CSkeletonSingleBone*>&	GetChildBoneList()const;
public:
	void	SetParentBone(CSkeletonSingleBone* bone);
	void	AddChildBone(CSkeletonSingleBone* bone);
	void	ResetChildrenBone();
protected:
	UINT								m_NodeIndex;
	CustomType::Matrix4x4				m_BindPose;
	std::string							m_BoneName;
	CSkeletonSingleBone*				m_RootBone;
	CSkeletonSingleBone*				m_ParentBone;
	std::vector<CSkeletonSingleBone*>	m_ChildrenBone;
public:
	BOOL				operator==(const CSkeletonSingleBone* v);
	BOOL				operator!=(const CSkeletonSingleBone* v);
public:
	const CTransform*	GetTransform()const;
	CTransform*			GetTransformNoConst();
protected:
	CTransform			m_Transform;
public:
	CSkeletonSingleBone();
	virtual ~CSkeletonSingleBone();
private:
	friend class CSkeletonComponent;
};

class CSkeletonComponent : public CBaseComponent
{
public:
	virtual void	Init()override;
	virtual void	Uninit()override;
	virtual void	Update()override;
public:
	BOOL	IsValid()const;
public:
	const CSkeletonSingleBone*		GetBoneByName(const std::string& boneName)const;
	const CSkeletonSingleBone*		GetBoneByBoneIndex(const UINT& boneIndex)const;
protected:
	CSkeletonSingleBone*			GetBoneByNameNoConst(const std::string& boneName);
	CSkeletonSingleBone*			GetBoneByBoneIndexNoConst(const UINT& boneIndex);
public:
	void	SetSkeleton(const std::vector<CustomStruct::CGameBoneNodeInfo>& skeletonData, const std::vector<UINT>& boneList, const UINT& rootNode);
public:
	void	BindVSSkeletonGPUConstantBuffer(const UINT& startSlot)const;
	void	BindPSSkeletonGPUConstantBuffer(const UINT& startSlot)const;
	void	BindCSSkeletonGPUConstantBuffer(const UINT& startSlot)const;
	void	UpdateSkeletonTransformInfoForGPU();
	void	UploadSkeletonGPUConstantBuffer()const;
public:
	const static UINT	_StaticBoneMaxNum	= 288u;
protected:
	void	RecursionBoneTransformMatrix(const CSkeletonSingleBone* parentBone, CustomType::Matrix4x4& globalMat);
protected:
	mutable BOOL									m_SkeletonPerFrameUpload;
	std::string										m_SkeletonName;
	CSkeletonSingleBone*							m_RootBone;
	CustomType::Matrix4x4							m_RootTransformInverse;
	Microsoft::WRL::ComPtr<ID3D11Buffer>			m_SkeletonGPUCBuffer;
	std::vector<DirectX::XMFLOAT4X4>				m_SkeletonGPUCBufferData;
	std::vector<USHORT>								m_BoneListVector;
	std::map<std::string, CSkeletonSingleBone*>		m_NodeListMap;
	std::vector<CSkeletonSingleBone>				m_NodeListVector;
public:
	CSkeletonComponent(const std::string& skeletonName);
	virtual ~CSkeletonComponent();
private:
	friend class CSkeletonAnimationComponent;
};