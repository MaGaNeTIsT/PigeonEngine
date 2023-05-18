#pragma once

#include <CoreMinimal.h>
#include <PigeonBase/Object/Component/SceneComponent.h>
#include <RenderCommon.h>
#include <RenderConfig/RenderConfig.h>

namespace PigeonEngine
{

	struct PCameraViewInfo
	{
		PCameraViewInfo() : Viewport(RViewport(Vector2::Zero(), Vector2::One(), Vector2::YVector()))
			, Fov(RENDER_CAMERA_FOV), Far(RENDER_CAMERA_FAR), Near(RENDER_CAMERA_NEAR)
		{
		}
		PCameraViewInfo(const PCameraViewInfo& Other)
			: Viewport(RViewport(Vector2(Other.Viewport.TopLeftX, Other.Viewport.TopLeftY), Vector2(Other.Viewport.Width, Other.Viewport.Height), Vector2::YVector()))
			, Fov(Other.Fov), Far(Other.Far), Near(Other.Near)
		{
		}
		PCameraViewInfo(FLOAT InTopLeftX, FLOAT InTopLeftY, FLOAT InWidth, FLOAT InHeight, FLOAT InFov = RENDER_CAMERA_FOV, FLOAT InFar = RENDER_CAMERA_FAR, FLOAT InNear = RENDER_CAMERA_NEAR)
			: Viewport(RViewport(Vector2(InTopLeftX, InTopLeftY), Vector2(InWidth, InHeight), Vector2::YVector()))
			, Fov(InFov), Far(InFar), Near(InNear)
		{
		}

		RViewport	Viewport;
		FLOAT		Fov;
		FLOAT		Far;
		FLOAT		Near;
	};

	class PCameraComponent : public PSceneComponent
	{

		EClass(PCameraComponent, PSceneComponent)

		CLASS_VIRTUAL_NOCOPY_BODY(PCameraComponent)

	public:
		const PCameraViewInfo&	GetCameraViewInfo()const { return CameraViewInfo; }
	public:
		void	SetCameraViewInfo(FLOAT InTopLeftX, FLOAT InTopLeftY, FLOAT InWidth, FLOAT InHeight, FLOAT InFovDegree = RENDER_CAMERA_FOV, FLOAT InFar = RENDER_CAMERA_FAR, FLOAT InNear = RENDER_CAMERA_NEAR);
		void	SetViewport(FLOAT InTopLeftX, FLOAT InTopLeftY, FLOAT InWidth, FLOAT InHeight);
		void	SetFov(FLOAT InFovDegree);
	public:
		void	TransformPointFromWorldToView(const Vector3& input, Vector3& output)const;
		void	TransformPointFromViewToWorld(const Vector3& input, Vector3& output)const;

		/*
		* Return value : if return false, the point is out of screen coord, else in screen.
		* Params input : specific space position of the point.
		* Params output : [0.f, screen max] range of screen coord which has anchor(zero point) in left top of RECT.
		*/
		void	TransformWorldPointToScreenCoord(const Vector3& input, Vector2& output)const;
		BOOL	TransformWorldPointToScreenCoord(const Vector3& input, Vector3& output)const;
		void	TransformViewPointToScreenCoord(const Vector3& input, Vector2& output)const;
		BOOL	TransformViewPointToScreenCoord(const Vector3& input, Vector3& output)const;

		/*
		* Params input X Y : screen space position of the point (range : [0.f, screen max]).
		* Params output : output space direction from camera to point. WARNING! output direction is NOT normalized.
		*/
		void	TransformScreenCoordToWorldPoint(const Vector2& mousePos, Vector3& output)const;
		void	TransformScreenCoordToViewPoint(const Vector2& mousePos, Vector3& output)const;
	protected:
		PCameraViewInfo		CameraViewInfo;
	};

};