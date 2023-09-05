#include "ViewProxy.h"
#include <Renderer/RenderScene.h>

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<RViewProxy, EObjectBase>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	RViewProxy::RViewProxy(const RScene* InScene, const PCameraComponent* InComponent)
		: Scene(InScene), Component(InComponent)
	{
		if (InComponent)
		{
			ViewDomainInfo.ViewMatrix	= InComponent->GetCameraMatrix();
			ViewDomainInfo.ViewFrustum	= InComponent->GetCameraFrustum();
			{
				Quaternion TempR(InComponent->GetComponentWorldRotation());
				for (UINT32 i = 0u; i < 4u; i++)
				{
					ViewDomainInfo.ViewFrustum.Plane[i] = QuaternionTransformVector(TempR, ViewDomainInfo.ViewFrustum.Plane[i]);
				}
				for (UINT32 i = 0u; i < 8u; i++)
				{
					ViewDomainInfo.ViewFrustum.FarNearPlanePoint[i] = Matrix4x4TransformPosition(ViewDomainInfo.ViewMatrix.ViewPart.InverseViewMatrix, ViewDomainInfo.ViewFrustum.FarNearPlanePoint[i]);
				}
			}
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
		: Scene(nullptr), Component(nullptr)
	{
	}
	RViewProxy::RViewProxy(const RViewProxy& Other)
		: VisibilityMap(Other.VisibilityMap)
		, CameraViewInfo(Other.CameraViewInfo)
		, ViewDomainInfo(Other.ViewDomainInfo)
		, Scene(Other.Scene)
		, Component(Other.Component)
	{
	}
	RViewProxy::~RViewProxy()
	{
	}
	const PCameraViewInfo& RViewProxy::GetViewInfo()const
	{
		return CameraViewInfo;
	}
	const EViewport& RViewProxy::GetRenderViewport()const
	{
		return (ViewDomainInfo.RenderViewport);
	}
	const EViewMatrix& RViewProxy::GetViewMatrix()const
	{
		return (ViewDomainInfo.ViewMatrix);
	}
	const EFrustum& RViewProxy::GetViewFrustum()const
	{
		return (ViewDomainInfo.ViewFrustum);
	}

};