#pragma once
#include "../Component.h"
#include "../ActorComponent.h"
#include "../../../../../../../EngineThirdParty/JoltPhysics/Headers/Shapes.h"
#include "../../../../../../../EngineThirdParty/JoltPhysics/Headers/PhysicsManager.h"

#ifndef PIGEONENGINE_NAMESPACE_BEGIN
#define PIGEONENGINE_NAMESPACE_BEGIN namespace PigeonEngine {
#endif
#ifndef PIGEONENGINE_NAMESPACE_END
#define PIGEONENGINE_NAMESPACE_END }
#endif

PIGEONENGINE_NAMESPACE_BEGIN

class PPhysicsComponent : PActorComponent
{
public:
	PPhysicsComponent():m_Shape(nullptr){};
	PPhysicsComponent(FShape* InShape):m_Shape(InShape){};
	virtual ~PPhysicsComponent();
	
	virtual void Init() override;
	virtual void Uninit() override;
public:
	virtual void Destroy() override;
	/// <summary>
	/// Add a Shape and host it;
	/// </summary>
	/// <param name="InShape">Hosted Shape, please do not delete it outside.</param>
	void SetShape(FShape* InShape);
	FShape* GetShape();
	/// <summary>
	/// Will delete shape & set null
	/// </summary>
	void RemoveShape();
	void AddForce(Vector3 InForce);
public:
	PhysicsUtility::EMotionType MotionType = PhysicsUtility::EMotionType::Static;
	UINT16						Layer = Layers::NON_MOVING;
private:
	FShape*						m_Shape = nullptr;
	FPhysicsBodyId				m_BodyId;
};
PIGEONENGINE_NAMESPACE_END