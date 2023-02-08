#pragma once

#include "../../../../Entry/EngineMain.h"
#include "../../EngineBase/Headers/CBaseType.h"
#include "./CGameStructCommon.h"
#include "./CComponent.h"
#include "../../EngineRender/RenderBase/Headers/CRenderStructCommon.h"

class CSkeletonSingleNode : public CObjectBase
{
public:
	BOOL	IsRootNode()const;
	BOOL	IsBoneNode()const;
	BOOL	HasParentNode()const;
	BOOL	HasChildrenNode()const;
public:
	const CustomType::Matrix4x4&	GetBoneSpaceGlobalMatrix()const;
	const CustomType::Matrix4x4&	GetBindPoseMatrix()const;
	const CustomType::Vector3&		GetDefaultPosition()const;
	const CustomType::Quaternion&	GetDefaultRotation()const;
	const CustomType::Vector3&		GetDefaultScaling()const;
	const std::string&				GetNodeName()const;
	const SHORT&					GetRootIndex()const;
	const SHORT&					GetBoneIndex()const;
	const SHORT&					GetNodeIndex()const;
	const SHORT&					GetParentIndex()const;
	USHORT							GetChildrenNum()const;
	const std::vector<SHORT>&		GetChildrenList()const;
protected:
	void	SetBoneSpaceGlobalMatrix(const CustomType::Matrix4x4& m);
	void	SetBindPoseMatrix(const CustomType::Matrix4x4& m);
	void	SetDefaultPosition(const CustomType::Vector3& v);
	void	SetDefaultRotation(const CustomType::Quaternion& v);
	void	SetDefaultScaling(const CustomType::Vector3& v);
	void	SetNodeName(const std::string& name);
	void	SetRootIndex(const SHORT& i);
	void	SetBoneIndex(const SHORT& i);
	void	SetNodeIndex(const SHORT& i);
	void	SetParentIndex(const SHORT& i);
	void	AddChildIndex(const SHORT& i);
public:
	void	SetAnimationPosition(const CustomType::Vector3& v);
	void	SetAnimationRotation(const CustomType::Quaternion& v);
	void	SetAnimationScaling(const CustomType::Vector3& v);
protected:
	CustomType::Matrix4x4		m_BoneSpaceGlobalMatrix;
	CustomType::Matrix4x4		m_BindPoseMatrix;
	CustomType::Vector3			m_DefaultPosition;
	CustomType::Quaternion		m_DefaultRotation;
	CustomType::Vector3			m_DefaultScaling;
	std::string					m_NodeName;
	SHORT						m_RootIndex;
	SHORT						m_BoneIndex;
	SHORT						m_NodeIndex;
	SHORT						m_ParentIndex;
	std::vector<SHORT>			m_ChildrenIndex;
public:
	const CTransform*	GetTransform()const;
protected:
	CTransform*			GetTransformNoConst();
protected:
	CTransform			m_Transform;
public:
	CSkeletonSingleNode();
	virtual ~CSkeletonSingleNode();
private:
	friend class CSkeletonComponent;
};

class CSkeletonComponent : public CBaseComponent
{
public:
	virtual void		Init()override;
	virtual void		Uninit()override;
	virtual void		Update()override;
public:
	BOOL				IsValid()const;
public:
	const std::string&	GetSkeletonName()const;
public:
	const CSkeletonSingleNode&		GetNodeByName(const std::string& name)const;
	const CSkeletonSingleNode&		GetNodeByNodeIndex(const USHORT& i)const;
protected:
	CSkeletonSingleNode&			GetNodeByNameNoConst(const std::string& name);
	CSkeletonSingleNode&			GetNodeByNodeIndexNoConst(const USHORT& i);
public:
	void	SetSkeleton(const std::vector<CustomStruct::CGameBoneNodeInfo>& skeleton, const std::map<std::string, SHORT>& boneIndexMap, const std::vector<USHORT>& boneList);
public:
	void	BindVSSkeletonGPUConstantBuffer(const UINT& startSlot)const;
	void	BindPSSkeletonGPUConstantBuffer(const UINT& startSlot)const;
	void	BindCSSkeletonGPUConstantBuffer(const UINT& startSlot)const;
	void	UpdateSkeletonTransformInfo();
	void	UploadSkeletonGPUConstantBuffer()const;
protected:
	void	UpdateBoneTransformMatrix(SHORT nodeIndex, CustomType::Matrix4x4 globalTransform);
protected:
	mutable BOOL									m_SkeletonPerFrameUpload;
	CustomStruct::CShaderSkeletonMatrix				m_SkeletonGPUCBufferData;
	Microsoft::WRL::ComPtr<ID3D11Buffer>			m_SkeletonGPUCBuffer;
protected:
	std::string										m_SkeletonName;
	CustomType::Matrix4x4							m_RootTransformInverse;
	SHORT											m_RootNodeIndex;
	std::vector<USHORT>								m_BoneListVector;
	std::map<std::string, USHORT>					m_NodeListMap;
	std::vector<CSkeletonSingleNode>				m_NodeListVector;
public:
	CSkeletonComponent(const std::string& skeletonName);
	virtual ~CSkeletonComponent();
private:
	friend class CSkeletonAnimationComponent;
};