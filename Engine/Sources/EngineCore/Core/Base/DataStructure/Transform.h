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

};