#pragma once

#include <CoreMinimal.h>
#include <EngineCommon.h>
#include <RenderConfig/RenderConfig.h>
#include <PigeonBase/Object/Component/SceneComponent.h>

namespace PigeonEngine
{

	struct PCameraViewInfo
	{
		PCameraViewInfo()noexcept : Viewport(EViewport(Vector2::Zero(), Vector2::One(), Vector2(RCommonSettings::RENDER_DEPTH_MIN, RCommonSettings::RENDER_DEPTH_MAX)))
			, FovAngleY(RCommonSettings::RENDER_CAMERA_FOV), FarDist(RCommonSettings::RENDER_CAMERA_FAR), NearDist(RCommonSettings::RENDER_CAMERA_NEAR)
		{
		}
		PCameraViewInfo(const PCameraViewInfo& Other)noexcept
			: Viewport(EViewport(Vector2(Other.Viewport.TopLeftX, Other.Viewport.TopLeftY), Vector2(Other.Viewport.Width, Other.Viewport.Height), Vector2(RCommonSettings::RENDER_DEPTH_MIN, RCommonSettings::RENDER_DEPTH_MAX)))
			, FovAngleY(Other.FovAngleY), FarDist(Other.FarDist), NearDist(Other.NearDist)
		{
		}
		PCameraViewInfo(FLOAT InTopLeftX, FLOAT InTopLeftY, FLOAT InWidth, FLOAT InHeight, FLOAT InFovAngleY = RCommonSettings::RENDER_CAMERA_FOV, FLOAT InFarDist = RCommonSettings::RENDER_CAMERA_FAR, FLOAT InNearDist = RCommonSettings::RENDER_CAMERA_NEAR)noexcept
			: Viewport(EViewport(Vector2(InTopLeftX, InTopLeftY), Vector2(InWidth, InHeight), Vector2(RCommonSettings::RENDER_DEPTH_MIN, RCommonSettings::RENDER_DEPTH_MAX)))
			, FovAngleY(InFovAngleY), FarDist(InFarDist), NearDist(InNearDist)
		{
		}
		PCameraViewInfo& operator=(const PCameraViewInfo& Other)
		{
			Viewport = EViewport(
				Vector2(Other.Viewport.TopLeftX, Other.Viewport.TopLeftY),
				Vector2(Other.Viewport.Width, Other.Viewport.Height),
				Vector2(RCommonSettings::RENDER_DEPTH_MIN, RCommonSettings::RENDER_DEPTH_MAX));
			FovAngleY	= Other.FovAngleY;
			FarDist		= Other.FarDist;
			NearDist	= Other.NearDist;
			return (*this);
		}

		EViewport	Viewport;
		FLOAT		FovAngleY;
		FLOAT		FarDist;
		FLOAT		NearDist;
	};

	class PCameraComponent : public PSceneComponent
	{

		CLASS_VIRTUAL_NOCOPY_BODY(PCameraComponent)

	public:
		PCameraComponent(FLOAT InViewportLeftTopX, FLOAT InViewportLeftTopY, FLOAT InViewportWidth, FLOAT InViewportHeight, FLOAT InFovAngleY = RCommonSettings::RENDER_CAMERA_FOV, FLOAT InFarDist = RCommonSettings::RENDER_CAMERA_FAR, FLOAT InNearDist = RCommonSettings::RENDER_CAMERA_NEAR);
		const PCameraViewInfo&	GetCameraViewInfo()const { return CameraViewInfo; }
		const EViewMatrix&		GetCameraMatrix()const { return CameraMatrix; }
		const EFrustum&			GetCameraFrustum()const { return CameraFrustum; }
	public:
		void	SetCameraViewInfo(FLOAT InTopLeftX, FLOAT InTopLeftY, FLOAT InWidth, FLOAT InHeight, FLOAT InFovAngleY = RCommonSettings::RENDER_CAMERA_FOV, FLOAT InFarDist = RCommonSettings::RENDER_CAMERA_FAR, FLOAT InNearDist = RCommonSettings::RENDER_CAMERA_NEAR);
		void	SetViewport(FLOAT InTopLeftX, FLOAT InTopLeftY, FLOAT InWidth, FLOAT InHeight);
		void	SetFov(FLOAT InFovAngleY);
	public:
		Vector3	TransformScreenToWorld(const Vector3& InScreenCoordWithZ)const;
		BOOL32	TransformWorldToScreen(const Vector3& InWorldLocation, Vector3& OutScreenCoordWithZ)const;
		Vector3	TransformWorldToView(const Vector3& InWorldLocation)const;
		Vector3	TransformViewToWorld(const Vector3& InViewLocation)const;
	public:
		void	UpdateCameraMatrix();
	protected:
		PCameraViewInfo		CameraViewInfo;
		EViewMatrix			CameraMatrix;
		EFrustum			CameraFrustum;
	};

};