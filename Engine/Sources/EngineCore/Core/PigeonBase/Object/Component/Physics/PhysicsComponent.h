#pragma once
#include "../Component.h"
#include "../ActorComponent.h"
#include "../../../../../../../EngineThirdParty/JoltPhysics/Headers/Shapes.h"
#include "../../../../../../../EngineThirdParty/JoltPhysics/Headers/PhysicsManager.h"

namespace PigeonEngine 
{
	class PPhysicsComponent : public PActorComponent, public FBodyActivationEventListenerInterface, public FContactEventListenerInterface
	{
	public:
		PPhysicsComponent() : m_Shape(nullptr)
		{
			POBJ_DEBUGNAME_SET(this, "PhysicsComponent");
		}
		PPhysicsComponent(FShape* InShape) : m_Shape(InShape)
		{
			POBJ_DEBUGNAME_SET(this, "PhysicsComponent");
			//InitPhysicsComponent();
		}
		virtual ~PPhysicsComponent();

		virtual void Init() override;
		virtual void Uninit() override;
	public:
		virtual void Destroy() override;

#if _EDITOR_ONLY
		virtual void EditorTick(FLOAT deltaTime) override;

		virtual void DrawPrimitive();
#endif
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
		void SetLayer(FPhysicsObjectLayer InLayer);
		FPhysicsObjectLayer GetLayer() const;
		const FPhysicsBodyId& GetBodyId() const;

	private://Listener interface functions
		virtual void OnBodyActivated(const FPhysicsBodyId& inBodyID, const ObjectIdentityType& inObjectID) override;
		virtual void OnBodyDeactivated(const FPhysicsBodyId& inBodyID, const ObjectIdentityType& inObjectID) override;
		virtual EPhysicsContactValidateResult OnContactValidate(const FPhysicsBodyId& inBodyID1, const FPhysicsBodyId& inBodyID2, const Vector3& inBaseOffset, const FPhysicsContactValidateResult& inCollisionResult) override;
		virtual void OnContactAdded(const FPhysicsBodyId& inBodyID1, const FPhysicsBodyId& inBodyID2, const FPhysicsContactManifold& inManifold, FPhysicsContactSettings& ioSettings) override;
		virtual void OnContactPersisted(const FPhysicsBodyId& inBodyID1, const FPhysicsBodyId& inBodyID2, const FPhysicsContactManifold& inManifold, FPhysicsContactSettings& ioSettings) override;
		virtual void OnContactRemoved(const FPhysicsSubShapePair& inSubShapePair) override;

	protected:
        virtual void BeginAddedToScene(PWorld* World) override;
        virtual void RemovedFromScene() override;
		virtual void OnPhysicsBodyActivated(const FPhysicsBodyId& inBodyID, const ObjectIdentityType& inObjectID) {}
		virtual void OnPhysicsBodyDeactivated(const FPhysicsBodyId& inBodyID, const ObjectIdentityType& inObjectID) {}
		virtual EPhysicsContactValidateResult OnPhysicsContactValidate(const FPhysicsBodyId& inBodyID1, const FPhysicsBodyId& inBodyID2, const Vector3& inBaseOffset, const FPhysicsContactValidateResult& inCollisionResult)
		{
			return EPhysicsContactValidateResult::AcceptAllContactsForThisBodyPair;
		}
		virtual void OnPhysicsContactAdded(const FPhysicsBodyId& inBodyID1, const FPhysicsBodyId& inBodyID2, const FPhysicsContactManifold& inManifold, FPhysicsContactSettings& ioSettings) {}
		virtual void OnPhysicsContactPersisted(const FPhysicsBodyId& inBodyID1, const FPhysicsBodyId& inBodyID2, const FPhysicsContactManifold& inManifold, FPhysicsContactSettings& ioSettings) {}
		virtual void OnPhysicsContactRemoved(const FPhysicsSubShapePair& inSubShapePair) {}
	public:
		PhysicsUtility::EMotionType MotionType = PhysicsUtility::EMotionType::Static;
		FPhysicsObjectLayer Layer = Layers::NON_MOVING;
	private:
		BOOL32 ContainsBody(const FPhysicsBodyId& InBodyID) const;
		BOOL32 ContainsAnyBody(const FPhysicsBodyId& InBodyID1, const FPhysicsBodyId& InBodyID2) const;
        void TryRegisterPostPhysicsTick();
        void TryUnregisterPostPhysicsTick();
		void HandlePostPhysicsTick(FLOAT deltaTime);
		FShape* m_Shape = nullptr;
		FPhysicsBodyId m_BodyId;
		TFunction<void(FLOAT)> PostPhysicsTickHandler;
		BOOL32 bPostPhysicsTickRegistered = FALSE;
	};
}