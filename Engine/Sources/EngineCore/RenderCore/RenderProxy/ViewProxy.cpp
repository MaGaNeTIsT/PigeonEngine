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
			ViewMatrix	= InComponent->GetCameraMatrix();
			ViewFrustum	= InComponent->GetCameraFrustum();
			{
				Quaternion TempR(InComponent->GetComponentWorldRotation());
				for (UINT32 i = 0u; i < 4u; i++)
				{
					ViewFrustum.Plane[i] = QuaternionTransformVector(TempR, ViewFrustum.Plane[i]);
				}
				for (UINT32 i = 0u; i < 8u; i++)
				{
					ViewFrustum.FarNearPlanePoint[i] = Matrix4x4TransformPosition(ViewMatrix.ViewPart.InverseViewMatrix, ViewFrustum.FarNearPlanePoint[i]);
				}
			}
			CameraViewInfo = InComponent->GetCameraViewInfo();
			{
				RenderViewport.TopLeftX	= 0.f;
				RenderViewport.TopLeftY	= 0.f;
				RenderViewport.Width	= CameraViewInfo.Viewport.Width;
				RenderViewport.Height	= CameraViewInfo.Viewport.Height;
				RenderViewport.MinDepth = RCommonSettings::RENDER_DEPTH_MIN;
				RenderViewport.MaxDepth = RCommonSettings::RENDER_DEPTH_MAX;
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
		, RenderViewport(Other.RenderViewport)
		, ViewMatrix(Other.ViewMatrix)
		, ViewFrustum(Other.ViewFrustum)
		, Scene(Other.Scene)
		, Component(Other.Component)
	{
	}
	RViewProxy::~RViewProxy()
	{
	}

};