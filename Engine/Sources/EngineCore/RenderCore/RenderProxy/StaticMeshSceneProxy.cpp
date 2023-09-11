#include "StaticMeshSceneProxy.h"
#include <PigeonBase/Object/Component/Primitive/StaticMeshComponent.h>

namespace PigeonEngine
{

    static void RegisterClassTypes()
    {
        RegisterClassType<RStaticMeshSceneProxy, RMeshSceneProxy>();
    }

    PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	RStaticMeshSceneProxy::RStaticMeshSceneProxy(PStaticMeshComponent* InComponent, const BOOL32 InIsMovable, const BOOL32 InIsCastShadow, const BOOL32 InIsReceiveShadow)
		: Component(InComponent)
	{
		SetupSceneProxy(InComponent, InIsMovable, InIsCastShadow, InIsReceiveShadow);
	}
	RStaticMeshSceneProxy::RStaticMeshSceneProxy()
		: Component(nullptr)
	{
	}
	RStaticMeshSceneProxy::RStaticMeshSceneProxy(const RStaticMeshSceneProxy& Other)
		: RMeshSceneProxy(Other), Component(Other.Component)
	{
	}
	RStaticMeshSceneProxy::~RStaticMeshSceneProxy()
	{
	}
	void RStaticMeshSceneProxy::SetupSceneProxy(PStaticMeshComponent* InComponent, const BOOL32 InIsMovable, const BOOL32 InIsCastShadow, const BOOL32 InIsReceiveShadow)
	{
		RMeshSceneProxy::SetupSceneProxy(InComponent, InIsMovable, InIsCastShadow, InIsReceiveShadow);
		//TODO
	}

};