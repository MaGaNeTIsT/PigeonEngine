#include "StaticMeshSceneProxy.h"
#include <PigeonBase/Object/Component/Primitive/StaticMeshComponent.h>

namespace PigeonEngine
{

    static void RegisterClassTypes()
    {
        RegisterClassType<RStaticMeshSceneProxy, RMeshSceneProxy>();
    }

    PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	RStaticMeshSceneProxy::RStaticMeshSceneProxy(PStaticMeshComponent* InComponent)
		: Component(InComponent)
    {
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

};