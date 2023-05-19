#include "CameraComponent.h"

namespace PigeonEngine
{

	PCameraComponent::PCameraComponent()
	{
	}
	PCameraComponent::~PCameraComponent()
	{
	}
	void PCameraComponent::SetCameraViewInfo(FLOAT InTopLeftX, FLOAT InTopLeftY, FLOAT InWidth, FLOAT InHeight, FLOAT InFovDegree, FLOAT InFar, FLOAT InNear)
	{
		CameraViewInfo.Viewport.TopLeftX = InTopLeftX;
		CameraViewInfo.Viewport.TopLeftY = InTopLeftY;
		CameraViewInfo.Viewport.Width = InWidth;
		CameraViewInfo.Viewport.Height = InHeight;
		CameraViewInfo.Viewport.MinDepth = 0.f;
		CameraViewInfo.Viewport.MaxDepth = 1.f;
		CameraViewInfo.Fov = InFovDegree;
		CameraViewInfo.Far = InFar;
		CameraViewInfo.Near = InNear;
	}
	void PCameraComponent::SetViewport(FLOAT InTopLeftX, FLOAT InTopLeftY, FLOAT InWidth, FLOAT InHeight)
	{
		CameraViewInfo.Viewport.TopLeftX = InTopLeftX;
		CameraViewInfo.Viewport.TopLeftY = InTopLeftY;
		CameraViewInfo.Viewport.Width = InWidth;
		CameraViewInfo.Viewport.Height = InHeight;
		CameraViewInfo.Viewport.MinDepth = 0.f;
		CameraViewInfo.Viewport.MaxDepth = 1.f;
	}
	void PCameraComponent::SetFov(FLOAT InFovAngleY)
	{
		CameraViewInfo.Fov = InFovAngleY;
	}

};