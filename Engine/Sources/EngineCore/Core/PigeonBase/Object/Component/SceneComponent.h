#pragma once
#include <CoreMinimal.h>
#include <set>

#include "ActorComponent.h"
#include "Base/DataStructure/Container/Set.h"

namespace PigeonEngine
{

	class PWorld;
	class PActor;
    // Render proxy functions START
    struct ERenderTransformInfo
    {
    public:
        Vector3     WorldLocation;
        Quaternion  WorldRotation;
        Vector3     WorldScaling;
    public:
        ERenderTransformInfo(const Vector3& InWorldLocation, const Quaternion& InWorldRotation, const Vector3& InWorldScaling)
            : WorldLocation(InWorldLocation), WorldRotation(InWorldRotation), WorldScaling(InWorldScaling)
        {
        }
        ERenderTransformInfo() = default;
        ERenderTransformInfo(const ERenderTransformInfo& Other)
            : WorldLocation(Other.WorldLocation), WorldRotation(Other.WorldRotation), WorldScaling(Other.WorldScaling)
        {
        }
        void CopyFromOther(const ERenderTransformInfo& Other)
        {
            WorldLocation   = Other.WorldLocation;
            WorldRotation   = Other.WorldRotation;
            WorldScaling    = Other.WorldScaling;
        }
        ERenderTransformInfo& operator=(const ERenderTransformInfo& Other)
        {
            CopyFromOther(Other);
            return (*this);
        }
    };
    // Render proxy functions END

    class PSceneComponent : public PActorComponent
    {
    	CLASS_VIRTUAL_NOCOPY_BODY(PSceneComponent)
		friend class PWorld;
		friend class PActor;
    public:
    	virtual void Init() override;
    	virtual void Uninit() override;
    	virtual void Destroy() override;

    protected:
    	
    	virtual void BeginAddedToScene(PWorld* World);
    	virtual void RemovedFromScene();
    private:
    	PWorld* MyWolrd;
    public:
    	PE_NODISCARD  EMobilityType GetMobility() const;
    	void                        SetMobility(const EMobilityType& NewMobility);
    private:
    	EMobilityType Mobility = EMobilityType::EMT_DYNAMIC;
	public:
		PE_NODISCARD BOOL32 ContainTransform()const override;

    	PE_NODISCARD Vector3    GetComponentLocalLocation() const;
    	PE_NODISCARD Quaternion GetComponentLocalRotation() const;
    	PE_NODISCARD Vector3    GetComponentLocalScale() const;

    	PE_NODISCARD Vector3    GetComponentWorldLocation() const;
    	PE_NODISCARD Quaternion GetComponentWorldRotation() const;
    	PE_NODISCARD Vector3    GetComponentWorldScale() const;

    	PE_NODISCARD Vector3 GetComponentForwardVector()const;
    	PE_NODISCARD Vector3 GetComponentRightVector()const;
    	PE_NODISCARD Vector3 GetComponentUpVector()const;
    	PE_NODISCARD const ETransform* GetTransform()const ;

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
    	void AttachToComponent(PSceneComponent* Parent, const ETransform& RelativeTransform = ETransform());
    
    	// static function to attach
    	static void AttachComponentToComponent(PSceneComponent* Child, PSceneComponent* Parent, const ETransform& RelativeTransform = ETransform());

    	// Get this component's attached parent component.
    	PE_NODISCARD PSceneComponent* GetAttachedParentComponent()const;

    protected:
    	// Set this component's attached parent component, will keep the relative transform.
    	void             SetAttachedParentComponent(PSceneComponent* NewParent);
    	void             RemoveFromAttachedParent();
    private:
    	PSceneComponent* AttachedParentComponent = nullptr;

    public:
    	
    	void RemoveChildComponent(PSceneComponent* NewChild);
    	void ClearChildren       ();
    	
    private:
    	TSet<PSceneComponent*> ChildrenComponents;

    
    public:
        TSharedPtr<CJsonObject> Serialize() override;

        // Render proxy functions START
    public:
        BOOL32          IsRenderTransformDirty()const;
        virtual void	CreateRenderState()override;
        virtual void	DestroyRenderState()override;
        virtual void	SendUpdateRenderState()override;
    protected:
        virtual void    MarkRenderTransformAsDirty();
        virtual void	MarkRenderStateAsDirty()override;
        virtual void	CleanMarkRenderStateDirty()override;
    protected:
        BOOL32          RenderTransformDirty = FALSE;
        // Render proxy functions END
    	
    };

};

