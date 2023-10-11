#pragma once
#include "../Component.h"
#include "../ActorComponent.h"
#include "../../../../../../../EngineThirdParty/JoltPhysics/Headers/Shapes.h"
#include "../../../../../../../EngineThirdParty/JoltPhysics/Headers/PhysicsManager.h"

namespace PigeonEngine 
{

	class PPhysicsComponent : PActorComponent
	{
	public:
		PPhysicsComponent() :m_Shape(nullptr) { POBJ_DEBUGNAME_SET(this, "PhysicsComponent"); }
		PPhysicsComponent(FShape* InShape) :m_Shape(InShape) {};
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
		void InitPhysicsComponent();
		void UninitPhysicsComponent();
		FShape* GetShape();
		/// <summary>
		/// Will delete shape & set null
		/// </summary>
		void RemoveShape();
		void AddForce(Vector3 InForce);
		void AddImpluse(Vector3 InImpluse);
	public:
		PhysicsUtility::EMotionType MotionType = PhysicsUtility::EMotionType::Static;
		UINT16						Layer = Layers::NON_MOVING;
	private:
		FShape* m_Shape = nullptr;
		FPhysicsBodyId				m_BodyId;
	};
}