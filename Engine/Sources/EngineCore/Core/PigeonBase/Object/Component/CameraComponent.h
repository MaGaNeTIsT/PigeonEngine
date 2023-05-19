#pragma once

#include <CoreMinimal.h>
#include <PigeonBase/Object/Component/SceneComponent.h>
#include <EngineCommon.h>
#include <RenderConfig/RenderConfig.h>

namespace PigeonEngine
{

	struct PCameraViewInfo
	{
		PCameraViewInfo() : Viewport(EViewport(Vector2::Zero(), Vector2::One(), Vector2::YVector()))
			, Fov(RENDER_CAMERA_FOV), Far(RENDER_CAMERA_FAR), Near(RENDER_CAMERA_NEAR)
		{
		}
		PCameraViewInfo(const PCameraViewInfo& Other)
			: Viewport(EViewport(Vector2(Other.Viewport.TopLeftX, Other.Viewport.TopLeftY), Vector2(Other.Viewport.Width, Other.Viewport.Height), Vector2::YVector()))
			, Fov(Other.Fov), Far(Other.Far), Near(Other.Near)
		{
		}
		PCameraViewInfo(FLOAT InTopLeftX, FLOAT InTopLeftY, FLOAT InWidth, FLOAT InHeight, FLOAT InFov = RENDER_CAMERA_FOV, FLOAT InFar = RENDER_CAMERA_FAR, FLOAT InNear = RENDER_CAMERA_NEAR)
			: Viewport(EViewport(Vector2(InTopLeftX, InTopLeftY), Vector2(InWidth, InHeight), Vector2::YVector()))
			, Fov(InFov), Far(InFar), Near(InNear)
		{
		}

		EViewport	Viewport;
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
		void	SetFov(FLOAT InFovAngleY);
	public:
		
	protected:
		PCameraViewInfo		CameraViewInfo;
		ECameraMatrix		CameraMatrix;
	};

};