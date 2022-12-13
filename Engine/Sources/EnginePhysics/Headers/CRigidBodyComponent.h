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

	void			SetGameObject(const class CGameObject* gameObject);

	virtual void	FixedUpdate() override;
};