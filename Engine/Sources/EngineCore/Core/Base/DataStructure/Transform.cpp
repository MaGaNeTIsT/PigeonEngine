#include "Transform.h"
#include <PigeonBase/Object/Component/SceneComponent.h>
#include <PigeonBase/Object/Actor.h>

namespace PigeonEngine
{

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
	template<ECoordinateSpaceType _CoordinateSpaceType>
	Matrix4x4 ETransform::ToMatrix4x4WithScaling(const PSceneComponent* InParentComponent, const PActor* InParentActor)const
	{
		if (_CoordinateSpaceType == ECoordinateSpaceType::ECST_LOCAL)
		{
			return (Matrix4x4(LocalLocation, LocalRotation, LocalScaling));
		}
		else if (_CoordinateSpaceType == ECoordinateSpaceType::ECST_ACTOR)
		{
			return (Matrix4x4(
				GetLocalToActorLocationInternal(InParentComponent),
				GetLocalToActorRotationInternal(InParentComponent),
				GetLocalToActorScalingInternal(InParentComponent)));
		}
		else if (_CoordinateSpaceType == ECoordinateSpaceType::ECST_WORLD)
		{
			return (Matrix4x4(
				GetLocalToWorldLocationInternal(InParentComponent, InParentActor),
				GetLocalToWorldRotationInternal(InParentComponent, InParentActor),
				GetLocalToWorldScalingInternal(InParentComponent, InParentActor)));
		}
		else
		{
			PE_FAILED(ENGINE_RENDER_CORE_ERROR, "The function of ETransform::ToMatrix4x4WithScaling enum check coordinate space type failed.");
			return (Matrix4x4::Identity());
		}
	}
	template<ECoordinateSpaceType _CoordinateSpaceType>
	Matrix4x4 ETransform::ToMatrix4x4NoScaling(const PSceneComponent* InParentComponent, const PActor* InParentActor)const
	{
		if (_CoordinateSpaceType == ECoordinateSpaceType::ECST_LOCAL)
		{
			return (Matrix4x4(LocalLocation, LocalRotation, Vector3::One()));
		}
		else if (_CoordinateSpaceType == ECoordinateSpaceType::ECST_ACTOR)
		{
			return (Matrix4x4(
				GetLocalToActorLocationInternal(InParentComponent),
				GetLocalToActorRotationInternal(InParentComponent),
				Vector3::One()));
		}
		else if (_CoordinateSpaceType == ECoordinateSpaceType::ECST_WORLD)
		{
			return (Matrix4x4(
				GetLocalToWorldLocationInternal(InParentComponent, InParentActor),
				GetLocalToWorldRotationInternal(InParentComponent, InParentActor),
				Vector3::One()));
		}
		else
		{
			PE_FAILED(ENGINE_RENDER_CORE_ERROR, "The function of ETransform::ToMatrix4x4NoScaling enum check coordinate space type failed.");
			return (Matrix4x4::Identity());
		}
	}
	template<ECoordinateSpaceType _CoordinateSpaceType>
	Matrix4x4 ETransform::ToInverseMatrix4x4WithScaling(const PSceneComponent* InParentComponent, const PActor* InParentActor)const
	{
		if (_CoordinateSpaceType == ECoordinateSpaceType::ECST_LOCAL)
		{
			return (Matrix4x4(LocalLocation, LocalRotation, LocalScaling).Inverse());
		}
		else if (_CoordinateSpaceType == ECoordinateSpaceType::ECST_ACTOR)
		{
			return (Matrix4x4(
				GetLocalToActorLocationInternal(InParentComponent),
				GetLocalToActorRotationInternal(InParentComponent),
				GetLocalToActorScalingInternal(InParentComponent)).Inverse());
		}
		else if (_CoordinateSpaceType == ECoordinateSpaceType::ECST_WORLD)
		{
			return (Matrix4x4(
				GetLocalToWorldLocationInternal(InParentComponent, InParentActor),
				GetLocalToWorldRotationInternal(InParentComponent, InParentActor),
				GetLocalToWorldScalingInternal(InParentComponent, InParentActor)).Inverse());
		}
		else
		{
			PE_FAILED(ENGINE_RENDER_CORE_ERROR, "The function of ETransform::ToInverseMatrix4x4WithScaling enum check coordinate space type failed.");
			return (Matrix4x4::Identity());
		}
	}
	template<ECoordinateSpaceType _CoordinateSpaceType>
	Matrix4x4 ETransform::ToInverseMatrix4x4NoScaling(const PSceneComponent* InParentComponent, const PActor* InParentActor)const
	{
		if (_CoordinateSpaceType == ECoordinateSpaceType::ECST_LOCAL)
		{
			return (Matrix4x4(LocalLocation, LocalRotation, Vector3::One()).Inverse());
		}
		else if (_CoordinateSpaceType == ECoordinateSpaceType::ECST_ACTOR)
		{
			return (Matrix4x4(
				GetLocalToActorLocationInternal(InParentComponent),
				GetLocalToActorRotationInternal(InParentComponent),
				Vector3::One()).Inverse());
		}
		else if (_CoordinateSpaceType == ECoordinateSpaceType::ECST_WORLD)
		{
			return (Matrix4x4(
				GetLocalToWorldLocationInternal(InParentComponent, InParentActor),
				GetLocalToWorldRotationInternal(InParentComponent, InParentActor),
				Vector3::One()).Inverse());
		}
		else
		{
			PE_FAILED(ENGINE_RENDER_CORE_ERROR, "The function of ETransform::ToInverseMatrix4x4NoScaling enum check coordinate space type failed.");
			return (Matrix4x4::Identity());
		}
	}
	template<ECoordinateSpaceType _CoordinateSpaceType>
	Vector3 ETransform::GetLocation(const PSceneComponent* InParentComponent, const PActor* InParentActor)const
	{
		if (_CoordinateSpaceType == ECoordinateSpaceType::ECST_LOCAL)
		{
			return LocalLocation;
		}
		else if (_CoordinateSpaceType == ECoordinateSpaceType::ECST_ACTOR)
		{
			return (GetLocalToActorLocationInternal(InParentComponent));
		}
		else if (_CoordinateSpaceType == ECoordinateSpaceType::ECST_WORLD)
		{
			return (GetLocalToWorldLocationInternal(InParentComponent, InParentActor));
		}
		else
		{
			PE_FAILED(ENGINE_RENDER_CORE_ERROR, "The function of ETransform::GetLocation enum check coordinate space type failed.");
			return (Vector3::Zero());
		}
	}
	template<ECoordinateSpaceType _CoordinateSpaceType>
	Quaternion ETransform::GetRotation(const PSceneComponent* InParentComponent, const PActor* InParentActor)const
	{
		if (_CoordinateSpaceType == ECoordinateSpaceType::ECST_LOCAL)
		{
			return LocalRotation;
		}
		else if (_CoordinateSpaceType == ECoordinateSpaceType::ECST_ACTOR)
		{
			return (GetLocalToActorRotationInternal(InParentComponent));
		}
		else if (_CoordinateSpaceType == ECoordinateSpaceType::ECST_WORLD)
		{
			return (GetLocalToWorldRotationInternal(InParentComponent, InParentActor));
		}
		else
		{
			PE_FAILED(ENGINE_RENDER_CORE_ERROR, "The function of ETransform::GetRotation enum check coordinate space type failed.");
			return (Quaternion::Identity());
		}
	}
	template<ECoordinateSpaceType _CoordinateSpaceType>
	Vector3 ETransform::GetScaling(const PSceneComponent* InParentComponent, const PActor* InParentActor)const
	{
		if (_CoordinateSpaceType == ECoordinateSpaceType::ECST_LOCAL)
		{
			return LocalScaling;
		}
		else if (_CoordinateSpaceType == ECoordinateSpaceType::ECST_ACTOR)
		{
			return (GetLocalToActorScalingInternal(InParentComponent));
		}
		else if (_CoordinateSpaceType == ECoordinateSpaceType::ECST_WORLD)
		{
			return (GetLocalToWorldScalingInternal(InParentComponent, InParentActor));
		}
		else
		{
			PE_FAILED(ENGINE_RENDER_CORE_ERROR, "The function of ETransform::GetScaling enum check coordinate space type failed.");
			return (Vector3::One());
		}
	}
	template<ECoordinateSpaceType _CoordinateSpaceType>
	Vector3 ETransform::GetForwardVector(const PSceneComponent* InParentComponent, const PActor* InParentActor)const
	{
		if (_CoordinateSpaceType == ECoordinateSpaceType::ECST_LOCAL)
		{
			return (LocalRotation.MultiplyVector(Vector3::ZVector()));
		}
		else if (_CoordinateSpaceType == ECoordinateSpaceType::ECST_ACTOR)
		{
			return (GetLocalToActorForwardVectorInternal(InParentComponent));
		}
		else if (_CoordinateSpaceType == ECoordinateSpaceType::ECST_WORLD)
		{
			return (GetLocalToWorldForwardVectorInternal(InParentComponent, InParentActor));
		}
		else
		{
			PE_FAILED(ENGINE_RENDER_CORE_ERROR, "The function of ETransform::GetForwardVector enum check coordinate space type failed.");
			return (Vector3::ZVector());
		}
	}
	template<ECoordinateSpaceType _CoordinateSpaceType>
	Vector3 ETransform::GetUpVector(const PSceneComponent* InParentComponent, const PActor* InParentActor)const
	{
		if (_CoordinateSpaceType == ECoordinateSpaceType::ECST_LOCAL)
		{
			return (LocalRotation.MultiplyVector(Vector3::YVector()));
		}
		else if (_CoordinateSpaceType == ECoordinateSpaceType::ECST_ACTOR)
		{
			return (GetLocalToActorUpVectorInternal(InParentComponent));
		}
		else if (_CoordinateSpaceType == ECoordinateSpaceType::ECST_WORLD)
		{
			return (GetLocalToWorldUpVectorInternal(InParentComponent, InParentActor));
		}
		else
		{
			PE_FAILED(ENGINE_RENDER_CORE_ERROR, "The function of ETransform::GetUpVector enum check coordinate space type failed.");
			return (Vector3::YVector());
		}
	}
	template<ECoordinateSpaceType _CoordinateSpaceType>
	Vector3 ETransform::GetRightVector(const PSceneComponent* InParentComponent, const PActor* InParentActor)const
	{
		if (_CoordinateSpaceType == ECoordinateSpaceType::ECST_LOCAL)
		{
			return (LocalRotation.MultiplyVector(Vector3::XVector()));
		}
		else if (_CoordinateSpaceType == ECoordinateSpaceType::ECST_ACTOR)
		{
			return (GetLocalToActorRightVectorInternal(InParentComponent));
		}
		else if (_CoordinateSpaceType == ECoordinateSpaceType::ECST_WORLD)
		{
			return (GetLocalToWorldRightVectorInternal(InParentComponent, InParentActor));
		}
		else
		{
			PE_FAILED(ENGINE_RENDER_CORE_ERROR, "The function of ETransform::GetRightVector enum check coordinate space type failed.");
			return (Vector3::XVector());
		}
	}
	template<ECoordinateSpaceType _CoordinateSpaceType>
	void ETransform::SetLocation(const Vector3& NewValue, const PSceneComponent* InParentComponent)const
	{
		if (_CoordinateSpaceType == ECoordinateSpaceType::ECST_LOCAL)
		{
			LocalLocation = NewValue;
		}
		else if (_CoordinateSpaceType == ECoordinateSpaceType::ECST_WORLD)
		{
			SetWorldLocationInternal(NewValue, InParentComponent);
		}
		else
		{
			PE_FAILED(ENGINE_RENDER_CORE_ERROR, "The function of ETransform::SetLocation enum check coordinate space type failed.");
		}
	}
	template<ECoordinateSpaceType _CoordinateSpaceType>
	void ETransform::SetRotation(const Quaternion& NewValue, const PSceneComponent* InParentComponent)const
	{
		if (_CoordinateSpaceType == ECoordinateSpaceType::ECST_LOCAL)
		{
			LocalRotation = NewValue;
		}
		else if (_CoordinateSpaceType == ECoordinateSpaceType::ECST_WORLD)
		{
			SetWorldRotationInternal(NewValue, InParentComponent);
		}
		else
		{
			PE_FAILED(ENGINE_RENDER_CORE_ERROR, "The function of ETransform::SetRotation enum check coordinate space type failed.");
		}
	}
	template<ECoordinateSpaceType _CoordinateSpaceType>
	void ETransform::SetScaling(const Vector3& NewValue, const PSceneComponent* InParentComponent)const
	{
		if (_CoordinateSpaceType == ECoordinateSpaceType::ECST_LOCAL)
		{
			LocalScaling = NewValue;
		}
		else if (_CoordinateSpaceType == ECoordinateSpaceType::ECST_WORLD)
		{
			SetWorldScalingInternal(NewValue, InParentComponent);
		}
		else
		{
			PE_FAILED(ENGINE_RENDER_CORE_ERROR, "The function of ETransform::SetScaling enum check coordinate space type failed.");
		}
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
		return (GetLocalToActorRotationInternal(InParentComponent).MultiplyVector(Vector3::ZVector()));
	}
	Vector3 ETransform::GetLocalToActorUpVectorInternal(const PSceneComponent* InParentComponent)const
	{
		return (GetLocalToActorRotationInternal(InParentComponent).MultiplyVector(Vector3::YVector()));
	}
	Vector3 ETransform::GetLocalToActorRightVectorInternal(const PSceneComponent* InParentComponent)const
	{
		return (GetLocalToActorRotationInternal(InParentComponent).MultiplyVector(Vector3::XVector()));
	}
	Vector3 ETransform::GetLocalToWorldForwardVectorInternal(const PSceneComponent* InParentComponent, const PActor* InParentActor)const
	{
		return (GetLocalToWorldRotationInternal(InParentComponent, InParentActor).MultiplyVector(Vector3::ZVector()));
	}
	Vector3 ETransform::GetLocalToWorldUpVectorInternal(const PSceneComponent* InParentComponent, const PActor* InParentActor)const
	{
		return (GetLocalToWorldRotationInternal(InParentComponent, InParentActor).MultiplyVector(Vector3::YVector()));
	}
	Vector3 ETransform::GetLocalToWorldRightVectorInternal(const PSceneComponent* InParentComponent, const PActor* InParentActor)const
	{
		return (GetLocalToWorldRotationInternal(InParentComponent, InParentActor).MultiplyVector(Vector3::XVector()));
	}

};