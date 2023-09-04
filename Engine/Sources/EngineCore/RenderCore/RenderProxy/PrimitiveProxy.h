#pragma once

#include <CoreMinimal.h>

namespace PigeonEngine
{

	class RPrimitiveProxy : public EObjectBase
	{
	public:
		BOOL32				IsRenderValid()const;
		const Vector3&		GetWorldLocation()const;
		const Quaternion&	GetWorldRotation()const;
		const Vector3&		GetWorldScaling()const;
		const Matrix4x4&	GetLocalToWorldMatrix()const;
	protected:
		Vector3		WorldLocation;
		Quaternion	WorldRotation;
		Vector3		WorldScaling;
		Matrix4x4	LocalToWorldMatrix;
	};

};