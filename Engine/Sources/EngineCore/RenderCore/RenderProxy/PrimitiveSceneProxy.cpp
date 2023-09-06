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

	RPrimitiveSceneProxy::RPrimitiveSceneProxy(const PPrimitiveComponent* InComponent)
		: WorldLocation(InComponent->GetComponentWorldLocation())
		, WorldRotation(InComponent->GetComponentWorldRotation())
		, WorldScaling(InComponent->GetComponentWorldScale())
		, Component(InComponent)
	{
		LocalToWorldMatrix = MakeMatrix4x4(WorldLocation, WorldRotation, WorldScaling);
	}
	RPrimitiveSceneProxy::RPrimitiveSceneProxy()
		: WorldLocation(Vector3::Zero())
		, WorldRotation(Quaternion::Identity())
		, WorldScaling(Vector3::One())
		, LocalToWorldMatrix(Matrix4x4::Identity())
		, Component(nullptr)
	{
	}
	RPrimitiveSceneProxy::RPrimitiveSceneProxy(const RPrimitiveSceneProxy& Other)
		: RBaseSceneProxy(Other)
		, WorldLocation(Other.WorldLocation)
		, WorldRotation(Other.WorldRotation)
		, WorldScaling(Other.WorldScaling)
		, LocalToWorldMatrix(Other.LocalToWorldMatrix)
		, Component(Other.Component)
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

};