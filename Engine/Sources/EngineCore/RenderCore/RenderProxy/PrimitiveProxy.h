#pragma once

#include <CoreMinimal.h>

namespace PigeonEngine
{

	class PPrimitiveComponent;
	class RScene;

	class RPrimitiveProxy : public EObjectBase
	{
	public:
		RPrimitiveProxy(const RScene* InScene, const PPrimitiveComponent* InComponent);
	public:
		virtual BOOL32		IsRenderValid()const;
		const Vector3&		GetWorldLocation()const;
		const Quaternion&	GetWorldRotation()const;
		const Vector3&		GetWorldScaling()const;
		const Matrix4x4&	GetLocalToWorldMatrix()const;
	protected:
		Vector3			WorldLocation;
		Quaternion		WorldRotation;
		Vector3			WorldScaling;
		Matrix4x4		LocalToWorldMatrix;
	protected:
		const RScene*				Scene;
		const PPrimitiveComponent*	Component;
	public:
		RPrimitiveProxy();
		RPrimitiveProxy(const RPrimitiveProxy& Other);
		virtual ~RPrimitiveProxy();
		RPrimitiveProxy& operator=(const RPrimitiveProxy&) = delete;
	};

};