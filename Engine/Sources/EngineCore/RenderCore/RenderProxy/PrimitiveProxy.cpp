#include "PrimitiveProxy.h"

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<RPrimitiveProxy, EObjectBase>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	BOOL32 RPrimitiveProxy::IsRenderValid()const
	{
		//TODO
		return TRUE;
	}
	const Vector3& RPrimitiveProxy::GetWorldLocation()const
	{
		return WorldLocation;
	}
	const Quaternion& RPrimitiveProxy::GetWorldRotation()const
	{
		return WorldRotation;
	}
	const Vector3& RPrimitiveProxy::GetWorldScaling()const
	{
		return WorldScaling;
	}
	const Matrix4x4& RPrimitiveProxy::GetLocalToWorldMatrix()const
	{
		return LocalToWorldMatrix;
	}

};