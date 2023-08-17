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
	Matrix4x4 ETransform::GetLocalToWorldMatrix(const PSceneComponent* InParentComponent, const PActor* InParentActor)const
	{
		return (MakeMatrix4x4(
			GetLocalToWorldLocationInternal(InParentComponent, InParentActor),
			GetLocalToWorldRotationInternal(InParentComponent, InParentActor),
			GetLocalToWorldScalingInternal(InParentComponent, InParentActor)));
	}
	Matrix4x4 ETransform::GetWorldToLocalMatrix(const PSceneComponent* InParentComponent, const PActor* InParentActor)const
	{
		Matrix4x4 Result(MakeMatrix4x4(
			GetLocalToWorldLocationInternal(InParentComponent, InParentActor),
			GetLocalToWorldRotationInternal(InParentComponent, InParentActor),
			GetLocalToWorldScalingInternal(InParentComponent, InParentActor)));
		return (Result.Inverse());
	}
	Vector3 ETransform::GetLocalToActorLocationInternal(const PSceneComponent* InParentComponent)const
	{
		Vector3 Result(LocalLocation);
		GetLocalToActorLocationInternal(Result, InParentComponent);
		return Result;
	}
	Quaternion ETransform::GetLocalToActorRotationInternal(const PSceneComponent* InParentComponent)const
	{
		Quaternion Result(LocalRotation);
		GetLocalToActorRotationInternal(Result, InParentComponent);
		return Result;
	}
	Vector3 ETransform::GetLocalToActorScalingInternal(const PSceneComponent* InParentComponent)const
	{
		Vector3 Result(LocalScaling);
		GetLocalToActorScalingInternal(Result, InParentComponent);
		return Result;
	}
	Vector3 ETransform::GetLocalToWorldLocationInternal(const PSceneComponent* InParentComponent, const PActor* InParentActor)const
	{
		Vector3 LocalToActor(GetLocalToActorLocationInternal(InParentComponent));
		Vector3 RootToWorld(Vector3::Zero());
		GetRootToWorldLocationInternal(RootToWorld, InParentActor);
		return (RootToWorld + LocalToActor);
	}
	Quaternion ETransform::GetLocalToWorldRotationInternal(const PSceneComponent* InParentComponent, const PActor* InParentActor)const
	{
		Quaternion LocalToActor(GetLocalToActorRotationInternal(InParentComponent));
		Quaternion RootToWorld(Quaternion::Identity());
		GetRootToWorldRotationInternal(RootToWorld, InParentActor);
		return (RootToWorld * LocalToActor);
	}
	Vector3 ETransform::GetLocalToWorldScalingInternal(const PSceneComponent* InParentComponent, const PActor* InParentActor)const
	{
		Vector3 LocalToActor(GetLocalToActorScalingInternal(InParentComponent));
		Vector3 RootToWorld(Vector3::One());
		GetRootToWorldScalingInternal(RootToWorld, InParentActor);
		return (RootToWorld * LocalToActor);
	}
	void ETransform::GetLocalToActorLocationInternal(Vector3& InOutLocation, const PSceneComponent* InParentComponent)const
	{
		if (!InParentComponent)
		{
			return;
		}
		if (const ETransform* ParentTransform = InParentComponent->GetTransform(); ParentTransform)
		{
			InOutLocation = ParentTransform->LocalLocation + InOutLocation;
		}
		GetLocalToActorLocationInternal(InOutLocation, InParentComponent->GetAttachedParentComponent());
	}
	void ETransform::GetLocalToActorRotationInternal(Quaternion& InOutRotation, const PSceneComponent* InParentComponent)const
	{
		if (!InParentComponent)
		{
			return;
		}
		if (const ETransform* ParentTransform = InParentComponent->GetTransform(); ParentTransform)
		{
			InOutRotation = ParentTransform->LocalRotation * InOutRotation;
		}
		GetLocalToActorRotationInternal(InOutRotation, InParentComponent->GetAttachedParentComponent());
	}
	void ETransform::GetLocalToActorScalingInternal(Vector3& InOutScaling, const PSceneComponent* InParentComponent)const
	{
		if (!InParentComponent)
		{
			return;
		}
		if (const ETransform* ParentTransform = InParentComponent->GetTransform(); ParentTransform)
		{
			InOutScaling = ParentTransform->LocalScaling * InOutScaling;
		}
		GetLocalToActorScalingInternal(InOutScaling, InParentComponent->GetAttachedParentComponent());
	}
	void ETransform::GetRootToWorldLocationInternal(Vector3& InOutLocation, const PActor* InParentActor)const
	{
		if (!InParentActor)
		{
			return;
		}
		if (const PSceneComponent* ParentRootComponent = InParentActor->GetRootComponent(); ParentRootComponent)
		{
			if (const ETransform* TempTransform = ParentRootComponent->GetTransform(); TempTransform)
			{
				InOutLocation = TempTransform->LocalLocation + InOutLocation;
			}
		}
		GetRootToWorldLocationInternal(InOutLocation, InParentActor->GetAttachedParentActor());
	}
	void ETransform::GetRootToWorldRotationInternal(Quaternion& InOutRotation, const PActor* InParentActor)const
	{
		if (!InParentActor)
		{
			return;
		}
		if (const PSceneComponent* ParentRootComponent = InParentActor->GetRootComponent(); ParentRootComponent)
		{
			if (const ETransform* TempTransform = ParentRootComponent->GetTransform(); TempTransform)
			{
				InOutRotation = TempTransform->LocalRotation * InOutRotation;
			}
		}
		GetRootToWorldRotationInternal(InOutRotation, InParentActor->GetAttachedParentActor());
	}
	void ETransform::GetRootToWorldScalingInternal(Vector3& InOutScaling, const PActor* InParentActor)const
	{
		if (!InParentActor)
		{
			return;
		}
		if (const PSceneComponent* ParentRootComponent = InParentActor->GetRootComponent(); ParentRootComponent)
		{
			if (const ETransform* TempTransform = ParentRootComponent->GetTransform(); TempTransform)
			{
				InOutScaling = TempTransform->LocalScaling * InOutScaling;
			}
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
		Vector3 LocalToActor(Vector3::Zero());
		GetLocalToActorLocationInternal(LocalToActor, InParentComponent);
		Vector3 RootToWorld(Vector3::Zero());
		GetRootToWorldLocationInternal(RootToWorld, InParentActor);
		LocalLocation = (NewLocation - RootToWorld) - LocalToActor;
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
	Vector3 ETransform::GetLocalToActorForwardVectorInternal(const PSceneComponent* InParentComponent)const
	{
		return QuaternionTransformVector(GetLocalToActorRotationInternal(InParentComponent), Vector3::ZVector());
	}
	Vector3 ETransform::GetLocalToActorUpVectorInternal(const PSceneComponent* InParentComponent)const
	{
		return QuaternionTransformVector(GetLocalToActorRotationInternal(InParentComponent), Vector3::YVector());
	}
	Vector3 ETransform::GetLocalToActorRightVectorInternal(const PSceneComponent* InParentComponent)const
	{
		return QuaternionTransformVector(GetLocalToActorRotationInternal(InParentComponent), Vector3::XVector());
	}
	Vector3 ETransform::GetLocalToWorldForwardVectorInternal(const PSceneComponent* InParentComponent, const PActor* InParentActor)const
	{
		return QuaternionTransformVector(GetLocalToWorldRotationInternal(InParentComponent, InParentActor), Vector3::ZVector());
	}
	Vector3 ETransform::GetLocalToWorldUpVectorInternal(const PSceneComponent* InParentComponent, const PActor* InParentActor)const
	{
		return QuaternionTransformVector(GetLocalToWorldRotationInternal(InParentComponent, InParentActor), Vector3::YVector());
	}
	Vector3 ETransform::GetLocalToWorldRightVectorInternal(const PSceneComponent* InParentComponent, const PActor* InParentActor)const
	{
		return QuaternionTransformVector(GetLocalToWorldRotationInternal(InParentComponent, InParentActor), Vector3::XVector());
	}
	Matrix4x4 ETransform::ToMatrix4x4WithScaling_Local()const
	{
		return (MakeMatrix4x4(LocalLocation, LocalRotation, LocalScaling));
	}
	Matrix4x4 ETransform::ToMatrix4x4WithScaling_Actor(const PSceneComponent* InParentComponent)const
	{
		return (MakeMatrix4x4(
			GetLocalToActorLocationInternal(InParentComponent),
			GetLocalToActorRotationInternal(InParentComponent),
			GetLocalToActorScalingInternal(InParentComponent)));
	}
	Matrix4x4 ETransform::ToMatrix4x4WithScaling_World(const PSceneComponent* InParentComponent, const PActor* InParentActor)const
	{
		return (MakeMatrix4x4(
			GetLocalToWorldLocationInternal(InParentComponent, InParentActor),
			GetLocalToWorldRotationInternal(InParentComponent, InParentActor),
			GetLocalToWorldScalingInternal(InParentComponent, InParentActor)));
	}
	Matrix4x4 ETransform::ToMatrix4x4NoScaling_Local()const
	{
		return (MakeMatrix4x4(LocalLocation, LocalRotation, Vector3::One()));
	}
	Matrix4x4 ETransform::ToMatrix4x4NoScaling_Actor(const PSceneComponent* InParentComponent)const
	{
		return (MakeMatrix4x4(
			GetLocalToActorLocationInternal(InParentComponent),
			GetLocalToActorRotationInternal(InParentComponent),
			Vector3::One()));
	}
	Matrix4x4 ETransform::ToMatrix4x4NoScaling_World(const PSceneComponent* InParentComponent, const PActor* InParentActor)const
	{
		return (MakeMatrix4x4(
			GetLocalToWorldLocationInternal(InParentComponent, InParentActor),
			GetLocalToWorldRotationInternal(InParentComponent, InParentActor),
			Vector3::One()));
	}
	Matrix4x4 ETransform::ToInverseMatrix4x4WithScaling_Local()const
	{
		return (MakeMatrix4x4(LocalLocation, LocalRotation, LocalScaling).Inverse());
	}
	Matrix4x4 ETransform::ToInverseMatrix4x4WithScaling_Actor(const PSceneComponent* InParentComponent)const
	{
		return (MakeMatrix4x4(
			GetLocalToActorLocationInternal(InParentComponent),
			GetLocalToActorRotationInternal(InParentComponent),
			GetLocalToActorScalingInternal(InParentComponent)).Inverse());
	}
	Matrix4x4 ETransform::ToInverseMatrix4x4WithScaling_World(const PSceneComponent* InParentComponent, const PActor* InParentActor)const
	{
		return (MakeMatrix4x4(
			GetLocalToWorldLocationInternal(InParentComponent, InParentActor),
			GetLocalToWorldRotationInternal(InParentComponent, InParentActor),
			GetLocalToWorldScalingInternal(InParentComponent, InParentActor)).Inverse());
	}
	Matrix4x4 ETransform::ToInverseMatrix4x4NoScaling_Local()const
	{
		return (MakeMatrix4x4(LocalLocation, LocalRotation, Vector3::One()).Inverse());
	}
	Matrix4x4 ETransform::ToInverseMatrix4x4NoScaling_Actor(const PSceneComponent* InParentComponent)const
	{
		return (MakeMatrix4x4(
			GetLocalToActorLocationInternal(InParentComponent),
			GetLocalToActorRotationInternal(InParentComponent),
			Vector3::One()).Inverse());
	}
	Matrix4x4 ETransform::ToInverseMatrix4x4NoScaling_World(const PSceneComponent* InParentComponent, const PActor* InParentActor)const
	{
		return (MakeMatrix4x4(
			GetLocalToWorldLocationInternal(InParentComponent, InParentActor),
			GetLocalToWorldRotationInternal(InParentComponent, InParentActor),
			Vector3::One()).Inverse());
	}
	Vector3 ETransform::GetLocation_Local()const
	{
		return LocalLocation;
	}
	Vector3 ETransform::GetLocation_Actor(const PSceneComponent* InParentComponent)const
	{
		return (GetLocalToActorLocationInternal(InParentComponent));
	}
	Vector3 ETransform::GetLocation_World(const PSceneComponent* InParentComponent, const PActor* InParentActor)const
	{
		return (GetLocalToWorldLocationInternal(InParentComponent, InParentActor));
	}
	Quaternion ETransform::GetRotation_Local()const
	{
		return LocalRotation;
	}
	Quaternion ETransform::GetRotation_Actor(const PSceneComponent* InParentComponent)const
	{
		return (GetLocalToActorRotationInternal(InParentComponent));
	}
	Quaternion ETransform::GetRotation_World(const PSceneComponent* InParentComponent, const PActor* InParentActor)const
	{
		return (GetLocalToWorldRotationInternal(InParentComponent, InParentActor));
	}
	Vector3 ETransform::GetScaling_Local()const
	{
		return LocalScaling;
	}
	Vector3 ETransform::GetScaling_Actor(const PSceneComponent* InParentComponent)const
	{
		return (GetLocalToActorScalingInternal(InParentComponent));
	}
	Vector3 ETransform::GetScaling_World(const PSceneComponent* InParentComponent, const PActor* InParentActor)const
	{
		return (GetLocalToWorldScalingInternal(InParentComponent, InParentActor));
	}
	Vector3 ETransform::GetForwardVector_Local()const
	{
		return QuaternionTransformVector(LocalRotation, Vector3::ZVector());
	}
	Vector3 ETransform::GetForwardVector_Actor(const PSceneComponent* InParentComponent)const
	{
		return (GetLocalToActorForwardVectorInternal(InParentComponent));
	}
	Vector3 ETransform::GetForwardVector_World(const PSceneComponent* InParentComponent, const PActor* InParentActor)const
	{
		return (GetLocalToWorldForwardVectorInternal(InParentComponent, InParentActor));
	}
	Vector3 ETransform::GetUpVector_Local()const
	{
		return QuaternionTransformVector(LocalRotation, Vector3::YVector());
	}
	Vector3 ETransform::GetUpVector_Actor(const PSceneComponent* InParentComponent)const
	{
		return (GetLocalToActorUpVectorInternal(InParentComponent));
	}
	Vector3 ETransform::GetUpVector_World(const PSceneComponent* InParentComponent, const PActor* InParentActor)const
	{
		return (GetLocalToWorldUpVectorInternal(InParentComponent, InParentActor));
	}
	Vector3 ETransform::GetRightVector_Local()const
	{
		return QuaternionTransformVector(LocalRotation, Vector3::XVector());
	}
	Vector3 ETransform::GetRightVector_Actor(const PSceneComponent* InParentComponent)const
	{
		return (GetLocalToActorRightVectorInternal(InParentComponent));
	}
	Vector3 ETransform::GetRightVector_World(const PSceneComponent* InParentComponent, const PActor* InParentActor)const
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