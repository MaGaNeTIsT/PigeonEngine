#pragma once
#include "../SceneComponent.h"
#include <Base/DataStructure/Container/Set.h>
#include "../../../../../../../EngineThirdParty/JoltPhysics/Headers/Shapes.h"
#include "../../../../../../../EngineThirdParty/JoltPhysics/Headers/PhysicsManager.h"

PIGEONENGINE_NAMESPACE_BEGIN

/*
* Overlap volume backend.
*
* Query: every post-physics tick runs FPhysicsQuery::OverlapShape and diffs the
*        result against the previous tick. It sees sleeping, static and moving
*        bodies and has exact enter/exit semantics. Cost is one shape query per
*        trigger per fixed tick.
*
* Sensor: creates a sensor rigid body and receives the deferred contact events.
*         Cheaper when nothing overlaps, but Jolt does not run CCD for sensors
*         (fast objects can tunnel through thin volumes) and Layers::SENSOR only
*         collides with MOVING. A moving sensor follows the component with one
*         fixed-tick delay, so use Query when both movement and exact timing
*         matter.
*
* CharacterVirtual is not a rigid body, so both backends detect characters
* through the engine's character registry (an extra shape test per character).
*/
enum class ETriggerBackendType : UINT8
{
	QUERY = 0,
	SENSOR = 1,
};

/*
* Gameplay overlap volume. Derive and override OnTriggerEnter/Exit/Stay, or
* assign the TFunction callbacks. All callbacks run on the game thread after
* PhysicsSystem::Update, so they may create/destroy actors and call the physics
* interfaces.
*
* Do not destroy or reconfigure the trigger from inside its own callbacks: the
* dispatcher is iterating the current/previous overlap maps. Defer that work
* (e.g. to the next tick or the game thread command queue).
*/
class PTriggerComponent : public PSceneComponent, public FContactEventListenerInterface
{
	CLASS_VIRTUAL_NOCOPY_BODY(PTriggerComponent)
public:
	virtual void Init() override;
	virtual void Uninit() override;

	/*
	* The component owns InShape when InIsOwnShape is TRUE. Set the shape before
	* the actor joins the scene; changing it later is supported and rebuilds the
	* sensor body in Sensor mode.
	*/
	void SetShape(FShape* InShape, BOOL32 InIsOwnShape = TRUE);
	PE_NODISCARD FShape* GetShape() const;

	void SetBackend(ETriggerBackendType InBackend);
	PE_NODISCARD ETriggerBackendType GetBackend() const;
	PE_NODISCARD const FPhysicsBodyId& GetSensorBodyId() const;

	/// Overlaps collected by the last UpdateOverlaps call.
	PE_NODISCARD const TArray<FOverlapResult>& GetOverlaps() const;
	PE_NODISCARD BOOL32 IsOverlapping(const ObjectIdentityType& InObjectId) const;

public:
	// Simple wiring form. The virtual OnTrigger* hooks call these when valid.
	TFunction<void(const FOverlapResult&)> OnTriggerEnterCallback;
	TFunction<void(const FOverlapResult&)> OnTriggerExitCallback;
	TFunction<void(const FOverlapResult&)> OnTriggerStayCallback;

	// Sensor backend body settings. Kinematic sensors are active, never sleep
	// automatically and detect sleeping bodies; Layers::SENSOR only collides
	// with MOVING, which matches Jolt's recommendation for sensors.
	PhysicsUtility::EMotionType SensorMotionType = PhysicsUtility::EMotionType::Kinematic;
	FPhysicsObjectLayer SensorLayer = Layers::SENSOR;

	// Query backend filters. Empty layer filter = every layer.
	FPhysicsQueryLayerFilter QueryLayerFilter;
	BOOL32 bDetectStaticBodies = FALSE;
	BOOL32 bDetectCharacters = TRUE;
	BOOL32 bIgnoreOwnerActor = TRUE;
	BOOL32 bFireStayEvents = FALSE;
	UINT32 MaxOverlaps = 64u;

protected:
	virtual void OnTriggerEnter(const FOverlapResult& InOther);
	virtual void OnTriggerExit(const FOverlapResult& InOther);
	virtual void OnTriggerStay(const FOverlapResult& InOther);

	virtual void BeginAddedToScene(PWorld* World) override;
	virtual void RemovedFromScene() override;

	// FContactEventListenerInterface (Sensor backend, game thread)
	virtual void OnContactAdded(const FPhysicsContactEvent& InEvent) override;
	virtual void OnContactPersisted(const FPhysicsContactEvent& InEvent) override;
	virtual void OnContactRemoved(const FPhysicsContactEvent& InEvent) override;

private:
	struct FSensorBodyOverlap
	{
		ObjectIdentityType ObjectId = 0ull;
		TSet<UINT64> SubShapePairs;
	};

	void TryRegisterPostPhysicsTick();
	void TryUnregisterPostPhysicsTick();
	void HandlePostPhysicsTick(FLOAT DeltaTime);
	void UpdateOverlaps(FLOAT DeltaTime);
	void CollectCurrentOverlaps();
	void DispatchOverlapEvents();
	void ClearOverlaps(BOOL32 bFireExitEvents);

	void CreateSensorBody();
	void DestroySensorBody();
	void UpdateSensorBodyTransform();

	PE_NODISCARD ObjectIdentityType GetIgnoredObjectId() const;

private:
	FShape* Shape = nullptr;
	BOOL32 bOwnShape = FALSE;
	ETriggerBackendType Backend = ETriggerBackendType::QUERY;

	FPhysicsBodyId SensorBodyId;
	TMap<FPhysicsBodyId, FSensorBodyOverlap> SensorOverlaps;
	Vector3 LastSensorLocation = Vector3::Zero();
	Quaternion LastSensorRotation = Quaternion::Identity();

	TMap<FPhysicsBodyId, FOverlapResult> PreviousBodies;
	TMap<FPhysicsBodyId, FOverlapResult> CurrentBodies;
	TMap<ObjectIdentityType, FOverlapResult> PreviousCharacters;
	TMap<ObjectIdentityType, FOverlapResult> CurrentCharacters;
	TArray<FOverlapResult> CurrentOverlaps;

	TFunction<void(FLOAT)> PostPhysicsTickHandler;
	BOOL32 bPostPhysicsTickRegistered = FALSE;
};

PIGEONENGINE_NAMESPACE_END
