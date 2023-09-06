#include "MeshSceneProxy.h"
#include <PigeonBase/Object/Component/MeshComponent.h>

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<RMeshSceneProxy, RPrimitiveSceneProxy>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	RMeshSceneProxy::RMeshSceneProxy(PMeshComponent* InComponent)
		: RPrimitiveSceneProxy(InComponent), Component(InComponent)
	{
	}
	RMeshSceneProxy::RMeshSceneProxy()
		: Component(nullptr)
	{
	}
	RMeshSceneProxy::RMeshSceneProxy(const RMeshSceneProxy& Other)
		: RPrimitiveSceneProxy(Other), Component(Other.Component)
	{
	}
	RMeshSceneProxy::~RMeshSceneProxy()
	{
	}

};