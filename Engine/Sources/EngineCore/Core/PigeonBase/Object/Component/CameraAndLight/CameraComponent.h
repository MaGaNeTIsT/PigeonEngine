#pragma once

#include <CoreMinimal.h>
#include <EngineCommon.h>
#include <Config/EngineConfig.h>
#include <RenderConfig/RenderConfig.h>
#include <PigeonBase/Object/Component/SceneComponent.h>

namespace PigeonEngine
{

	class RViewProxy;

	struct ECameraViewInfo
	{
		ECameraViewInfo()noexcept : Viewport(EViewport(Vector2::Zero(), Vector2::One(), Vector2(RCommonSettings::RENDER_DEPTH_MIN, RCommonSettings::RENDER_DEPTH_MAX)))
			, FovAngleY(ESettings::ENGINE_CAMERA_FOV), FarDist(RCommonSettings::RENDER_CAMERA_FAR), NearDist(RCommonSettings::RENDER_CAMERA_NEAR)
		{
		}
		ECameraViewInfo(const ECameraViewInfo& Other)noexcept
			: Viewport(EViewport(Vector2(Other.Viewport.TopLeftX, Other.Viewport.TopLeftY), Vector2(Other.Viewport.Width, Other.Viewport.Height), Vector2(RCommonSettings::RENDER_DEPTH_MIN, RCommonSettings::RENDER_DEPTH_MAX)))
			, FovAngleY(Other.FovAngleY), FarDist(Other.FarDist), NearDist(Other.NearDist)
		{
		}
		ECameraViewInfo(FLOAT InTopLeftX, FLOAT InTopLeftY, FLOAT InWidth, FLOAT InHeight, FLOAT InFovAngleY = ESettings::ENGINE_CAMERA_FOV, FLOAT InFarDist = RCommonSettings::RENDER_CAMERA_FAR, FLOAT InNearDist = RCommonSettings::RENDER_CAMERA_NEAR)noexcept
			: Viewport(EViewport(Vector2(InTopLeftX, InTopLeftY), Vector2(InWidth, InHeight), Vector2(RCommonSettings::RENDER_DEPTH_MIN, RCommonSettings::RENDER_DEPTH_MAX)))
			, FovAngleY(InFovAngleY), FarDist(InFarDist), NearDist(InNearDist)
		{
		}
		ECameraViewInfo& operator=(const ECameraViewInfo& Other)
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

	struct ERenderViewMatrices : public ERenderTransformInfo
	{
	public:
		EViewMatrix		ViewMatrix;
	public:
		ERenderViewMatrices() = default;
		ERenderViewMatrices(const ERenderViewMatrices& Other)
			: ERenderTransformInfo(Other), ViewMatrix(Other.ViewMatrix)
		{
		}
		ERenderViewMatrices& operator=(const ERenderViewMatrices& Other)
		{
			ERenderTransformInfo::CopyFromOther(Other);
			ViewMatrix = Other.ViewMatrix;
			return (*this);
		}
	};

	struct ERenderViewParams
	{
	public:
		EFrustum		ViewFrustum;
		ECameraViewInfo	CameraViewInfo;
	public:
		ERenderViewParams() = default;
		ERenderViewParams(const ERenderViewParams& Other)
			: ViewFrustum(Other.ViewFrustum), CameraViewInfo(Other.CameraViewInfo)
		{
		}
		ERenderViewParams& operator=(const ERenderViewParams& Other)
		{
			ViewFrustum		= Other.ViewFrustum;
			CameraViewInfo	= Other.CameraViewInfo;
			return (*this);
		}
	};

	class PCameraComponent : public PSceneComponent
	{

		CLASS_VIRTUAL_NOCOPY_BODY(PCameraComponent)

	public:
		PCameraComponent(FLOAT InViewportLeftTopX, FLOAT InViewportLeftTopY, FLOAT InViewportWidth, FLOAT InViewportHeight, FLOAT InFovAngleY = ESettings::ENGINE_CAMERA_FOV, FLOAT InFarDist = RCommonSettings::RENDER_CAMERA_FAR, FLOAT InNearDist = RCommonSettings::RENDER_CAMERA_NEAR);
		const ECameraViewInfo&	GetCameraViewInfo()const { return CameraViewInfo; }
		const EViewMatrix&		GetCameraMatrix()const { return CameraMatrix; }
		const EFrustum&			GetCameraFrustum()const { return CameraFrustum; }
	public:
		void	SetCameraViewInfo(FLOAT InTopLeftX, FLOAT InTopLeftY, FLOAT InWidth, FLOAT InHeight, FLOAT InFovAngleY = ESettings::ENGINE_CAMERA_FOV, FLOAT InFarDist = RCommonSettings::RENDER_CAMERA_FAR, FLOAT InNearDist = RCommonSettings::RENDER_CAMERA_NEAR);
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
		ECameraViewInfo		CameraViewInfo;
		EViewMatrix			CameraMatrix;
		EFrustum			CameraFrustum;

		// Render proxy functions START
	public:
		enum PCameraUpdateState : UINT8
		{
			CAMERA_UPDATE_STATE_NONE	= 0,
			CAMERA_UPDATE_STATE_MATRIX	= (1 << 0),
			CAMERA_UPDATE_STATE_VIEW	= (1 << 1)
		};
	public:
		UINT8	GetUpdateRenderState()const;
		void	MarkAsDirty(PCameraUpdateState InState);
	protected:
		RViewProxy*		ViewProxy;
		UINT8			UpdateState;
	public:
		RViewProxy*			GetSceneProxy();
		const RViewProxy*	GetSceneProxy()const;
		RViewProxy*			CreateSceneProxy();
		virtual void		MarkRenderStateAsDirty()override;
		virtual void		CreateRenderState()override;
		virtual void		DestroyRenderState()override;
		virtual void		SendUpdateRenderState()override;
	protected:
		virtual void		CleanMarkRenderStateDirty()override;
		// Render proxy functions END
	};

};