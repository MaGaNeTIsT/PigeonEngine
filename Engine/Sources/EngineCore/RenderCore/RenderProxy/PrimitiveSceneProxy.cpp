#include "PrimitiveSceneProxy.h"
#include <Renderer/RenderScene.h>

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<RPrimitiveSceneProxy, RBaseSceneProxy>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	void RPrimitiveMaterialParameter::AddPrimitiveMaterialParameter()
	{
		AddParameter<Matrix4x4, EShaderParameterValueType::SHADER_PARAMETER_TYPE_MATRIX44>(("_WorldMatrix"));
		AddParameter<Matrix4x4, EShaderParameterValueType::SHADER_PARAMETER_TYPE_MATRIX44>(("_WorldInvMatrix"));
		AddParameter<Matrix4x4, EShaderParameterValueType::SHADER_PARAMETER_TYPE_MATRIX44>(("_WorldInvTransposeMatrix"));
	}

	RPrimitiveSceneProxy::RPrimitiveSceneProxy()
		: IsMovable(FALSE), IsCastShadow(FALSE), IsReceiveShadow(FALSE)
	{
	}
	RPrimitiveSceneProxy::RPrimitiveSceneProxy(const RPrimitiveSceneProxy& Other)
		: RBaseSceneProxy(Other), IsMovable(Other.IsMovable), IsCastShadow(Other.IsCastShadow), IsReceiveShadow(Other.IsReceiveShadow)
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

};