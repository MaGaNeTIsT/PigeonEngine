#include "BaseSceneProxy.h"
// The default CollectShadowViews takes a TArray<FShadowViewRequest>& parameter, and instantiating
// the array's allocator needs the element type complete. The header can only forward declare it -
// ShadowViewProxy.h includes ViewProxy.h, which includes this header - so the definition lives
// here, in the translation unit that instantiates the array.
#include "ShadowViewProxy.h"

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
	BOOL32 RBaseSceneProxy::CollectShadowViews(const RViewProxy& InView, TArray<FShadowViewRequest>& OutRequests)const
	{
		(void)InView;
		(void)OutRequests;
		return FALSE;
	}
	BOOL32 RBaseSceneProxy::BuildShadowViewDomain(const RViewProxy& InView, const FShadowViewRequest& InRequest
		, const UINT32 InResolution, EViewDomainInfo& OutDomain
		, Vector3& OutAnchor, Quaternion& OutRotation)const
	{
		(void)InView;
		(void)InRequest;
		(void)InResolution;
		(void)OutDomain;
		(void)OutAnchor;
		(void)OutRotation;
		return FALSE;
	}
	const Vector3& RBaseSceneProxy::GetWorldLocation()const
	{
		return WorldLocation;
	}
	const Quaternion& RBaseSceneProxy::GetWorldRotation()const
	{
		return WorldRotation;
	}
	const Vector3& RBaseSceneProxy::GetWorldScaling()const
	{
		return WorldScaling;
	}
	const Matrix4x4& RBaseSceneProxy::GetLocalToWorldMatrix()const
	{
		return LocalToWorldMatrix;
	}
	void RBaseSceneProxy::SetProxyWorldTransform(const Vector3& InWorldLocation, const Quaternion& InWorldRotation, const Vector3& InWorldScaling)
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
