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
		virtual BOOL32		IsRenderValid()const;
		const Vector3&		GetWorldLocation()const;
		const Quaternion&	GetWorldRotation()const;
		const Vector3&		GetWorldScaling()const;
		const Matrix4x4&	GetLocalToWorldMatrix()const;
	protected:
		virtual void		SetupProxy(PPrimitiveComponent* InComponent);
	protected:
		Vector3			WorldLocation;
		Quaternion		WorldRotation;
		Vector3			WorldScaling;
		Matrix4x4		LocalToWorldMatrix;

		RENDER_PROXY_CLASS_BODY(RPrimitiveSceneProxy)

	};

};