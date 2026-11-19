#pragma once
#include "../Component.h"
#include "../ActorComponent.h"
#include "../../../../../../../EngineThirdParty/JoltPhysics/Headers/Shapes.h"
#include "../../../../../../../EngineThirdParty/JoltPhysics/Headers/PhysicsManager.h"

namespace PigeonEngine 
{
	class PPhysicsComponent : public PActorComponent, public FBodyActivationEventListenerInterface, public FContactEventListenerInterface, public FPhysicsContactModifierInterface
	{
	public:
		PPhysicsComponent() : Shape(nullptr)
		{
			POBJ_DEBUGNAME_SET(this, "PhysicsComponent");
		}
		PPhysicsComponent(FShape* InShape) : Shape(InShape)
		{
			POBJ_DEBUGNAME_SET(this, "PhysicsComponent");
			//InitPhysicsComponent();
		}
		virtual ~PPhysicsComponent();

		virtual void Init() override;
		virtual void Uninit() override;
	public:
		// Serialization/editor access. Same ownership contract as the protected SetShape:
		// the component hosts the shape and deletes it on teardown.
		PE_NODISCARD FShape* GetShapeForSerialization() const { return Shape; }
		void SetShapeForSerialization(FShape* InShape) { SetShape(InShape); }
	protected:
		// Removes the physics body while the owner actor is still set; the old Destroy()
		// cleared the owner before Uninit, making this same removal dereference null.
		virtual void OnDestroy() override;

#if ENGINE_EDITOR_ONLY
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
		// Game thread, after PhysicsSystem::Update.
		virtual void OnBodyActivated(const FPhysicsBodyId& inBodyID, const ObjectIdentityType& inObjectID) override;
		virtual void OnBodyDeactivated(const FPhysicsBodyId& inBodyID, const ObjectIdentityType& inObjectID) override;
		virtual void OnContactAdded(const FPhysicsContactEvent& InEvent) override;
		virtual void OnContactPersisted(const FPhysicsContactEvent& InEvent) override;
		virtual void OnContactRemoved(const FPhysicsContactEvent& InEvent) override;

		// Physics worker thread, inside PhysicsSystem::Update. All bodies are locked:
		// read only, never touch game objects.
		virtual EPhysicsContactValidateResult OnContactValidate(const FPhysicsBodyId& inBodyID1, const FPhysicsBodyId& inBodyID2, const Vector3& inBaseOffset, const FPhysicsContactValidateResult& inCollisionResult) override;
		virtual void OnContactSettingsAdded(const FPhysicsBodyId& inBodyID1, const FPhysicsBodyId& inBodyID2, const FPhysicsContactManifold& inManifold, FPhysicsContactSettings& ioSettings) override;
		virtual void OnContactSettingsPersisted(const FPhysicsBodyId& inBodyID1, const FPhysicsBodyId& inBodyID2, const FPhysicsContactManifold& inManifold, FPhysicsContactSettings& ioSettings) override;

	protected:
		virtual void BeginAddedToScene(PWorld* World) override;
		virtual void RemovedFromScene() override;

		// Game thread. Safe to create or destroy actors, play sounds and call the
		// physics interfaces.
		virtual void OnPhysicsBodyActivated(const FPhysicsBodyId& inBodyID, const ObjectIdentityType& inObjectID) {}
		virtual void OnPhysicsBodyDeactivated(const FPhysicsBodyId& inBodyID, const ObjectIdentityType& inObjectID) {}
		virtual void OnPhysicsContactAdded(const FPhysicsContactEvent& InEvent) {}
		virtual void OnPhysicsContactPersisted(const FPhysicsContactEvent& InEvent) {}
		virtual void OnPhysicsContactRemoved(const FPhysicsContactEvent& InEvent) {}

		// Physics worker thread. All bodies are locked, only reads are allowed and
		// changes to the game state are not safe here. Use the deferred hooks above
		// for anything that is not required to alter the simulation.
		virtual EPhysicsContactValidateResult OnPhysicsContactValidate(const FPhysicsBodyId& inBodyID1, const FPhysicsBodyId& inBodyID2, const Vector3& inBaseOffset, const FPhysicsContactValidateResult& inCollisionResult)
		{
			return EPhysicsContactValidateResult::AcceptAllContactsForThisBodyPair;
		}
		virtual void OnPhysicsContactSettingsAdded(const FPhysicsBodyId& inBodyID1, const FPhysicsBodyId& inBodyID2, const FPhysicsContactManifold& inManifold, FPhysicsContactSettings& ioSettings) {}
		virtual void OnPhysicsContactSettingsPersisted(const FPhysicsBodyId& inBodyID1, const FPhysicsBodyId& inBodyID2, const FPhysicsContactManifold& inManifold, FPhysicsContactSettings& ioSettings) {}
	public:
		PhysicsUtility::EMotionType MotionType = PhysicsUtility::EMotionType::Static;
		FPhysicsObjectLayer Layer = Layers::NON_MOVING;
	private:
		BOOL32 ContainsBody(const FPhysicsBodyId& InBodyID) const;
		BOOL32 ContainsAnyBody(const FPhysicsBodyId& InBodyID1, const FPhysicsBodyId& InBodyID2) const;
	protected:
		virtual void HandlePostPhysicsTick(FLOAT deltaTime) override;
	private:
		FShape* Shape = nullptr;
		FPhysicsBodyId BodyId;
		BOOL32 bBodyActive = FALSE;
		BOOL32 bHasSyncedTransform = FALSE;
		Vector3 LastSyncedPosition = Vector3::Zero();
		Quaternion LastSyncedRotation = Quaternion::Identity();
	};
}
