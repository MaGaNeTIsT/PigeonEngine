#pragma once

#include "BuiltInType.h"
#include "BaseType.h"
#include "ObjectBase.h"

namespace PigeonEngine
{
	
	class PSceneComponent;
	class PActor;
	enum class ECoordinateSpaceType : UINT8
	{
		ECST_LOCAL,
		ECST_ACTOR,
		ECST_WORLD,
	};

	enum class EMobilityType : UINT8
	{
		EMT_STATIC,
		EMT_DYNAMIC,
		EMT_STATION
	};

	class ETransform : public EObjectBase
	{

		EClass(ETransform, EObjectBase)

	public:
		template<ECoordinateSpaceType _CoordinateSpaceType>
		Matrix4x4	ToMatrix4x4WithScaling(const PSceneComponent* InParentComponent = nullptr, const PActor* InParentActor = nullptr)const;
		template<ECoordinateSpaceType _CoordinateSpaceType>
		Matrix4x4	ToMatrix4x4NoScaling(const PSceneComponent* InParentComponent = nullptr, const PActor* InParentActor = nullptr)const;
		template<ECoordinateSpaceType _CoordinateSpaceType>
		Matrix4x4	ToInverseMatrix4x4WithScaling(const PSceneComponent* InParentComponent = nullptr, const PActor* InParentActor = nullptr)const;
		template<ECoordinateSpaceType _CoordinateSpaceType>
		Matrix4x4	ToInverseMatrix4x4NoScaling(const PSceneComponent* InParentComponent = nullptr, const PActor* InParentActor = nullptr)const;
		template<ECoordinateSpaceType _CoordinateSpaceType>
		Vector3		GetLocation(const PSceneComponent* InParentComponent = nullptr, const PActor* InParentActor = nullptr)const;
		template<ECoordinateSpaceType _CoordinateSpaceType>
		Quaternion	GetRotation(const PSceneComponent* InParentComponent = nullptr, const PActor* InParentActor = nullptr)const;
		template<ECoordinateSpaceType _CoordinateSpaceType>
		Vector3		GetScaling(const PSceneComponent* InParentComponent = nullptr, const PActor* InParentActor = nullptr)const;
		template<ECoordinateSpaceType _CoordinateSpaceType = ECoordinateSpaceType::ECST_WORLD>
		Vector3		GetForwardVector(const PSceneComponent* InParentComponent = nullptr, const PActor* InParentActor = nullptr)const;
		template<ECoordinateSpaceType _CoordinateSpaceType = ECoordinateSpaceType::ECST_WORLD>
		Vector3		GetUpVector(const PSceneComponent* InParentComponent = nullptr, const PActor* InParentActor = nullptr)const;
		template<ECoordinateSpaceType _CoordinateSpaceType = ECoordinateSpaceType::ECST_WORLD>
		Vector3		GetRightVector(const PSceneComponent* InParentComponent = nullptr, const PActor* InParentActor = nullptr)const;
	public:
		template<ECoordinateSpaceType _CoordinateSpaceType>
		void	SetLocation(const Vector3& NewValue, const PSceneComponent* InParentComponent = nullptr)const;
		template<ECoordinateSpaceType _CoordinateSpaceType>
		void	SetRotation(const Quaternion& NewValue, const PSceneComponent* InParentComponent = nullptr)const;
		template<ECoordinateSpaceType _CoordinateSpaceType>
		void	SetScaling(const Vector3& NewValue, const PSceneComponent* InParentComponent = nullptr)const;
	public:
		Matrix4x4	GetLocalToWorldMatrix(const PSceneComponent* InParentComponent, const PActor* InParentActor)const;
		Matrix4x4	GetWorldToLocalMatrix(const PSceneComponent* InParentComponent, const PActor* InParentActor)const;
	private:
		Vector3		GetLocalToActorLocationInternal(const PSceneComponent* InParentComponent)const;
		Quaternion	GetLocalToActorRotationInternal(const PSceneComponent* InParentComponent)const;
		Vector3		GetLocalToActorScalingInternal(const PSceneComponent* InParentComponent)const;
		Vector3		GetLocalToWorldLocationInternal(const PSceneComponent* InParentComponent, const PActor* InParentActor)const;
		Quaternion	GetLocalToWorldRotationInternal(const PSceneComponent* InParentComponent, const PActor* InParentActor)const;
		Vector3		GetLocalToWorldScalingInternal(const PSceneComponent* InParentComponent, const PActor* InParentActor)const;
	private:
		void		GetLocalToActorLocationInternal(Vector3& InOutLocation, const PSceneComponent* InParentComponent)const;
		void		GetLocalToActorRotationInternal(Quaternion& InOutRotation, const PSceneComponent* InParentComponent)const;
		void		GetLocalToActorScalingInternal(Vector3& InOutScaling, const PSceneComponent* InParentComponent)const;
		void		GetRootToWorldLocationInternal(Vector3& InOutLocation, const PActor* InParentActor)const;
		void		GetRootToWorldRotationInternal(Quaternion& InOutRotation, const PActor* InParentActor)const;
		void		GetRootToWorldScalingInternal(Vector3& InOutScaling, const PActor* InParentActor)const;
	private:
		void		SetLocalToActorLocationInternal(const Vector3& NewLocation, const PSceneComponent* InParentComponent);
		void		SetLocalToActorRotationInternal(const Quaternion& NewRotation, const PSceneComponent* InParentComponent);
		void		SetLocalToActorScalingInternal(const Vector3& NewScaling, const PSceneComponent* InParentComponent);
		void		SetLocalToWorldLocationInternal(const Vector3& NewLocation, const PSceneComponent* InParentComponent, const PActor* InParentActor);
		void		SetLocalToWorldRotationInternal(const Quaternion& NewRotation, const PSceneComponent* InParentComponent, const PActor* InParentActor);
		void		SetLocalToWorldScalingInternal(const Vector3& NewScaling, const PSceneComponent* InParentComponent, const PActor* InParentActor);
	private:
		Vector3		GetLocalToActorForwardVectorInternal(const PSceneComponent* InParentComponent)const;
		Vector3		GetLocalToActorUpVectorInternal(const PSceneComponent* InParentComponent)const;
		Vector3		GetLocalToActorRightVectorInternal(const PSceneComponent* InParentComponent)const;
		Vector3		GetLocalToWorldForwardVectorInternal(const PSceneComponent* InParentComponent, const PActor* InParentActor)const;
		Vector3		GetLocalToWorldUpVectorInternal(const PSceneComponent* InParentComponent, const PActor* InParentActor)const;
		Vector3		GetLocalToWorldRightVectorInternal(const PSceneComponent* InParentComponent, const PActor* InParentActor)const;
	protected:
		Vector3		LocalLocation;
		Quaternion	LocalRotation;
		Vector3		LocalScaling;

		CLASS_VIRTUAL_COPY_BODY(ETransform)

	};

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

};