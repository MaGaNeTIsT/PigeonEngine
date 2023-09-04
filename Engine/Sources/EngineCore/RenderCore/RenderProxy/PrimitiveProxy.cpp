#include "PrimitiveProxy.h"
#include <Renderer/RenderScene.h>
#include <PigeonBase/Object/Component/PrimitiveComponent.h>

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<RPrimitiveProxy, EObjectBase>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	RPrimitiveProxy::RPrimitiveProxy(const RScene* InScene, const PPrimitiveComponent* InComponent)
		: WorldLocation(InComponent->GetComponentWorldLocation())
		, WorldRotation(InComponent->GetComponentWorldRotation())
		, WorldScaling(InComponent->GetComponentWorldScale())
		, Scene(InScene)
		, Component(InComponent)
	{
		LocalToWorldMatrix = MakeMatrix4x4(WorldLocation, WorldRotation, WorldScaling);
	}
	RPrimitiveProxy::RPrimitiveProxy()
		: WorldLocation(Vector3::Zero())
		, WorldRotation(Quaternion::Identity())
		, WorldScaling(Vector3::One())
		, LocalToWorldMatrix(Matrix4x4::Identity())
		, Scene(nullptr)
		, Component(nullptr)
	{
	}
	RPrimitiveProxy::RPrimitiveProxy(const RPrimitiveProxy& Other)
		: WorldLocation(Other.WorldLocation)
		, WorldRotation(Other.WorldRotation)
		, WorldScaling(Other.WorldScaling)
		, LocalToWorldMatrix(Other.LocalToWorldMatrix)
		, Scene(Other.Scene)
		, Component(Other.Component)
	{
	}
	RPrimitiveProxy::~RPrimitiveProxy()
	{
	}
	BOOL32 RPrimitiveProxy::IsRenderValid()const
	{
		return FALSE;
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