#pragma once
#include <CoreMinimal.h>
#include <Base/DataStructure/Container/Map.h>
#include <Base/DataStructure/Container/Array.h>
#include <PigeonBase/Object/Component/Primitive/PrimitiveComponent.h>
#include "PhysicsUtility.h"
#include "PhysicsBodyId.h"
#include "JoltIncludes.h"
#include "JoltPhysicsLayer.h"
#include "JoltPhysicsListener.h"
#include "PhysicsManagerInterface.h"
#include "Shapes.h"
#include "PhysicsData.h"

PIGEONENGINE_NAMESPACE_BEGIN
using PhysicsUtility::EActivate;

static void TraceImpl(const CHAR* inFMT, ...)
{
	va_list list;
	va_start(list, inFMT);
	CHAR buffer[1024];
	vsnprintf(buffer, sizeof(buffer), inFMT, list);
}

class FPhysics_Jolt : public FPhysicsManagerInterface
{
public:
	FPhysics_Jolt() : PhysicsData(nullptr), LayerConfig(nullptr) {}
	virtual ~FPhysics_Jolt() {}
public:
	virtual void InitPhysics() override;
	virtual void UninitPhysics() override;
	virtual void PrePhysicsUpdate(FLOAT InDeltaTime) override;
	virtual void PhysicsUpdate(FLOAT DeltaTime) override;
	virtual void PostPhysicsUpdate(FLOAT InDeltaTime) override;
	FPhysicsData* GetPhysicsData() { return PhysicsData; }

	void AddCharacter(class FCharacterVirtual* Character);
	void RemoveCharacter(class FCharacterVirtual* Character);
	void AddPhysicsListener(FBodyActivationEventListenerInterface* InListener);
	void RemovePhysicsListener(FBodyActivationEventListenerInterface* InListener);
	void AddPhysicsListener(FContactEventListenerInterface* InListener);
	void RemovePhysicsListener(FContactEventListenerInterface* InListener);
	void AddPhysicsListener(FSoftBodyContactEventListenerInterface* InListener);
	void RemovePhysicsListener(FSoftBodyContactEventListenerInterface* InListener);

public:
	BOOL32 TryCreateBody(FShape* inShape, BOOL32 CreateNew, Vector3 inPosition, Quaternion inRotation, PhysicsUtility::EMotionType inMotionType, FPhysicsObjectLayer inLayer, const ObjectIdentityType& InObjectID, FPhysicsBodyId& outBodyID);
	void AddBody(const ObjectIdentityType& GameObjectId, const FPhysicsBodyId& inBodyID, EActivate inActivationMode = EActivate::DontActivate);
	BOOL32 FindObjectIdentityByBodyId(const FPhysicsBodyId& inBodyID, ObjectIdentityType& outGameObjectId) const;
	void RemoveBody(const ObjectIdentityType& GameObjectId, BOOL32 bDeleteShape = TRUE);

	Vector3 GetPosition(const FPhysicsBodyId& FPhysicsBodyId);
	Quaternion GetRotation(const FPhysicsBodyId& FPhysicsBodyId);
	void SetPosition(const FPhysicsBodyId& inPhysicsBodyId, Vector3 inPosition, EActivate inActivationMode = EActivate::DontActivate);
	void SetRoation(const FPhysicsBodyId& inPhysicsBodyId, Quaternion inRotation, EActivate inActivationMode = EActivate::DontActivate);
	void AddForce(const FPhysicsBodyId& inPhysicsBodyId, Vector3 inForce);
	void AddForce(const FPhysicsBodyId& inPhysicsBodyId, Vector3 inForce, Vector3 inPoint);
	void AddImpulse(const FPhysicsBodyId& inPhysicsBodyId, Vector3 inImpulse);
	void AddImpulse(const FPhysicsBodyId& inPhysicsBodyId, Vector3 inImpulse, Vector3 inPoint);

	void SetGravity(Vector3 inGravity);
	void SetLayerConfig(const FPhysicsLayerConfig& InLayerConfig);
	const FPhysicsLayerConfig& GetLayerConfig() const;
private:
	TMap<ObjectIdentityType, TArray<FPhysicsBodyId>> m_Bodys;
	TMap<FPhysicsBodyId, ObjectIdentityType> m_BodyToObjectIds;
	TMap<FPhysicsBodyId, FShape*> m_Shapes;
	TSet<class FCharacterVirtual*> m_Characters;
private:
	FPhysicsData* PhysicsData;
	const FPhysicsLayerConfig* LayerConfig;
};
PIGEONENGINE_NAMESPACE_END