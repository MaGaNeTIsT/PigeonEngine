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
	void PCameraComponent::SetFov(FLOAT InFovDegree)
	{
		CameraViewInfo.Fov = InFovDegree;
	}
	void PCameraComponent::TransformPointFromWorldToView(const Vector3& input, Vector3& output)const;
	void PCameraComponent::TransformPointFromViewToWorld(const Vector3& input, Vector3& output)const;
	void PCameraComponent::TransformWorldPointToScreenCoord(const Vector3& input, Vector2& output)const;
	BOOL PCameraComponent::TransformWorldPointToScreenCoord(const Vector3& input, Vector3& output)const;
	void PCameraComponent::TransformViewPointToScreenCoord(const Vector3& input, Vector2& output)const;
	BOOL PCameraComponent::TransformViewPointToScreenCoord(const Vector3& input, Vector3& output)const;
	void PCameraComponent::TransformScreenCoordToWorldPoint(const Vector2& mousePos, Vector3& output)const;
	void PCameraComponent::TransformScreenCoordToViewPoint(const Vector2& mousePos, Vector3& output)const;

};