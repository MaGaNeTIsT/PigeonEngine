#include "PrimitiveSceneProxy.h"
#include <Renderer/RenderScene.h>
#include <PigeonBase/Object/Component/PrimitiveComponent.h>

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<RPrimitiveSceneProxy, RBaseSceneProxy>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	RPrimitiveSceneProxy::RPrimitiveSceneProxy()
		: WorldLocation(Vector3::Zero())
		, WorldRotation(Quaternion::Identity())
		, WorldScaling(Vector3::One())
		, LocalToWorldMatrix(Matrix4x4::Identity())
	{
	}
	RPrimitiveSceneProxy::RPrimitiveSceneProxy(const RPrimitiveSceneProxy& Other)
		: RBaseSceneProxy(Other)
		, WorldLocation(Other.WorldLocation)
		, WorldRotation(Other.WorldRotation)
		, WorldScaling(Other.WorldScaling)
		, LocalToWorldMatrix(Other.LocalToWorldMatrix)
	{
	}
	RPrimitiveSceneProxy::~RPrimitiveSceneProxy()
	{
	}
	BOOL32 RPrimitiveSceneProxy::IsRenderValid()const
	{
		return FALSE;
	}
	const Vector3& RPrimitiveSceneProxy::GetWorldLocation()const
	{
		return WorldLocation;
	}
	const Quaternion& RPrimitiveSceneProxy::GetWorldRotation()const
	{
		return WorldRotation;
	}
	const Vector3& RPrimitiveSceneProxy::GetWorldScaling()const
	{
		return WorldScaling;
	}
	const Matrix4x4& RPrimitiveSceneProxy::GetLocalToWorldMatrix()const
	{
		return LocalToWorldMatrix;
	}
	void RPrimitiveSceneProxy::SetupProxy(PPrimitiveComponent* InComponent)
	{
		//WorldLocation(Vector3::Zero())
		//	, WorldRotation(Quaternion::Identity())
		//	, WorldScaling(Vector3::One())
		//	, LocalToWorldMatrix(Matrix4x4::Identity())
	}

};