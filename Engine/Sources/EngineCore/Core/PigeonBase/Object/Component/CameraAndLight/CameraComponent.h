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
		ECameraViewInfo() noexcept : Viewport(EViewport(Vector2::Zero(), Vector2::One(), Vector2(RCommonSettings::RENDER_DEPTH_NEAR, RCommonSettings::RENDER_DEPTH_FAR)))
			, FovAngleY(EEngineSettings::ENGINE_CAMERA_FOV), FarDist(RCommonSettings::RENDER_CAMERA_FAR), NearDist(RCommonSettings::RENDER_CAMERA_NEAR)
			, ExposureValue100(EEngineSettings::ENGINE_CAMERA_EV100), ISO(EEngineSettings::ENGINE_CAMERA_ISO)
			, ExposureCompensation(EEngineSettings::ENGINE_CAMERA_EXPOSURE_COMPENSATION)
		{
		}
		ECameraViewInfo(const ECameraViewInfo& Other) noexcept
			: Viewport(EViewport(Vector2(Other.Viewport.TopLeftX, Other.Viewport.TopLeftY), Vector2(Other.Viewport.Width, Other.Viewport.Height), Vector2(RCommonSettings::RENDER_DEPTH_NEAR, RCommonSettings::RENDER_DEPTH_FAR)))
			, FovAngleY(Other.FovAngleY), FarDist(Other.FarDist), NearDist(Other.NearDist)
			, ExposureValue100(Other.ExposureValue100), ISO(Other.ISO)
			, ExposureCompensation(Other.ExposureCompensation)
		{
		}
		ECameraViewInfo(FLOAT InTopLeftX, FLOAT InTopLeftY, FLOAT InWidth, FLOAT InHeight, FLOAT InFovAngleY = EEngineSettings::ENGINE_CAMERA_FOV, FLOAT InFarDist = RCommonSettings::RENDER_CAMERA_FAR, FLOAT InNearDist = RCommonSettings::RENDER_CAMERA_NEAR) noexcept
			: Viewport(EViewport(Vector2(InTopLeftX, InTopLeftY), Vector2(InWidth, InHeight), Vector2(RCommonSettings::RENDER_DEPTH_NEAR, RCommonSettings::RENDER_DEPTH_FAR)))
			, FovAngleY(InFovAngleY), FarDist(InFarDist), NearDist(InNearDist)
			, ExposureValue100(EEngineSettings::ENGINE_CAMERA_EV100), ISO(EEngineSettings::ENGINE_CAMERA_ISO)
			, ExposureCompensation(EEngineSettings::ENGINE_CAMERA_EXPOSURE_COMPENSATION)
		{
		}
		ECameraViewInfo& operator=(const ECameraViewInfo& Other)
		{
			Viewport = EViewport(
				Vector2(Other.Viewport.TopLeftX, Other.Viewport.TopLeftY),
				Vector2(Other.Viewport.Width, Other.Viewport.Height),
				Vector2(RCommonSettings::RENDER_DEPTH_NEAR, RCommonSettings::RENDER_DEPTH_FAR));
			FovAngleY	= Other.FovAngleY;
			FarDist		= Other.FarDist;
			NearDist	= Other.NearDist;
			ExposureValue100		= Other.ExposureValue100;
			ISO						= Other.ISO;
			ExposureCompensation	= Other.ExposureCompensation;
			return (*this);
		}
		// The luminance in cd/m2 that maps to display white, from the exposure value at ISO 100
		// and the sensitivity. The 99.3 is the calibration constant that makes EV100 14.64 put
		// display white just above a sunlit white surface, which is the Sunny-16 rule.
		PE_NODISCARD FLOAT GetSaturationLuminance() const noexcept
		{
			const FLOAT SafeISO = ((ISO > 0.001f) ? ISO : 0.001f);
			return ((99.3f * EMath::Exp2(ExposureValue100)) / SafeISO);
		}
		// Scene luminance to display signal, the one scalar the display pipeline applies before
		// tone mapping. Exposure compensation is in stops, so it divides in linear space.
		PE_NODISCARD FLOAT GetExposureGain() const noexcept
		{
			const FLOAT SaturationLuminance = GetSaturationLuminance();
			if (SaturationLuminance <= 0.f)
			{
				return 0.f;
			}
			return (EMath::Exp2(-ExposureCompensation) / SaturationLuminance);
		}

		EViewport	Viewport;
		FLOAT		FovAngleY;
		FLOAT		FarDist;
		FLOAT		NearDist;
		// Physical exposure. EV100 is the exposure value at ISO 100, and the pair is the whole
		// exposure API; aperture and shutter reach it as EV, never as separate fields.
		FLOAT		ExposureValue100;
		FLOAT		ISO;
		FLOAT		ExposureCompensation;	// in stops (EV)
	};

	struct ERenderViewMatrices : public ERenderTransformInfo
	{
	public:
		EViewMatrix		ViewMatrix;
	public:
		ERenderViewMatrices(const Vector3& InWorldLocation, const Quaternion& InWorldRotation
			, const Vector3& InWorldScaling, const EViewMatrix& InViewMatrix)
			: ERenderTransformInfo(InWorldLocation, InWorldRotation, InWorldScaling), ViewMatrix(InViewMatrix)
		{
		}
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
		ERenderViewParams(const EFrustum& InViewFrustum, const ECameraViewInfo& InCameraViewInfo)
			: ViewFrustum(InViewFrustum), CameraViewInfo(InCameraViewInfo)
		{
		}
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
		friend class PController;

		CLASS_VIRTUAL_NOCOPY_BODY(PCameraComponent)
	protected:
		virtual void TickRenderImpl() override;
	public:

		PCameraComponent(FLOAT InViewportLeftTopX, FLOAT InViewportLeftTopY, FLOAT InViewportWidth, FLOAT InViewportHeight, FLOAT InFovAngleY = EEngineSettings::ENGINE_CAMERA_FOV, FLOAT InFarDist = RCommonSettings::RENDER_CAMERA_FAR, FLOAT InNearDist = RCommonSettings::RENDER_CAMERA_NEAR);
		const ECameraViewInfo&	GetCameraViewInfo()const { return CameraViewInfo; }
		const EViewMatrix&		GetCameraMatrix()const { return CameraMatrix; }
		const EFrustum&			GetCameraFrustum()const { return CameraFrustum; }
	public:
		void	SetCameraViewInfo(FLOAT InTopLeftX, FLOAT InTopLeftY, FLOAT InWidth, FLOAT InHeight, FLOAT InFovAngleY = EEngineSettings::ENGINE_CAMERA_FOV, FLOAT InFarDist = RCommonSettings::RENDER_CAMERA_FAR, FLOAT InNearDist = RCommonSettings::RENDER_CAMERA_NEAR);
		void	SetViewport(FLOAT InTopLeftX, FLOAT InTopLeftY, FLOAT InWidth, FLOAT InHeight);
		void	SetFov(FLOAT InFovAngleY);
	public:
		// Exposure. EV100 and ISO are the whole exposure API: aperture and shutter are folded
		// into EV100 by whoever authors them, which is what keeps the number the renderer reads
		// independent of how the shot was described.
		FLOAT	GetExposureValue100() const noexcept { return CameraViewInfo.ExposureValue100; }
		FLOAT	GetISO() const noexcept { return CameraViewInfo.ISO; }
		FLOAT	GetExposureCompensation() const noexcept { return CameraViewInfo.ExposureCompensation; }
		// The scene luminance in cd/m2 that this camera maps to display white.
		FLOAT	GetSaturationLuminance() const noexcept { return CameraViewInfo.GetSaturationLuminance(); }
		FLOAT	GetExposureGain() const noexcept { return CameraViewInfo.GetExposureGain(); }
	public:
		void	SetExposureValue100(FLOAT InExposureValue100);
		void	SetISO(FLOAT InISO);
		// In stops. Positive values brighten, which is the direction a photographer expects.
		void	SetExposureCompensation(FLOAT InStops);
	public:
		Vector3	TransformScreenToWorld(const Vector3& InScreenCoordWithZ)const;
		BOOL32	TransformWorldToScreen(const Vector3& InWorldLocation, Vector3& OutScreenCoordWithZ)const;
		Vector3	TransformWorldToView(const Vector3& InWorldLocation)const;
		Vector3	TransformViewToWorld(const Vector3& InViewLocation)const;
	public:
		BOOL32	IsMainCamera()const;
		void	UpdateCameraMatrix();
		void	SetIsMainCamera(BOOL32 IsMain);
	protected:
		BOOL32				bMainCamera = FALSE;
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
		RViewProxy*			ViewProxy;	// ONLY render thread could use this proxy pointer.
	public:
		UINT8				GetUpdateRenderState()const;
		RViewProxy*			CreateSceneProxy();
		virtual void		CreateRenderState()override;
		virtual void		DestroyRenderState()override;
		virtual void		SendUpdateRenderState()override;
	protected:
		void				MarkAsDirty(PCameraUpdateState InState);
		virtual void		MarkRenderTransformAsDirty()override;
		virtual void		MarkRenderStateAsDirty()override;
		virtual void		CleanMarkRenderStateDirty()override;
	protected:
		UINT8				UpdateState;
		// Render proxy functions END
	protected:
		// for frame, 
		void BeginAddedToScene(PWorld* World) override;
		void RemovedFromScene() override;


	};

};
