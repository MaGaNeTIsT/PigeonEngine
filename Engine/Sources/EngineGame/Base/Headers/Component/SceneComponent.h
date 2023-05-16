#pragma once
#include <Main.h>
#include <set>

#include "./ActorComponent.h"
#include "./Transform.h"

namespace PigeonEngine
{

    class PSceneComponent : public PActorComponent
    {

		EClass(PSceneComponent, PActorComponent)

	
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

	protected:
        ETransform	Transform;
		

        CLASS_VIRTUAL_NOCOPY_BODY(PSceneComponent)

    public:
    	// WIP
    	// 1 attach rules
    	// 2 relative transform

    	// Attach this component to another component
    	void AttachToComponent(PSceneComponent* AttachTo);
    	// Attach another component to this
    	void AttachComponentTo(PSceneComponent* Component);
    	static void AttachComponentToComponent(PSceneComponent* Component, PSceneComponent* AttachTo);
    	PSceneComponent* GetAttachedParentComponent()const;
    private:
    	PSceneComponent* AttachedParentComponent = nullptr;
    	std::set<PSceneComponent*> ChildrenComponents;
    };

};

