#include "PrimitiveSceneProxy.h"
#include <Renderer/RenderScene.h>

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<RPrimitiveSceneProxy, RBaseSceneProxy>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	RPrimitiveSceneProxy::RPrimitiveSceneProxy()
		: IsMovable(FALSE), IsCastShadow(FALSE), IsReceiveShadow(FALSE)
#if _EDITOR_ONLY
		, PrimitiveCBufferSize(0u)
#endif
	{
	}
	RPrimitiveSceneProxy::RPrimitiveSceneProxy(const RPrimitiveSceneProxy& Other)
		: RBaseSceneProxy(Other), IsMovable(Other.IsMovable), IsCastShadow(Other.IsCastShadow), IsReceiveShadow(Other.IsReceiveShadow)
#if _EDITOR_ONLY
		, PrimitiveCBufferSize(Other.PrimitiveCBufferSize)
#endif
		, PrimitiveCBuffer(Other.PrimitiveCBuffer)
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
	void RPrimitiveSceneProxy::SetPrimitiveSettings(const BOOL32 InIsMovable, const BOOL32 InIsCastShadow, const BOOL32 InIsReceiveShadow)
	{
		IsMovable		= InIsMovable;
		IsCastShadow	= InIsCastShadow;
		IsReceiveShadow = InIsReceiveShadow;
	}
	void RPrimitiveSceneProxy::UpdatePrimitiveMatrices(const ERenderPrimitiveMatrices& InMatrices)
	{
		SetProxyWorldTransform(InMatrices.WorldLocation, InMatrices.WorldRotation, InMatrices.WorldScaling);
	}
	void RPrimitiveSceneProxy::UpdateRenderResource()
	{

	}
	void RPrimitiveSceneProxy::BindRenderResource()
	{

	}
	const RBufferResource& RPrimitiveSceneProxy::GetPrimitiveCBuffer()const
	{
		return PrimitiveCBuffer;
	}
	void RPrimitiveSceneProxy::BindPrimitiveCBuffer(const UINT32 InSlot)
	{
#if _EDITOR_ONLY
		if (PrimitiveCBuffer.IsRenderResourceValid())
#endif
		{
			RDeviceD3D11::GetDeviceSingleton()->BindVSConstantBuffer(PrimitiveCBuffer.Buffer, InSlot);
		}
	}

};