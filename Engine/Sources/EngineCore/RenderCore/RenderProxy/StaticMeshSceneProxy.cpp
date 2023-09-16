#include "StaticMeshSceneProxy.h"

namespace PigeonEngine
{

    static void RegisterClassTypes()
    {
        RegisterClassType<RStaticMeshSceneProxy, RMeshSceneProxy>();
    }

    PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	RStaticMeshSceneProxy::RStaticMeshSceneProxy(PStaticMeshComponent* InComponent)
		: MeshAsset(nullptr), Component(InComponent)
	{
		Check((ENGINE_RENDER_CORE_ERROR), ("Create static mesh scene proxy failed"), (!!Component));
	}
	RStaticMeshSceneProxy::RStaticMeshSceneProxy()
		: MeshAsset(nullptr), Component(nullptr)
	{
	}
	RStaticMeshSceneProxy::RStaticMeshSceneProxy(const RStaticMeshSceneProxy& Other)
		: RMeshSceneProxy(Other), MeshAsset(Other.MeshAsset), Component(Other.Component)
	{
	}
	RStaticMeshSceneProxy::~RStaticMeshSceneProxy()
	{
	}
	void RStaticMeshSceneProxy::SetupProxy(const BOOL32 InIsMovable, const BOOL32 InIsCastShadow, const BOOL32 InIsReceiveShadow, const ERenderPrimitiveMatrices& InMatrices, const EStaticMeshAsset* InMeshAsset)
	{
		SetPrimitiveSettings(InIsMovable, InIsCastShadow, InIsReceiveShadow);
		UpdatePrimitiveMatrices(InMatrices);
		UpdateMeshAsset(InMeshAsset);
	}
	void RStaticMeshSceneProxy::UpdateMeshAsset(const EStaticMeshAsset* InMeshAsset)
	{
		MeshAsset = InMeshAsset;
	}

};