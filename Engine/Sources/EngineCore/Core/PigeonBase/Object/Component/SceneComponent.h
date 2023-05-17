#pragma once
#include <CoreMinimal.h>
#include <set>

#include "ActorComponent.h"

namespace PigeonEngine
{

    class PSceneComponent : public PActorComponent
    {

		EClass(PSceneComponent, PActorComponent)

    	CLASS_VIRTUAL_NOCOPY_BODY(PSceneComponent)
	public:
		virtual BOOL ContainTransform()const override;

    	Vector3    GetComponentLocalLocation() const;
    	Quaternion GetComponentLocalRotation() const;
    	Vector3    GetComponentLocalScale() const;

    	Vector3    GetComponentWorldLocation() const;
    	Quaternion GetComponentWorldRotation() const;
    	Vector3    GetComponentWorldScale() const;

    	Vector3 GetComponentForwardVector()const;
    	Vector3 GetComponentRightVector()const;
    	Vector3 GetComponentUpVector()const;
    	const ETransform* GetTransform()const ;

    	void SetComponentLocation(const Vector3& Location);
    	void SetComponentRotation(const Quaternion& Rotation);
    	void SetComponentScale   (const Vector3& Scale);
		void SetComponentTransform(const ETransform& Trans);
    	
    	void SetComponentWorldLocation(const Vector3& Location);
    	void SetComponentWorldRotation(const Quaternion& Rotation);
    	void SetComponentWorldScale   (const Vector3& Scale);
    	void SetComponentWorldTransform(const ETransform& Trans);

	protected:
        ETransform	Transform;
    	
    public:
    	
    	// Attach this component to another component
    	void AttachToComponent(PSceneComponent* AttachTo, const ETransform& RelativeTransform = ETransform());
    	// Attach another component to this
    	void AttachComponentTo(PSceneComponent* Component, const ETransform& RelativeTransform = ETransform());
    	static void AttachComponentToComponent(PSceneComponent* Component, PSceneComponent* AttachTo, const ETransform& RelativeTransform = ETransform());
    	PSceneComponent* GetAttachedParentComponent()const;
    private:
    	PSceneComponent* AttachedParentComponent = nullptr;
    	std::set<PSceneComponent*> ChildrenComponents;
    };

};

