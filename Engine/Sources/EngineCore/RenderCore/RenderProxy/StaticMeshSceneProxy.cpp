#include "StaticMeshSceneProxy.h"

#include <MeshAsset/MeshAsset.h>
#include <RenderProxy/PrimitiveSceneProxyFactory.h>

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<RStaticMeshSceneProxy, RMeshSceneProxy>();
		// The render thread builds this proxy itself, inside the frame's add command, so the
		// component never hands a game thread object across the boundary. The name is the one
		// PStaticMeshComponent reports through GetRenderProxyTypeName.
		RPrimitiveSceneProxyFactory::Register(EName(PE_TEXT("StaticMeshPrimitive"))
			, []() -> RPrimitiveSceneProxy* { return new RStaticMeshSceneProxy(); });
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	RStaticMeshSceneProxy::RStaticMeshSceneProxy()
		: MeshAsset(nullptr)
	{
	}
	RStaticMeshSceneProxy::RStaticMeshSceneProxy(const PStaticMeshComponent* InComponent)
		: MeshAsset(nullptr)
	{
		// The component is not stored: everything this proxy needs arrives through the
		// snapshot, so no game thread object is reachable from here. The argument stays so the
		// existing create path keeps its meaning while the scene moves to the factory.
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Create static mesh scene proxy failed"), (!!InComponent));
	}
	RStaticMeshSceneProxy::RStaticMeshSceneProxy(const RStaticMeshSceneProxy& Other)
		: RMeshSceneProxy(Other), MeshAsset(Other.MeshAsset)
	{
	}
	RStaticMeshSceneProxy::~RStaticMeshSceneProxy()
	{
	}
	BOOL32 RStaticMeshSceneProxy::IsRenderValid()const
	{
		//TODO
		return TRUE;
	}
	void RStaticMeshSceneProxy::ApplyAssets(const FPrimitiveRenderAssetReport& InAssets)
	{
		// The report carries the asset as its generic handle; a static mesh component only ever
		// reports a static mesh asset, so the concrete handle is what it pointed at.
		MeshAsset = static_cast<const EStaticMeshAsset*>(InAssets.GetMeshAsset());
		RMeshSceneProxy::ApplyAssets(InAssets);
	}
	const EMesh* RStaticMeshSceneProxy::GetMeshColdData()const
	{
		return (MeshAsset ? MeshAsset->GetStoragedResource() : nullptr);
	}
	const EMeshRenderResource* RStaticMeshSceneProxy::GetMeshRenderResource()const
	{
		return (MeshAsset ? MeshAsset->GetRenderResource() : nullptr);
	}

} // namespace PigeonEngine
