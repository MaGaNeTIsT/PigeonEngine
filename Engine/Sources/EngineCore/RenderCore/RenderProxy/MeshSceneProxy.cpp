#include "MeshSceneProxy.h"
#include <PigeonBase/Object/Component/Primitive/MeshComponent.h>

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
	void RMeshSceneProxy::SetupSceneProxy(PMeshComponent* InComponent, const BOOL32 InIsMovable, const BOOL32 InIsCastShadow, const BOOL32 InIsReceiveShadow)
	{
		RPrimitiveSceneProxy::SetupSceneProxy(InComponent, InIsMovable, InIsCastShadow, InIsReceiveShadow);
		//TODO
	}

};