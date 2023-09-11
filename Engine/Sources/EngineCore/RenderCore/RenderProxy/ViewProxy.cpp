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
		if (InComponent)
		{
			SetupProxyWorldTransform(InComponent->GetComponentWorldLocation(), InComponent->GetComponentWorldRotation(), Vector3::One());
			ViewDomainInfo.ViewMatrix = InComponent->GetCameraMatrix();
			ViewDomainInfo.ViewFrustum = InComponent->GetCameraFrustum();
			ViewDomainInfo.ViewFrustum.GeneratePlaneWorldSpace(WorldLocation, WorldRotation);
			ViewDomainInfo.ViewFrustum.GenerateSeparatingProjectionWorldSpace();
			CameraViewInfo = InComponent->GetCameraViewInfo();
			{
				ViewDomainInfo.RenderViewport.TopLeftX	= 0.f;
				ViewDomainInfo.RenderViewport.TopLeftY	= 0.f;
				ViewDomainInfo.RenderViewport.Width		= CameraViewInfo.Viewport.Width;
				ViewDomainInfo.RenderViewport.Height	= CameraViewInfo.Viewport.Height;
				ViewDomainInfo.RenderViewport.MinDepth	= RCommonSettings::RENDER_DEPTH_MIN;
				ViewDomainInfo.RenderViewport.MaxDepth	= RCommonSettings::RENDER_DEPTH_MAX;
			}
		}
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
	RViewProxy::RVisibilityMapType& RViewProxy::GetVisibilityMap()
	{
		return VisibilityMap;
	}
	const RViewProxy::RVisibilityMapType& RViewProxy::GetVisibilityMap()const
	{
		return VisibilityMap;
	}
	PCameraViewInfo& RViewProxy::GetViewInfo()
	{
		return CameraViewInfo;
	}
	const PCameraViewInfo& RViewProxy::GetViewInfo()const
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

};