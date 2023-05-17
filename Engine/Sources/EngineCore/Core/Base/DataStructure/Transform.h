#pragma once

#include <CoreMinimal.h>


namespace PigeonEngine
{
	
	class PSceneComponent;
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
		Matrix4x4	ToMatrix4x4(const PSceneComponent* InParentComponent = nullptr)const;
		template<ECoordinateSpaceType _CoordinateSpaceType>
		Vector3		GetLocation(const PSceneComponent* InParentComponent = nullptr)const;
		template<ECoordinateSpaceType _CoordinateSpaceType>
		Quaternion	GetRotation(const PSceneComponent* InParentComponent = nullptr)const;
		template<ECoordinateSpaceType _CoordinateSpaceType>
		Vector3		GetScaling(const PSceneComponent* InParentComponent = nullptr)const;
		template<ECoordinateSpaceType _CoordinateSpaceType = ECoordinateSpaceType::ECST_WORLD>
		Vector3		GetForwardVector(const PSceneComponent* InParentComponent = nullptr)const;
		template<ECoordinateSpaceType _CoordinateSpaceType = ECoordinateSpaceType::ECST_WORLD>
		Vector3		GetUpVector(const PSceneComponent* InParentComponent = nullptr)const;
		template<ECoordinateSpaceType _CoordinateSpaceType = ECoordinateSpaceType::ECST_WORLD>
		Vector3		GetRightVector(const PSceneComponent* InParentComponent = nullptr)const;
	public:
		Matrix4x4	GetLocalToWorldMatrix(const PSceneComponent* InParentComponent)const;
		Matrix4x4	GetWorldToLocalMatrix(const PSceneComponent* InParentComponent)const;
	private:
		Vector3		GetForwardVectorInternal(const PSceneComponent* InParentComponent)const;
		Vector3		GetUpVectorInternal(const PSceneComponent* InParentComponent)const;
		Vector3		GetRightVectorInternal(const PSceneComponent* InParentComponent)const;
		Vector3		GetWorldLocationInternal(const PSceneComponent* InParentComponent)const;
		Quaternion	GetWorldRotationInternal(const PSceneComponent* InParentComponent)const;
		Vector3		GetWorldScalingInternal(const PSceneComponent* InParentComponent)const;
		void	GetWorldLocationInternal(Vector3& InOutWorldLocation, const PSceneComponent* InParentComponent)const;
		void	GetWorldRotationInternal(Quaternion& InOutWorldRotation, const PSceneComponent* InParentComponent)const;
		void	GetWorldScalingInternal(Vector3& InOutWorldScaling, const PSceneComponent* InParentComponent)const;
	protected:
		Vector3		LocalLocation;
		Quaternion	LocalRotation;
		Vector3		LocalScaling;

		CLASS_VIRTUAL_COPY_BODY(ETransform)

	};

};