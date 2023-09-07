#include "SkeletalMeshSceneProxy.h"
#include <PigeonBase/Object/Component/Primitive/SkeletalMeshComponent.h>

namespace PigeonEngine
{

    static void RegisterClassTypes()
    {
        RegisterClassType<RSkeletalMeshSceneProxy, RMeshSceneProxy>();
    }

    PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	RSkeletalMeshSceneProxy::RSkeletalMeshSceneProxy(PSkeletalMeshComponent* InComponent)
		: Component(InComponent)
	{
	}
	RSkeletalMeshSceneProxy::RSkeletalMeshSceneProxy()
		: Component(nullptr)
	{
	}
	RSkeletalMeshSceneProxy::RSkeletalMeshSceneProxy(const RSkeletalMeshSceneProxy& Other)
		: RMeshSceneProxy(Other), Component(Other.Component)
	{
	}
	RSkeletalMeshSceneProxy::~RSkeletalMeshSceneProxy()
	{
	}

};