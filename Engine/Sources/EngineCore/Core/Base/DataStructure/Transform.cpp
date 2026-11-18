#include "Transform.h"
#include <PigeonBase/Object/Component/SceneComponent.h>
#include <PigeonBase/Object/Actor.h>

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<ETransform, EObjectBase>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	ETransform::ETransform(const Vector3& InLocation, const Quaternion& InRotation, const Vector3& InScaling)
		: LocalLocation(InLocation), LocalRotation(InRotation), LocalScaling(InScaling)
	{
	}
	ETransform::ETransform() : LocalLocation(Vector3::Zero()), LocalRotation(Quaternion::Identity()), LocalScaling(Vector3::One())
	{
	}
	ETransform::~ETransform()
	{
	}
	ETransform::ETransform(const ETransform& other) : LocalLocation(other.LocalLocation), LocalRotation(other.LocalRotation), LocalScaling(other.LocalScaling)
	{
	}
	ETransform& ETransform::operator=(const ETransform& other)
	{
		LocalLocation = other.LocalLocation;
		LocalRotation = other.LocalRotation;
		LocalScaling = other.LocalScaling;
		return (*this);
	}
	Matrix4x4 ETransform::GetLocalToWorldMatrix(const PSceneComponent* InParentComponent, const PActor* InParentActor) const
	{
		return (MakeMatrix4x4(
			GetLocalToWorldLocationInternal(InParentComponent, InParentActor),
			GetLocalToWorldRotationInternal(InParentComponent, InParentActor),
			GetLocalToWorldScalingInternal(InParentComponent, InParentActor)));
	}
	Matrix4x4 ETransform::GetWorldToLocalMatrix(const PSceneComponent* InParentComponent, const PActor* InParentActor) const
	{
		Matrix4x4 Result(MakeMatrix4x4(
			GetLocalToWorldLocationInternal(InParentComponent, InParentActor),
			GetLocalToWorldRotationInternal(InParentComponent, InParentActor),
			GetLocalToWorldScalingInternal(InParentComponent, InParentActor)));
		return (Result.Inverse());
	}
	Vector3 ETransform::GetLocalToActorLocationInternal(const PSceneComponent* InParentComponent) const
	{
		Vector3 Result(LocalLocation);
		GetLocalToActorLocationInternal(Result, InParentComponent);
		return Result;
	}
	Quaternion ETransform::GetLocalToActorRotationInternal(const PSceneComponent* InParentComponent) const
	{
		Quaternion Result(LocalRotation);
		GetLocalToActorRotationInternal(Result, InParentComponent);
		return Result;
	}
	Vector3 ETransform::GetLocalToActorScalingInternal(const PSceneComponent* InParentComponent) const
	{
		Vector3 Result(LocalScaling);
		GetLocalToActorScalingInternal(Result, InParentComponent);
		return Result;
	}
	Vector3 ETransform::GetLocalToWorldLocationInternal(const PSceneComponent* InParentComponent, const PActor* InParentActor) const
	{
		// The parent chain's own locations already live in actor/world units and are not scaled
		// again; only this component's offset is expressed in the parent's local space, so the
		// parent chain's scaling applies to it. Without this the render transform and the bound
		// composition (which does scale the offset) disagreed, and a component lifted by half the
		// actor's height ended up thousands of units above the mesh in the bounds overlay.
		Vector3 ParentChainLocation(Vector3::Zero());
		GetLocalToActorLocationInternal(ParentChainLocation, InParentComponent);
		Vector3 ParentChainScaling(Vector3::One());
		GetLocalToActorScalingInternal(ParentChainScaling, InParentComponent);
		Vector3 RootToWorld(Vector3::Zero());
		GetRootToWorldLocationInternal(RootToWorld, InParentActor);
		Vector3 RootToWorldScaling(Vector3::One());
		GetRootToWorldScalingInternal(RootToWorldScaling, InParentActor);
		return (RootToWorld + ParentChainLocation + LocalLocation * ParentChainScaling * RootToWorldScaling);
	}
	Quaternion ETransform::GetLocalToWorldRotationInternal(const PSceneComponent* InParentComponent, const PActor* InParentActor) const
	{
		Quaternion LocalToActor(GetLocalToActorRotationInternal(InParentComponent));
		Quaternion RootToWorld(Quaternion::Identity());
		GetRootToWorldRotationInternal(RootToWorld, InParentActor);
		return (RootToWorld * LocalToActor);
	}
	Vector3 ETransform::GetLocalToWorldScalingInternal(const PSceneComponent* InParentComponent, const PActor* InParentActor) const
	{
		Vector3 LocalToActor(GetLocalToActorScalingInternal(InParentComponent));
		Vector3 RootToWorld(Vector3::One());
		GetRootToWorldScalingInternal(RootToWorld, InParentActor);
		return (RootToWorld * LocalToActor);
	}
	void ETransform::GetLocalToActorLocationInternal(Vector3& InOutLocation, const PSceneComponent* InParentComponent) const
	{
		if (!InParentComponent)
		{
			return;
		}
		InOutLocation = InParentComponent->GetTransform().LocalLocation + InOutLocation;
		// if (const ETransform& ParentTransform = InParentComponent->GetTransform(); ParentTransform)
		// {
		// 	InOutLocation = ParentTransform.LocalLocation + InOutLocation;
		// }
		GetLocalToActorLocationInternal(InOutLocation, InParentComponent->GetParentComponent());
	}
	void ETransform::GetLocalToActorRotationInternal(Quaternion& InOutRotation, const PSceneComponent* InParentComponent) const
	{
		if (!InParentComponent)
		{
			return;
		}
		InOutRotation = InParentComponent->GetTransform().LocalRotation * InOutRotation;
		// if (const ETransform* ParentTransform = InParentComponent->GetTransform(); ParentTransform)
		// {
		// 	InOutRotation = ParentTransform->LocalRotation * InOutRotation;
		// }
		GetLocalToActorRotationInternal(InOutRotation, InParentComponent->GetParentComponent());
	}
	void ETransform::GetLocalToActorScalingInternal(Vector3& InOutScaling, const PSceneComponent* InParentComponent) const
	{
		if (!InParentComponent)
		{
			return;
		}
		InOutScaling = InParentComponent->GetTransform().LocalScaling * InOutScaling;
		// if (const ETransform* ParentTransform = InParentComponent->GetTransform(); ParentTransform)
		// {
		// 	InOutScaling = ParentTransform->LocalScaling * InOutScaling;
		// }
		GetLocalToActorScalingInternal(InOutScaling, InParentComponent->GetParentComponent());
	}
	void ETransform::GetRootToWorldLocationInternal(Vector3& InOutLocation, const PActor* InParentActor) const
	{
		if (!InParentActor)
		{
			return;
		}
		if (const PSceneComponent* ParentRootComponent = InParentActor->GetRootComponent(); ParentRootComponent)
		{
			InOutLocation = ParentRootComponent->GetTransform().LocalLocation + InOutLocation;
			// if (const ETransform* TempTransform = ParentRootComponent->GetTransform(); TempTransform)
			// {
			// 	InOutLocation = TempTransform->LocalLocation + InOutLocation;
			// }
		}
		GetRootToWorldLocationInternal(InOutLocation, InParentActor->GetAttachedParentActor());
	}
	void ETransform::GetRootToWorldRotationInternal(Quaternion& InOutRotation, const PActor* InParentActor) const
	{
		if (!InParentActor)
		{
			return;
		}
		if (const PSceneComponent* ParentRootComponent = InParentActor->GetRootComponent(); ParentRootComponent)
		{
			InOutRotation = ParentRootComponent->GetTransform().LocalRotation * InOutRotation;
			// if (const ETransform* TempTransform = ParentRootComponent->GetTransform(); TempTransform)
			// {
			// 	InOutRotation = TempTransform->LocalRotation * InOutRotation;
			// }
		}
		GetRootToWorldRotationInternal(InOutRotation, InParentActor->GetAttachedParentActor());
	}
	void ETransform::GetRootToWorldScalingInternal(Vector3& InOutScaling, const PActor* InParentActor) const
	{
		if (!InParentActor)
		{
			return;
		}
		if (const PSceneComponent* ParentRootComponent = InParentActor->GetRootComponent(); ParentRootComponent)
		{
			InOutScaling = ParentRootComponent->GetTransform().LocalScaling * InOutScaling;
			// if (const ETransform* TempTransform = ParentRootComponent->GetTransform(); TempTransform)
			// {
			// 	InOutScaling = TempTransform->LocalScaling * InOutScaling;
			// }
		}
		GetRootToWorldScalingInternal(InOutScaling, InParentActor->GetAttachedParentActor());
	}
	void ETransform::SetLocalToActorLocationInternal(const Vector3& NewLocation, const PSceneComponent* InParentComponent)
	{
		Vector3 LocalToActor(Vector3::Zero());
		GetLocalToActorLocationInternal(LocalToActor, InParentComponent);
		LocalLocation = NewLocation - LocalToActor;
	}
	void ETransform::SetLocalToActorRotationInternal(const Quaternion& NewRotation, const PSceneComponent* InParentComponent)
	{
		Quaternion ActorToLocal(Quaternion::Identity());
		GetLocalToActorRotationInternal(ActorToLocal, InParentComponent);
		ActorToLocal.Inverse();
		LocalRotation = ActorToLocal * NewRotation;
	}
	void ETransform::SetLocalToActorScalingInternal(const Vector3& NewScaling, const PSceneComponent* InParentComponent)
	{
		Vector3 LocalToActor(Vector3::One());
		GetLocalToActorScalingInternal(LocalToActor, InParentComponent);
		LocalScaling = NewScaling / LocalToActor;
	}
	void ETransform::SetLocalToWorldLocationInternal(const Vector3& NewLocation, const PSceneComponent* InParentComponent, const PActor* InParentActor)
	{
		// The inverse of GetLocalToWorldLocationInternal: the difference is expressed in the same
		// parent local space the getter scales, so it has to be divided by that scaling to get the
		// local offset back.
		Vector3 ParentChainLocation(Vector3::Zero());
		GetLocalToActorLocationInternal(ParentChainLocation, InParentComponent);
		Vector3 ParentChainScaling(Vector3::One());
		GetLocalToActorScalingInternal(ParentChainScaling, InParentComponent);
		Vector3 RootToWorld(Vector3::Zero());
		GetRootToWorldLocationInternal(RootToWorld, InParentActor);
		Vector3 RootToWorldScaling(Vector3::One());
		GetRootToWorldScalingInternal(RootToWorldScaling, InParentActor);
		const Vector3 Scaling = ParentChainScaling * RootToWorldScaling;
		Vector3 InvScaling(Vector3::One());
		InvScaling.x = (EMath::Abs(Scaling.x) > PE_SMALL_NUMBER) ? (1.0f / Scaling.x) : 1.0f;
		InvScaling.y = (EMath::Abs(Scaling.y) > PE_SMALL_NUMBER) ? (1.0f / Scaling.y) : 1.0f;
		InvScaling.z = (EMath::Abs(Scaling.z) > PE_SMALL_NUMBER) ? (1.0f / Scaling.z) : 1.0f;
		LocalLocation = ((NewLocation - RootToWorld) - ParentChainLocation) * InvScaling;
	}
	void ETransform::SetLocalToWorldRotationInternal(const Quaternion& NewRotation, const PSceneComponent* InParentComponent, const PActor* InParentActor)
	{
		Quaternion ActorToLocal(Quaternion::Identity());
		GetLocalToActorRotationInternal(ActorToLocal, InParentComponent);
		ActorToLocal.Inverse();
		Quaternion WorldToRoot(Quaternion::Identity());
		GetRootToWorldRotationInternal(WorldToRoot, InParentActor);
		WorldToRoot.Inverse();
		LocalRotation = ActorToLocal * (WorldToRoot * NewRotation);
	}
	void ETransform::SetLocalToWorldScalingInternal(const Vector3& NewScaling, const PSceneComponent* InParentComponent, const PActor* InParentActor)
	{
		Vector3 LocalToActor(Vector3::One());
		GetLocalToActorScalingInternal(LocalToActor, InParentComponent);
		Vector3 RootToWorld(Vector3::One());
		GetRootToWorldScalingInternal(RootToWorld, InParentActor);
		LocalScaling = (NewScaling / RootToWorld) / LocalToActor;
	}
	Vector3 ETransform::GetLocalToActorForwardVectorInternal(const PSceneComponent* InParentComponent) const
	{
		return SplitForwardVector(GetLocalToActorRotationInternal(InParentComponent));
	}
	Vector3 ETransform::GetLocalToActorUpVectorInternal(const PSceneComponent* InParentComponent) const
	{
		return SplitUpVector(GetLocalToActorRotationInternal(InParentComponent));
	}
	Vector3 ETransform::GetLocalToActorRightVectorInternal(const PSceneComponent* InParentComponent) const
	{
		return SplitRightVector(GetLocalToActorRotationInternal(InParentComponent));
	}
	Vector3 ETransform::GetLocalToWorldForwardVectorInternal(const PSceneComponent* InParentComponent, const PActor* InParentActor) const
	{
		return SplitForwardVector(GetLocalToWorldRotationInternal(InParentComponent, InParentActor));
	}
	Vector3 ETransform::GetLocalToWorldUpVectorInternal(const PSceneComponent* InParentComponent, const PActor* InParentActor) const
	{
		return SplitUpVector(GetLocalToWorldRotationInternal(InParentComponent, InParentActor));
	}
	Vector3 ETransform::GetLocalToWorldRightVectorInternal(const PSceneComponent* InParentComponent, const PActor* InParentActor) const
	{
		return SplitRightVector(GetLocalToWorldRotationInternal(InParentComponent, InParentActor));
	}
	Matrix4x4 ETransform::ToMatrix4x4WithScaling_Local() const
	{
		return (MakeMatrix4x4(LocalLocation, LocalRotation, LocalScaling));
	}
	Matrix4x4 ETransform::ToMatrix4x4WithScaling_Actor(const PSceneComponent* InParentComponent) const
	{
		return (MakeMatrix4x4(
			GetLocalToActorLocationInternal(InParentComponent),
			GetLocalToActorRotationInternal(InParentComponent),
			GetLocalToActorScalingInternal(InParentComponent)));
	}
	Matrix4x4 ETransform::ToMatrix4x4WithScaling_World(const PSceneComponent* InParentComponent, const PActor* InParentActor) const
	{
		return (MakeMatrix4x4(
			GetLocalToWorldLocationInternal(InParentComponent, InParentActor),
			GetLocalToWorldRotationInternal(InParentComponent, InParentActor),
			GetLocalToWorldScalingInternal(InParentComponent, InParentActor)));
	}
	Matrix4x4 ETransform::ToMatrix4x4NoScaling_Local() const
	{
		return (MakeMatrix4x4(LocalLocation, LocalRotation, Vector3::One()));
	}
	Matrix4x4 ETransform::ToMatrix4x4NoScaling_Actor(const PSceneComponent* InParentComponent) const
	{
		return (MakeMatrix4x4(
			GetLocalToActorLocationInternal(InParentComponent),
			GetLocalToActorRotationInternal(InParentComponent),
			Vector3::One()));
	}
	Matrix4x4 ETransform::ToMatrix4x4NoScaling_World(const PSceneComponent* InParentComponent, const PActor* InParentActor) const
	{
		return (MakeMatrix4x4(
			GetLocalToWorldLocationInternal(InParentComponent, InParentActor),
			GetLocalToWorldRotationInternal(InParentComponent, InParentActor),
			Vector3::One()));
	}
	Matrix4x4 ETransform::ToInverseMatrix4x4WithScaling_Local() const
	{
		return (MakeMatrix4x4(LocalLocation, LocalRotation, LocalScaling).Inverse());
	}
	Matrix4x4 ETransform::ToInverseMatrix4x4WithScaling_Actor(const PSceneComponent* InParentComponent) const
	{
		return (MakeMatrix4x4(
			GetLocalToActorLocationInternal(InParentComponent),
			GetLocalToActorRotationInternal(InParentComponent),
			GetLocalToActorScalingInternal(InParentComponent)).Inverse());
	}
	Matrix4x4 ETransform::ToInverseMatrix4x4WithScaling_World(const PSceneComponent* InParentComponent, const PActor* InParentActor) const
	{
		return (MakeMatrix4x4(
			GetLocalToWorldLocationInternal(InParentComponent, InParentActor),
			GetLocalToWorldRotationInternal(InParentComponent, InParentActor),
			GetLocalToWorldScalingInternal(InParentComponent, InParentActor)).Inverse());
	}
	Matrix4x4 ETransform::ToInverseMatrix4x4NoScaling_Local() const
	{
		return (MakeMatrix4x4(LocalLocation, LocalRotation, Vector3::One()).Inverse());
	}
	Matrix4x4 ETransform::ToInverseMatrix4x4NoScaling_Actor(const PSceneComponent* InParentComponent) const
	{
		return (MakeMatrix4x4(
			GetLocalToActorLocationInternal(InParentComponent),
			GetLocalToActorRotationInternal(InParentComponent),
			Vector3::One()).Inverse());
	}
	Matrix4x4 ETransform::ToInverseMatrix4x4NoScaling_World(const PSceneComponent* InParentComponent, const PActor* InParentActor) const
	{
		return (MakeMatrix4x4(
			GetLocalToWorldLocationInternal(InParentComponent, InParentActor),
			GetLocalToWorldRotationInternal(InParentComponent, InParentActor),
			Vector3::One()).Inverse());
	}
	Vector3 ETransform::GetLocation_Local() const
	{
		return LocalLocation;
	}
	Vector3 ETransform::GetLocation_Actor(const PSceneComponent* InParentComponent) const
	{
		return (GetLocalToActorLocationInternal(InParentComponent));
	}
	Vector3 ETransform::GetLocation_World(const PSceneComponent* InParentComponent, const PActor* InParentActor) const
	{
		return (GetLocalToWorldLocationInternal(InParentComponent, InParentActor));
	}
	Quaternion ETransform::GetRotation_Local() const
	{
		return LocalRotation;
	}
	Quaternion ETransform::GetRotation_Actor(const PSceneComponent* InParentComponent) const
	{
		return (GetLocalToActorRotationInternal(InParentComponent));
	}
	Quaternion ETransform::GetRotation_World(const PSceneComponent* InParentComponent, const PActor* InParentActor) const
	{
		return (GetLocalToWorldRotationInternal(InParentComponent, InParentActor));
	}
	Vector3 ETransform::GetScaling_Local() const
	{
		return LocalScaling;
	}
	Vector3 ETransform::GetScaling_Actor(const PSceneComponent* InParentComponent) const
	{
		return (GetLocalToActorScalingInternal(InParentComponent));
	}
	Vector3 ETransform::GetScaling_World(const PSceneComponent* InParentComponent, const PActor* InParentActor) const
	{
		return (GetLocalToWorldScalingInternal(InParentComponent, InParentActor));
	}
	Vector3 ETransform::GetForwardVector_Local() const
	{
		return SplitForwardVector(LocalRotation);
	}
	Vector3 ETransform::GetForwardVector_Actor(const PSceneComponent* InParentComponent) const
	{
		return (GetLocalToActorForwardVectorInternal(InParentComponent));
	}
	Vector3 ETransform::GetForwardVector_World(const PSceneComponent* InParentComponent, const PActor* InParentActor) const
	{
		return (GetLocalToWorldForwardVectorInternal(InParentComponent, InParentActor));
	}
	Vector3 ETransform::GetUpVector_Local() const
	{
		return SplitUpVector(LocalRotation);
	}
	Vector3 ETransform::GetUpVector_Actor(const PSceneComponent* InParentComponent) const
	{
		return (GetLocalToActorUpVectorInternal(InParentComponent));
	}
	Vector3 ETransform::GetUpVector_World(const PSceneComponent* InParentComponent, const PActor* InParentActor) const
	{
		return (GetLocalToWorldUpVectorInternal(InParentComponent, InParentActor));
	}
	Vector3 ETransform::GetRightVector_Local() const
	{
		return SplitRightVector(LocalRotation);
	}
	Vector3 ETransform::GetRightVector_Actor(const PSceneComponent* InParentComponent) const
	{
		return (GetLocalToActorRightVectorInternal(InParentComponent));
	}
	Vector3 ETransform::GetRightVector_World(const PSceneComponent* InParentComponent, const PActor* InParentActor) const
	{
		return (GetLocalToWorldRightVectorInternal(InParentComponent, InParentActor));
	}
	void ETransform::SetLocation_Local(const Vector3& NewValue)
	{
		LocalLocation = NewValue;
	}
	void ETransform::SetLocation_Actor(const Vector3& NewValue, const PSceneComponent* InParentComponent)
	{
		SetLocalToActorLocationInternal(NewValue, InParentComponent);
	}
	void ETransform::SetLocation_World(const Vector3& NewValue, const PSceneComponent* InParentComponent, const PActor* InParentActor)
	{
		SetLocalToWorldLocationInternal(NewValue, InParentComponent, InParentActor);
	}
	void ETransform::SetRotation_Local(const Quaternion& NewValue)
	{
		LocalRotation = NewValue;
	}
	void ETransform::SetRotation_Actor(const Quaternion& NewValue, const PSceneComponent* InParentComponent)
	{
		SetLocalToActorRotationInternal(NewValue, InParentComponent);
	}
	void ETransform::SetRotation_World(const Quaternion& NewValue, const PSceneComponent* InParentComponent, const PActor* InParentActor)
	{
		SetLocalToWorldRotationInternal(NewValue, InParentComponent, InParentActor);
	}
	void ETransform::SetScaling_Local(const Vector3& NewValue)
	{
		LocalScaling = NewValue;
	}
	void ETransform::SetScaling_Actor(const Vector3& NewValue, const PSceneComponent* InParentComponent)
	{
		SetLocalToActorScalingInternal(NewValue, InParentComponent);
	}
	void ETransform::SetScaling_World(const Vector3& NewValue, const PSceneComponent* InParentComponent, const PActor* InParentActor)
	{
		SetLocalToWorldScalingInternal(NewValue, InParentComponent, InParentActor);
	}

};
