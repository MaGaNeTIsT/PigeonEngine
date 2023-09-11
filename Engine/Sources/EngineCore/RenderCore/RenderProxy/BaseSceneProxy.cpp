#include "BaseSceneProxy.h"

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<RBaseSceneProxy, EObjectBase>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	RBaseSceneProxy::RBaseSceneProxy()
		: WorldLocation(Vector3::Zero())
		, WorldRotation(Quaternion::Identity())
		, WorldScaling(Vector3::One())
		, LocalToWorldMatrix(Matrix4x4::Identity())
		, Scene(nullptr)
	{
	}
	RBaseSceneProxy::RBaseSceneProxy(const RBaseSceneProxy& Other)
		: WorldLocation(Other.WorldLocation)
		, WorldRotation(Other.WorldRotation)
		, WorldScaling(Other.WorldScaling)
		, LocalToWorldMatrix(Other.LocalToWorldMatrix)
		, Scene(nullptr)
	{
	}
	RBaseSceneProxy::~RBaseSceneProxy()
	{
	}
	void RBaseSceneProxy::SetupProxyWorldTransform(const Vector3& InWorldLocation, const Quaternion& InWorldRotation, const Vector3& InWorldScaling)
	{
		WorldLocation		= InWorldLocation;
		WorldRotation		= InWorldRotation;
		WorldScaling		= InWorldScaling;
		LocalToWorldMatrix	= MakeMatrix4x4(WorldLocation, WorldRotation, WorldScaling);
	}
	const RScene* RBaseSceneProxy::GetRenderScene()const
	{
		return Scene;
	}
	void RBaseSceneProxy::SetRenderScene(const RScene* InScene)
	{
		Scene = nullptr;
		if (!!InScene)
		{
			Scene = InScene;
		}
	}

};