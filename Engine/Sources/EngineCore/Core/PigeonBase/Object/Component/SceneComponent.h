#pragma once
#include <CoreMinimal.h>
#include <set>

#include "ActorComponent.h"
#include "Base/DataStructure/Container/Set.h"

namespace PigeonEngine
{

    class PSceneComponent : public PActorComponent
    {

		EClass(PSceneComponent, PActorComponent)

    	CLASS_VIRTUAL_NOCOPY_BODY(PSceneComponent)

    public:
    	virtual void Init() override;
    	virtual void Uninit() override;
    	virtual void Destroy() override;
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

    	void SetComponentLocation (const Vector3& Location);
    	void SetComponentRotation (const Quaternion& Rotation);
    	void SetComponentScale    (const Vector3& Scale);
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

    	// static function to attach
    	static void AttachComponentToComponent(PSceneComponent* Component, PSceneComponent* AttachTo, const ETransform& RelativeTransform = ETransform());

    	// Get this component's attached parent component.
    	PSceneComponent* GetAttachedParentComponent()const;
    	// Set this component's attached parent component, will keep the relative transform.
    	void             SetAttachedParentComponent(PSceneComponent* NewParent);
    	void             RemoveFromAttachedParent();
    private:
    	PSceneComponent* AttachedParentComponent = nullptr;

    public:
    	
		void AddChildComponent         (PSceneComponent* NewChild);
    	void RemoveChildComponent      (PSceneComponent* NewChild);
    	void ReparentChildrenComponents(PSceneComponent* NewParent);
    	void ClearChildren             ();
    	
    private:
    	TSet<PSceneComponent*> ChildrenComponents;

    public:

    	
    	
    };

};

