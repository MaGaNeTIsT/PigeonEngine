#pragma once

#include <CoreMinimal.h>
#include "BaseSceneProxy.h"

namespace PigeonEngine
{

	class PPrimitiveComponent;
	class RScene;

	class RPrimitiveSceneProxy : public RBaseSceneProxy
	{
	public:
		RPrimitiveSceneProxy(const PPrimitiveComponent* InComponent);
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
		const PPrimitiveComponent*	Component;

		RENDER_PROXY_CLASS_BODY(RPrimitiveSceneProxy)

	};

};