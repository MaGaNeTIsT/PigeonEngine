#include "CameraComponent.h"

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<PCameraComponent, PSceneComponent>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	PCameraComponent::PCameraComponent()
	{
	}
	PCameraComponent::~PCameraComponent()
	{
	}
	PCameraComponent::PCameraComponent(FLOAT InViewportLeftTopX, FLOAT InViewportLeftTopY, FLOAT InViewportWidth, FLOAT InViewportHeight, FLOAT InFovAngleY, FLOAT InFarDist, FLOAT InNearDist)
		: CameraViewInfo(PCameraViewInfo(InViewportLeftTopX, InViewportLeftTopY, InViewportWidth, InViewportHeight, InFovAngleY, InFarDist, InNearDist))
	{
		CameraMatrix.GenerateProjectPart(CameraViewInfo.Viewport, CameraViewInfo.FovAngleY, CameraViewInfo.NearDist, CameraViewInfo.FarDist);
		CameraMatrix.GenerateViewPart(GetComponentWorldLocation(), GetComponentWorldRotation());
		CameraMatrix.GenerateFinalMatrix();

		CameraFrustum.GenerateFrustumInfo(CameraViewInfo.FovAngleY, CameraViewInfo.Viewport.Width / CameraViewInfo.Viewport.Height, CameraViewInfo.NearDist, CameraViewInfo.FarDist);
	}
	void PCameraComponent::SetCameraViewInfo(FLOAT InTopLeftX, FLOAT InTopLeftY, FLOAT InWidth, FLOAT InHeight, FLOAT InFovAngleY, FLOAT InFarDist, FLOAT InNearDist)
	{
		CameraViewInfo.Viewport.TopLeftX = InTopLeftX;
		CameraViewInfo.Viewport.TopLeftY = InTopLeftY;
		CameraViewInfo.Viewport.Width = InWidth;
		CameraViewInfo.Viewport.Height = InHeight;
		CameraViewInfo.Viewport.MinDepth = RENDER_DEPTH_MIN;
		CameraViewInfo.Viewport.MaxDepth = RENDER_DEPTH_MAX;
		CameraViewInfo.FovAngleY = InFovAngleY;
		CameraViewInfo.FarDist = InFarDist;
		CameraViewInfo.NearDist = InNearDist;

		CameraMatrix.GenerateProjectPart(CameraViewInfo.Viewport, CameraViewInfo.FovAngleY, CameraViewInfo.NearDist, CameraViewInfo.FarDist);
		CameraMatrix.GenerateFinalMatrix();
	}
	void PCameraComponent::SetViewport(FLOAT InTopLeftX, FLOAT InTopLeftY, FLOAT InWidth, FLOAT InHeight)
	{
		CameraViewInfo.Viewport.TopLeftX = InTopLeftX;
		CameraViewInfo.Viewport.TopLeftY = InTopLeftY;
		CameraViewInfo.Viewport.Width = InWidth;
		CameraViewInfo.Viewport.Height = InHeight;
		CameraViewInfo.Viewport.MinDepth = RENDER_DEPTH_MIN;
		CameraViewInfo.Viewport.MaxDepth = RENDER_DEPTH_MAX;

		CameraMatrix.GenerateProjectPart(CameraViewInfo.Viewport, CameraViewInfo.FovAngleY, CameraViewInfo.NearDist, CameraViewInfo.FarDist);
		CameraMatrix.GenerateFinalMatrix();
	}
	void PCameraComponent::SetFov(FLOAT InFovAngleY)
	{
		CameraViewInfo.FovAngleY = InFovAngleY;

		CameraMatrix.GenerateProjectPart(CameraViewInfo.Viewport, CameraViewInfo.FovAngleY, CameraViewInfo.NearDist, CameraViewInfo.FarDist);
		CameraMatrix.GenerateFinalMatrix();
	}
	Vector3 PCameraComponent::TransformScreenToWorld(const Vector3& InScreenCoordWithZ)const
	{
		Vector3 Result;
		CameraMatrix.TransformScreenPointToWorld(CameraViewInfo.Viewport, InScreenCoordWithZ, Result);
		return Result;
	}
	BOOL PCameraComponent::TransformWorldToScreen(const Vector3& InWorldLocation, Vector3& OutScreenCoordWithZ)const
	{
		return (CameraMatrix.TransformWorldPointToScreen(CameraViewInfo.Viewport, InWorldLocation, OutScreenCoordWithZ));
	}
	Vector3 PCameraComponent::TransformWorldToView(const Vector3& InWorldLocation)const
	{
		Vector3 Result;
		CameraMatrix.TransformWorldPointToView(InWorldLocation, Result);
		return Result;
	}
	Vector3 PCameraComponent::TransformViewToWorld(const Vector3& InViewLocation)const
	{
		Vector3 Result;
		CameraMatrix.TransformViewPointToWorld(InViewLocation, Result);
		return Result;
	}
	void PCameraComponent::UpdateCameraMatrix()
	{
		CameraMatrix.GenerateViewPart(GetComponentWorldLocation(), GetComponentWorldRotation());
		CameraMatrix.GenerateFinalMatrix();
	}

};