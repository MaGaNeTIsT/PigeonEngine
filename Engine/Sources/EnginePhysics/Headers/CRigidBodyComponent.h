#pragma once
#include "../../../../Entry/EngineMain.h"
#include "../../EngineGame/Headers/CComponent.h"
#include "../../EngineGame/Headers/CGameObject.h"
#include "CPhysics_API_Jolt.h"
#include "CPhysicsManager.h"
#include "CPhysicsUtility.h"
#include <Jolt/Jolt.h>

class CRigidBodyComponent : CBaseComponent
{
private:
	BodyID m_Body;
public:
	CRigidBodyComponent() : 
		CBaseComponent(TRUE, FALSE, TRUE),
		m_Body(0)
	{
	}

	virtual void SetGameObjectHook(const class CGameObject* gameObject) override;

	inline void AddForce(CustomType::Vector3 inForce);
	inline void AddForce(CustomType::Vector3 inForce, CustomType::Vector3 inPoint);
	inline void AddImpulse(CustomType::Vector3 inImpulse);
	inline void AddImpulse(CustomType::Vector3 inImpulse, CustomType::Vector3 inPoint);
};