#pragma once

#include "../../../../Entry/EngineMain.h"
#include "../../EngineBase/Headers/CBaseType.h"
#include "./CGameStructCommon.h"
#include "./CComponent.h"

class CSkeletonAnimationComponent : public CBaseComponent
{
public:
	enum SkeletonAnimationPlayState
	{
		SkeletonAnimationPlayState_None				= 0x0,
		SkeletonAnimationPlayState_Pause			= 0x1,
		SkeletonAnimationPlayState_Play				= 0x2,
		SkeletonAnimationPlayState_Repeat			= 0x4,
		SkeletonAnimationPlayState_PlayAndRepeat	= SkeletonAnimationPlayState_Play | SkeletonAnimationPlayState_Repeat
	};
public:
	virtual void	Init()override;
	virtual void	Uninit()override;
	virtual void	Update()override;
#ifdef _DEVELOPMENT_EDITOR
public:
	virtual void	SelectedEditorUpdate()override;
protected:
	BOOL	m_PlayRepeat;
#endif
public:
	void	Play(const BOOL& repeat = FALSE);
	void	Pause();
	void	Reset();
	void	SetCurrentAnimation(const std::string& name);
	void	SetCurrentAnimation(const UINT& index);
protected:
	const class CGameTimer*						m_GameTimer;
	SkeletonAnimationPlayState					m_PlayState;
	SkeletonAnimationPlayState					m_LastState;
	DOUBLE										m_PlayTime;
	const CustomStruct::CGameAnimationInfo*		m_CurrentPlayAnimation;
public:
	void								SetSkeletonComponent(class CSkeletonComponent* skeletonComponent);
	const class CSkeletonComponent*		GetSkeletonComponent()const;
protected:
	class CSkeletonComponent*			m_SkeletonComponent;
public:
	void										AddAnimation(const std::string& name, const CustomStruct::CGameAnimationInfo* animation);
	void										RemoveAnimation(const std::string& name);
	void										RemoveAllAnimation();
	const CustomStruct::CGameAnimationInfo*		FindAnimation(const std::string& name)const;
protected:
	std::map<std::string, const CustomStruct::CGameAnimationInfo*>	m_AnimationList;
public:
	CSkeletonAnimationComponent();
	virtual ~CSkeletonAnimationComponent();
};