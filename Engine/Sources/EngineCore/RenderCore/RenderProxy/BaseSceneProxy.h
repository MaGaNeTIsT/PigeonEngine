#pragma once

#include <CoreMinimal.h>

namespace PigeonEngine
{

	class RScene;

	class RBaseSceneProxy : public EObjectBase
	{
	public:
		const Vector3&		GetWorldLocation()const;
		const Quaternion&	GetWorldRotation()const;
		const Vector3&		GetWorldScaling()const;
		const Matrix4x4&	GetLocalToWorldMatrix()const;
	public:
		void	SetupProxyWorldTransform(const Vector3& InWorldLocation, const Quaternion& InWorldRotation, const Vector3& InWorldScaling);
	protected:
		Vector3		WorldLocation;
		Quaternion	WorldRotation;
		Vector3		WorldScaling;
		Matrix4x4	LocalToWorldMatrix;
	public:
		const RScene*	GetRenderScene()const;
		PE_INLINE void	SetRenderScene(const RScene* InScene);
	protected:
		const RScene*	Scene;

		RENDER_PROXY_CLASS_BODY(RBaseSceneProxy)

	};

};