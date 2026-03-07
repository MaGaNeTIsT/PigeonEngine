#include "ViewProxy.h"
#include <RenderDevice/DeviceD3D11.h>
#include <Renderer/RenderScene.h>

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<RViewProxy, RBaseSceneProxy>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	void RViewMaterialParameter::SetupParameters()
	{
		ClearParameter();
		BeginSetupParameter();
		AddParameter<Matrix4x4, EShaderParameterValueType::SHADER_PARAMETER_TYPE_MATRIX44>(("_ViewMatrix"));
		AddParameter<Matrix4x4, EShaderParameterValueType::SHADER_PARAMETER_TYPE_MATRIX44>(("_ViewInvMatrix"));
		AddParameter<Matrix4x4, EShaderParameterValueType::SHADER_PARAMETER_TYPE_MATRIX44>(("_ProjectionMatrix"));
		AddParameter<Matrix4x4, EShaderParameterValueType::SHADER_PARAMETER_TYPE_MATRIX44>(("_ProjectionInvMatrix"));
		AddParameter<Matrix4x4, EShaderParameterValueType::SHADER_PARAMETER_TYPE_MATRIX44>(("_ViewProjectionMatrix"));
		AddParameter<Matrix4x4, EShaderParameterValueType::SHADER_PARAMETER_TYPE_MATRIX44>(("_ViewProjectionInvMatrix"));
		AddParameter<Vector4, EShaderParameterValueType::SHADER_PARAMETER_TYPE_FLOAT4>(("_TimeParams"));
		AddParameter<Vector4, EShaderParameterValueType::SHADER_PARAMETER_TYPE_FLOAT4>(("_DepthMultiAdd"));
		AddParameter<Vector4, EShaderParameterValueType::SHADER_PARAMETER_TYPE_FLOAT4>(("_ScreenToViewSpaceParams"));
		AddParameter<Vector4, EShaderParameterValueType::SHADER_PARAMETER_TYPE_FLOAT4>(("_CameraViewportMinSizeAndInvBufferSize"));
		AddParameter<Vector4, EShaderParameterValueType::SHADER_PARAMETER_TYPE_FLOAT4>(("_CameraViewportSizeAndInvSize"));
		AddParameter<Vector4, EShaderParameterValueType::SHADER_PARAMETER_TYPE_FLOAT4>(("_CameraViewportRect"));
		AddParameter<Vector4, EShaderParameterValueType::SHADER_PARAMETER_TYPE_FLOAT4>(("_CameraWorldPosition"));
		EndSetupParameter();
		CreateBuffer();
	}

	RViewProxy::RViewProxy(const PCameraComponent* InComponent)
		: IsMainView(FALSE), Component(InComponent)
	{
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Create view proxy failed"), (!!Component));
	}
	RViewProxy::RViewProxy()
		: IsMainView(FALSE), Component(nullptr)
	{
	}
	RViewProxy::RViewProxy(const RViewProxy& Other)
		: RBaseSceneProxy(Other), IsMainView(FALSE), VisibilityMap(Other.VisibilityMap)
		, CameraViewInfo(Other.CameraViewInfo), ViewDomainInfo(Other.ViewDomainInfo), Component(Other.Component)
	{
	}
	RViewProxy::~RViewProxy()
	{
	}
	void RViewProxy::SetupProxy(const BOOL32 InIsMainView, const ERenderViewMatrices& InMatrices, const ERenderViewParams& InParams)
	{
		UpdateViewSettings(InIsMainView);
		UpdateViewParams(InParams);
		UpdateMatrices(InMatrices);

		ViewMaterialParameter.SetupParameters();

		UpdateRenderResource();
	}
	BOOL32 RViewProxy::IsMainRenderView()const
	{
		return IsMainView;
	}
	RViewProxy::RVisibilityMapType& RViewProxy::GetVisibilityMap()
	{
		return VisibilityMap;
	}
	const RViewProxy::RVisibilityMapType& RViewProxy::GetVisibilityMap()const
	{
		return VisibilityMap;
	}
	ECameraViewInfo& RViewProxy::GetViewInfo()
	{
		return CameraViewInfo;
	}
	const ECameraViewInfo& RViewProxy::GetViewInfo()const
	{
		return CameraViewInfo;
	}
	EViewport& RViewProxy::GetRenderViewport()
	{
		return (ViewDomainInfo.RenderViewport);
	}
	const EViewport& RViewProxy::GetRenderViewport()const
	{
		return (ViewDomainInfo.RenderViewport);
	}
	EViewMatrix& RViewProxy::GetViewMatrix()
	{
		return (ViewDomainInfo.ViewMatrix);
	}
	const EViewMatrix& RViewProxy::GetViewMatrix()const
	{
		return (ViewDomainInfo.ViewMatrix);
	}
	EFrustum& RViewProxy::GetViewFrustum()
	{
		return (ViewDomainInfo.ViewFrustum);
	}
	const EFrustum& RViewProxy::GetViewFrustum()const
	{
		return (ViewDomainInfo.ViewFrustum);
	}
	void RViewProxy::ResetVisibilityMap()
	{
		VisibilityMap.Empty();
	}
	void RViewProxy::UpdateViewSettings(const BOOL32 InIsMainView)
	{
		IsMainView = InIsMainView;
	}
	void RViewProxy::UpdateMatrices(const ERenderViewMatrices& InMatrices)
	{
		SetProxyWorldTransform(InMatrices.WorldLocation, InMatrices.WorldRotation, Vector3::One());
		ViewDomainInfo.ViewMatrix = InMatrices.ViewMatrix;

		// Params need update by transform changed
		ViewDomainInfo.ViewFrustum.GeneratePlaneWorldSpace(WorldLocation, WorldRotation);
		ViewDomainInfo.ViewFrustum.GenerateSeparatingProjectionWorldSpace();
	}
	void RViewProxy::UpdateViewParams(const ERenderViewParams& InParams)
	{
		ViewDomainInfo.ViewFrustum	= InParams.ViewFrustum;
		CameraViewInfo				= InParams.CameraViewInfo;
		{
			ViewDomainInfo.RenderViewport.TopLeftX	= 0.f;
			ViewDomainInfo.RenderViewport.TopLeftY	= 0.f;
			ViewDomainInfo.RenderViewport.Width		= CameraViewInfo.Viewport.Width;
			ViewDomainInfo.RenderViewport.Height	= CameraViewInfo.Viewport.Height;
			ViewDomainInfo.RenderViewport.MinDepth	= RCommonSettings::RENDER_DEPTH_MIN;
			ViewDomainInfo.RenderViewport.MaxDepth	= RCommonSettings::RENDER_DEPTH_MAX;
		}
	}
	void RViewProxy::UpdateRenderResource()
	{
		ViewMaterialParameter["_ViewMatrix"] = &TranslateUploadMatrixType(ViewDomainInfo.ViewMatrix.ViewPart.ViewMatrix);
		ViewMaterialParameter["_ViewInvMatrix"] = &TranslateUploadMatrixType(ViewDomainInfo.ViewMatrix.ViewPart.InverseViewMatrix);
		ViewMaterialParameter["_ProjectionMatrix"] = &TranslateUploadMatrixType(ViewDomainInfo.ViewMatrix.ProjectionPart.ProjectionMatrix);
		ViewMaterialParameter["_ProjectionInvMatrix"] = &TranslateUploadMatrixType(ViewDomainInfo.ViewMatrix.ProjectionPart.InverseProjectionMatrix);
		ViewMaterialParameter["_ViewProjectionMatrix"] = &TranslateUploadMatrixType(ViewDomainInfo.ViewMatrix.ViewProjectionMatrix);
		ViewMaterialParameter["_ViewProjectionInvMatrix"] = &TranslateUploadMatrixType(ViewDomainInfo.ViewMatrix.InverseViewProjectionMatrix);
		ViewMaterialParameter["_TimeParams"] = &TranslateUploadVectorType(Vector4(0.f, 0.f, 0.f, 0.f));
		ViewMaterialParameter["_DepthMultiAdd"] = &TranslateUploadVectorType(Vector4(0.f, 0.f, 0.f, 0.f));
		ViewMaterialParameter["_ScreenToViewSpaceParams"] = &TranslateUploadVectorType(Vector4(0.f, 0.f, 0.f, 0.f));
		ViewMaterialParameter["_CameraViewportMinSizeAndInvBufferSize"] = &TranslateUploadVectorType(Vector4(ViewDomainInfo.RenderViewport.TopLeftX, ViewDomainInfo.RenderViewport.TopLeftY, 1.f / ViewDomainInfo.RenderViewport.Width, 1.f / ViewDomainInfo.RenderViewport.Height));
		ViewMaterialParameter["_CameraViewportSizeAndInvSize"] = &TranslateUploadVectorType(Vector4(ViewDomainInfo.RenderViewport.Width, ViewDomainInfo.RenderViewport.Height, 1.f / ViewDomainInfo.RenderViewport.Width, 1.f / ViewDomainInfo.RenderViewport.Height));
		ViewMaterialParameter["_CameraViewportRect"] = &TranslateUploadVectorType(Vector4(ViewDomainInfo.RenderViewport.TopLeftX, ViewDomainInfo.RenderViewport.TopLeftY, ViewDomainInfo.RenderViewport.Width, ViewDomainInfo.RenderViewport.Height));
		ViewMaterialParameter["_CameraWorldPosition"] = &TranslateUploadVectorType(MakeVector4(WorldLocation, 0.f));
		ViewMaterialParameter.UploadBuffer();
	}
	void RViewProxy::BindRenderResource(const UINT32 InSolt)const
	{
		BindViewMaterialParameter(InSolt);
	}
	void RViewProxy::BindViewMaterialParameter(const UINT32 InSlot)const
	{
		const RBufferResource& ConstantBuffer = ViewMaterialParameter.GetConstantBuffer();
#if _EDITOR_ONLY
		if (((InSlot >= 0u) && (InSlot < 16u)) && (ConstantBuffer.IsRenderResourceValid()))
#endif
		{
			RDeviceD3D11::GetDeviceSingleton()->BindVSConstantBuffer(ConstantBuffer.Buffer, InSlot);
			RDeviceD3D11::GetDeviceSingleton()->BindPSConstantBuffer(ConstantBuffer.Buffer, InSlot);
			RDeviceD3D11::GetDeviceSingleton()->BindCSConstantBuffer(ConstantBuffer.Buffer, InSlot);
		}
	}

};