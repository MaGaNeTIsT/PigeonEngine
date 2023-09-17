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

	RViewProxy::RViewProxy(const PCameraComponent* InComponent)
		:
#if _EDITOR_ONLY
		ViewCBufferSize(0u),
#endif
		IsMainView(FALSE), Component(InComponent)
	{
		Check((ENGINE_RENDER_CORE_ERROR), ("Create view proxy failed"), (!!Component));
	}
	RViewProxy::RViewProxy()
		:
#if _EDITOR_ONLY
		ViewCBufferSize(0u),
#endif
		IsMainView(FALSE), Component(nullptr)
	{
	}
	RViewProxy::RViewProxy(const RViewProxy& Other)
		: RBaseSceneProxy(Other)
#if _EDITOR_ONLY
		, ViewCBufferSize(Other.ViewCBufferSize)
#endif
		, ViewCBuffer(Other.ViewCBuffer), IsMainView(FALSE), VisibilityMap(Other.VisibilityMap), CameraViewInfo(Other.CameraViewInfo), ViewDomainInfo(Other.ViewDomainInfo), Component(Other.Component)
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

		CreateViewCBuffer<RViewCBufferData>();
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
		VisibilityMap.Clear();
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
		RViewCBufferData NewData;
		NewData._ViewMatrix = TranslateUploadType(ViewDomainInfo.ViewMatrix.ViewPart.ViewMatrix);
		NewData._ViewInvMatrix = TranslateUploadType(ViewDomainInfo.ViewMatrix.ViewPart.InverseViewMatrix);
		NewData._ProjectionMatrix = TranslateUploadType(ViewDomainInfo.ViewMatrix.ProjectionPart.ProjectionMatrix);
		NewData._ProjectionInvMatrix = TranslateUploadType(ViewDomainInfo.ViewMatrix.ProjectionPart.InverseProjectionMatrix);
		NewData._ViewProjectionMatrix = TranslateUploadType(ViewDomainInfo.ViewMatrix.ViewProjectionMatrix);
		NewData._ViewProjectionInvMatrix = TranslateUploadType(ViewDomainInfo.ViewMatrix.InverseViewProjectionMatrix);
		NewData._TimeParams = TranslateUploadType(Vector4(0.f, 0.f, 0.f, 0.f));
		NewData._DepthMultiAdd = TranslateUploadType(Vector4(0.f, 0.f, 0.f, 0.f));
		NewData._ScreenToViewSpaceParams = TranslateUploadType(Vector4(0.f, 0.f, 0.f, 0.f));
		NewData._CameraViewportMinSizeAndInvBufferSize = TranslateUploadType(Vector4(ViewDomainInfo.RenderViewport.TopLeftX, ViewDomainInfo.RenderViewport.TopLeftY, 1.f / ViewDomainInfo.RenderViewport.Width, 1.f / ViewDomainInfo.RenderViewport.Height));
		NewData._CameraViewportSizeAndInvSize = TranslateUploadType(Vector4(ViewDomainInfo.RenderViewport.Width, ViewDomainInfo.RenderViewport.Height, 1.f / ViewDomainInfo.RenderViewport.Width, 1.f / ViewDomainInfo.RenderViewport.Height));
		NewData._CameraViewportRect = TranslateUploadType(Vector4(ViewDomainInfo.RenderViewport.TopLeftX, ViewDomainInfo.RenderViewport.TopLeftY, ViewDomainInfo.RenderViewport.Width, ViewDomainInfo.RenderViewport.Height));
		NewData._CameraWorldPosition = TranslateUploadType(MakeVector4(WorldLocation, 0.f));
		UploadViewCBuffer(&NewData);
	}
	void RViewProxy::BindRenderResource()
	{
		BindViewCBuffer(0u);
	}
	const RBufferResource& RViewProxy::GetViewCBuffer()const
	{
		return ViewCBuffer;
	}
	template<typename _TStructType>
	void RViewProxy::CreateViewCBuffer()
	{
#if _EDITOR_ONLY
		ViewCBufferSize = sizeof(_TStructType);
#endif
		RDeviceD3D11::GetDeviceSingleton()->CreateBuffer(ViewCBuffer.Buffer,
			RBufferDesc(sizeof(_TStructType), RBindFlagType::BIND_CONSTANT_BUFFER, sizeof(FLOAT)));
	}
	template<typename _TStructType>
	void RViewProxy::UploadViewCBuffer(const _TStructType* InStruct)
	{
#if _EDITOR_ONLY
		if (ViewCBufferSize != sizeof(_TStructType) || (!(ViewCBuffer.IsRenderResourceValid())))
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Upload view constant buffer failed(buffer size is not matched or buffer is null)."));
			return;
		}
#endif
		RDeviceD3D11::GetDeviceSingleton()->UploadBuffer(ViewCBuffer.Buffer, InStruct);
	}
	void RViewProxy::BindViewCBuffer(const UINT32 InSlot)
	{
#if _EDITOR_ONLY
		if (ViewCBuffer.IsRenderResourceValid())
#endif
		{
			RDeviceD3D11::GetDeviceSingleton()->BindVSConstantBuffer(ViewCBuffer.Buffer, InSlot);
		}
	}

};