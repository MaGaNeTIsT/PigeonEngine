#pragma once
#include "../../../../Entry/EngineMain.h"
#include "../../EngineGame/Headers/CComponent.h"
#include "../../EngineGame/Headers/CGameObject.h"
#include <Jolt/Jolt.h>
#include "CPhysicsManager.h"
#include "CPhysicsUtility.h"

class CRigidBodyComponent :public CBaseComponent
{
private:
	BodyID			m_Body;
	EMotionType		m_EMotionType;
	UINT16			m_ObjectLayer;
	EActivation		m_EActivation;
public:
	CRigidBodyComponent() :
		CBaseComponent(TRUE, FALSE, TRUE),
		m_Body(0),
		m_EMotionType(EMotionType::Static),
		m_ObjectLayer(0),
		m_EActivation(EActivation::Activate)
	{
	}

	CRigidBodyComponent(EMotionType inEMotionType, UINT16 inObjectLayer, BOOL inEActivation) :
		CBaseComponent(TRUE, FALSE, TRUE),
		m_Body(0),
		m_EMotionType(inEMotionType),
		m_ObjectLayer(inObjectLayer),
		m_EActivation(inEActivation == TRUE ? EActivation::Activate: EActivation::DontActivate)
	{
	}
	virtual ~CRigidBodyComponent() {}

	virtual void	Init() {}
	virtual void	Uninit() {}
protected:
	virtual void SetGameObjectHook(const class CGameObject* gameObject) override;
public:
	inline void AddForce(CustomType::Vector3 inForce);
	inline void AddForce(CustomType::Vector3 inForce, CustomType::Vector3 inPoint);
	inline void AddImpulse(CustomType::Vector3 inImpulse);
	inline void AddImpulse(CustomType::Vector3 inImpulse, CustomType::Vector3 inPoint);
};