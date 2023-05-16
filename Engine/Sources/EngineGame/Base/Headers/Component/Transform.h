#pragma once

#include <Main.h>
#include "../../../../EngineCore/Base/Headers/BaseType.h"
#include "../../../../EngineCore/Base/Headers/ObjectBase.h"

namespace PigeonEngine
{

	class PActorComponent;

	enum class ECoordinateSpaceType : UINT8
	{
		ECST_LOCAL,
		ECST_WORLD
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
		Matrix4x4	ToMatrix4x4(const PActorComponent* InParentComponent = nullptr)const;
		template<ECoordinateSpaceType _CoordinateSpaceType>
		Vector3		GetLocation(const PActorComponent* InParentComponent = nullptr)const;
		template<ECoordinateSpaceType _CoordinateSpaceType>
		Quaternion	GetRotation(const PActorComponent* InParentComponent = nullptr)const;
		template<ECoordinateSpaceType _CoordinateSpaceType>
		Vector3		GetScaling(const PActorComponent* InParentComponent = nullptr)const;
		template<ECoordinateSpaceType _CoordinateSpaceType>
		Vector3		GetForwardVector(const PActorComponent* InParentComponent = nullptr)const;
		template<ECoordinateSpaceType _CoordinateSpaceType>
		Vector3		GetUpVector(const PActorComponent* InParentComponent = nullptr)const;
		template<ECoordinateSpaceType _CoordinateSpaceType>
		Vector3		GetRightVector(const PActorComponent* InParentComponent = nullptr)const;
	public:
		Matrix4x4	GetLocalToWorldMatrix(const PActorComponent* InParentComponent)const;
		Matrix4x4	GetWorldToLocalMatrix(const PActorComponent* InParentComponent)const;
	private:
		Vector3		GetForwardVectorInternal(const PActorComponent* InParentComponent)const;
		Vector3		GetUpVectorInternal(const PActorComponent* InParentComponent)const;
		Vector3		GetRightVectorInternal(const PActorComponent* InParentComponent)const;
		Vector3		GetWorldLocationInternal(const PActorComponent* InParentComponent)const;
		Quaternion	GetWorldRotationInternal(const PActorComponent* InParentComponent)const;
		Vector3		GetWorldScalingInternal(const PActorComponent* InParentComponent)const;
		void	GetWorldLocationInternal(Vector3& InOutWorldLocation, const PActorComponent* InParentComponent)const;
		void	GetWorldRotationInternal(Quaternion& InOutWorldRotation, const PActorComponent* InParentComponent)const;
		void	GetWorldScalingInternal(Vector3& InOutWorldScaling, const PActorComponent* InParentComponent)const;
	protected:
		Vector3		LocalLocation;
		Quaternion	LocalRotation;
		Vector3		LocalScaling;

		CLASS_VIRTUAL_COPY_BODY(ETransform)

	};

};