#include "ViewProxy.h"
#include <Renderer/RenderScene.h>

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<RViewProxy, RBaseSceneProxy>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	RViewProxy::RViewProxy(const PCameraComponent* InComponent)
		: Component(InComponent)
	{
		Check((ENGINE_RENDER_CORE_ERROR), ("Create view proxy failed"), (!!Component));
	}
	RViewProxy::RViewProxy()
		: Component(nullptr)
	{
	}
	RViewProxy::RViewProxy(const RViewProxy& Other)
		: RBaseSceneProxy(Other), VisibilityMap(Other.VisibilityMap), CameraViewInfo(Other.CameraViewInfo), ViewDomainInfo(Other.ViewDomainInfo), Component(Other.Component)
	{
	}
	RViewProxy::~RViewProxy()
	{
	}
	void RViewProxy::SetupProxy(const ERenderViewMatrices& InMatrices, const ERenderViewParams& InParams)
	{
		UpdateViewParams(InParams);

		UpdateMatrices(InMatrices);
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
	void RViewProxy::ResetVisibilityMap()
	{
		VisibilityMap.Clear();
	}

};