#pragma once

#include "../../../Main/Main.h"
#include "BuiltInType.h"
#include "BaseType.h"
#include "ObjectBase.h"
#if _EDITOR_ONLY
#include <Config/ErrorCaption.h>
#include "../../../../Development/Alert/DevelopmentDefines.h"
#endif

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
	public:
		Matrix4x4	ToMatrix4x4WithScaling_Local()const;
		Matrix4x4	ToMatrix4x4WithScaling_Actor(const PSceneComponent* InParentComponent)const;
		Matrix4x4	ToMatrix4x4WithScaling_World(const PSceneComponent* InParentComponent, const PActor* InParentActor)const;
		Matrix4x4	ToMatrix4x4NoScaling_Local()const;
		Matrix4x4	ToMatrix4x4NoScaling_Actor(const PSceneComponent* InParentComponent)const;
		Matrix4x4	ToMatrix4x4NoScaling_World(const PSceneComponent* InParentComponent, const PActor* InParentActor)const;
		Matrix4x4	ToInverseMatrix4x4WithScaling_Local()const;
		Matrix4x4	ToInverseMatrix4x4WithScaling_Actor(const PSceneComponent* InParentComponent)const;
		Matrix4x4	ToInverseMatrix4x4WithScaling_World(const PSceneComponent* InParentComponent, const PActor* InParentActor)const;
		Matrix4x4	ToInverseMatrix4x4NoScaling_Local()const;
		Matrix4x4	ToInverseMatrix4x4NoScaling_Actor(const PSceneComponent* InParentComponent)const;
		Matrix4x4	ToInverseMatrix4x4NoScaling_World(const PSceneComponent* InParentComponent, const PActor* InParentActor)const;
		Vector3		GetLocation_Local()const;
		Vector3		GetLocation_Actor(const PSceneComponent* InParentComponent)const;
		Vector3		GetLocation_World(const PSceneComponent* InParentComponent, const PActor* InParentActor)const;
		Quaternion	GetRotation_Local()const;
		Quaternion	GetRotation_Actor(const PSceneComponent* InParentComponent)const;
		Quaternion	GetRotation_World(const PSceneComponent* InParentComponent, const PActor* InParentActor)const;
		Vector3		GetScaling_Local()const;
		Vector3		GetScaling_Actor(const PSceneComponent* InParentComponent)const;
		Vector3		GetScaling_World(const PSceneComponent* InParentComponent, const PActor* InParentActor)const;
		Vector3		GetForwardVector_Local()const;
		Vector3		GetForwardVector_Actor(const PSceneComponent* InParentComponent)const;
		Vector3		GetForwardVector_World(const PSceneComponent* InParentComponent, const PActor* InParentActor)const;
		Vector3		GetUpVector_Local()const;
		Vector3		GetUpVector_Actor(const PSceneComponent* InParentComponent)const;
		Vector3		GetUpVector_World(const PSceneComponent* InParentComponent, const PActor* InParentActor)const;
		Vector3		GetRightVector_Local()const;
		Vector3		GetRightVector_Actor(const PSceneComponent* InParentComponent)const;
		Vector3		GetRightVector_World(const PSceneComponent* InParentComponent, const PActor* InParentActor)const;
	public:
		void	SetLocation_Local(const Vector3& NewValue);
		void	SetLocation_Actor(const Vector3& NewValue, const PSceneComponent* InParentComponent);
		void	SetLocation_World(const Vector3& NewValue, const PSceneComponent* InParentComponent, const PActor* InParentActor);
		void	SetRotation_Local(const Quaternion& NewValue);
		void	SetRotation_Actor(const Quaternion& NewValue, const PSceneComponent* InParentComponent);
		void	SetRotation_World(const Quaternion& NewValue, const PSceneComponent* InParentComponent, const PActor* InParentActor);
		void	SetScaling_Local(const Vector3& NewValue);
		void	SetScaling_Actor(const Vector3& NewValue, const PSceneComponent* InParentComponent);
		void	SetScaling_World(const Vector3& NewValue, const PSceneComponent* InParentComponent, const PActor* InParentActor);
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