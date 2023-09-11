#include "PrimitiveSceneProxy.h"
#include <Renderer/RenderScene.h>
#include <PigeonBase/Object/Component/Primitive/PrimitiveComponent.h>

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<RPrimitiveSceneProxy, RBaseSceneProxy>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	RPrimitiveSceneProxy::RPrimitiveSceneProxy()
	{
	}
	RPrimitiveSceneProxy::RPrimitiveSceneProxy(const RPrimitiveSceneProxy& Other)
		: RBaseSceneProxy(Other)
	{
	}
	RPrimitiveSceneProxy::~RPrimitiveSceneProxy()
	{
	}
	BOOL32 RPrimitiveSceneProxy::IsRenderValid()const
	{
		return FALSE;
	}
	BOOL32 RPrimitiveSceneProxy::IsSceneProxyMovable()const
	{
		return IsMovable;
	}
	BOOL32 RPrimitiveSceneProxy::IsSceneProxyCastShadow()const
	{
		return IsCastShadow;
	}
	BOOL32 RPrimitiveSceneProxy::IsSceneProxyReceiveShadow()const
	{
		return IsReceiveShadow;
	}
	void RPrimitiveSceneProxy::SetupSceneProxy(PPrimitiveComponent* InComponent, const BOOL32 InIsMovable, const BOOL32 InIsCastShadow, const BOOL32 InIsReceiveShadow)
	{
		IsMovable = InIsMovable;
		IsCastShadow = InIsCastShadow;
		IsReceiveShadow = InIsReceiveShadow;
		SetupProxyWorldTransform(InComponent->GetComponentWorldLocation(), InComponent->GetComponentWorldRotation(), InComponent->GetComponentWorldScale());
		//TODO
	}

};