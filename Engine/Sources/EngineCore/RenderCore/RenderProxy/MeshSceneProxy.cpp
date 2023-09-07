#include "MeshSceneProxy.h"
#include <PigeonBase/Object/Component/MeshComponent.h>

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<RMeshSceneProxy, RPrimitiveSceneProxy>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	RMeshSceneProxy::RMeshSceneProxy()
	{
	}
	RMeshSceneProxy::RMeshSceneProxy(const RMeshSceneProxy& Other)
		: RPrimitiveSceneProxy(Other)
	{
	}
	RMeshSceneProxy::~RMeshSceneProxy()
	{
	}

};