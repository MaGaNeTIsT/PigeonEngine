#include "CameraComponent.h"
#include <RenderProxy/ViewProxy.h>
#include <PigeonBase/Object/World/World.h>
#include <Renderer/RenderInterface.h>
#if ENGINE_EDITOR_ONLY
#include <Editor/EditorLogManager.h>
#include <imgui.h>
#endif


namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<PCameraComponent, PSceneComponent>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);


	void PCameraComponent::TickRenderImpl()
	{
		if(IsRenderStateDirty())
		{
			UpdateCameraMatrix();
			SendUpdateRenderState();
		}
	}

	PCameraComponent::PCameraComponent(FLOAT InViewportLeftTopX, FLOAT InViewportLeftTopY, FLOAT InViewportWidth, FLOAT InViewportHeight, FLOAT InFovAngleY, FLOAT InFarDist, FLOAT InNearDist)
		: CameraViewInfo(ECameraViewInfo(InViewportLeftTopX, InViewportLeftTopY, InViewportWidth, InViewportHeight, InFovAngleY, InFarDist, InNearDist)), ViewProxy(nullptr), UpdateState(PCameraUpdateState::CAMERA_UPDATE_STATE_NONE)
	{
		CameraMatrix.GeneratePerspectiveProjectPart(CameraViewInfo.Viewport, CameraViewInfo.FovAngleY, CameraViewInfo.NearDist, CameraViewInfo.FarDist);
		CameraMatrix.GenerateViewPart(GetComponentWorldLocation(), GetComponentWorldRotation());
		CameraMatrix.GenerateFinalMatrix();

		RefreshCameraFrustum();
		POBJ_DEBUGNAME_SET(this, "CameraComponent");
	}
	PCameraComponent::PCameraComponent()
		: ViewProxy(nullptr), UpdateState(PCameraUpdateState::CAMERA_UPDATE_STATE_NONE)
	{
		RefreshCameraFrustum();
	}
	PCameraComponent::~PCameraComponent()
	{
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("SceneProxy is not null in camera component distruction."), (!ViewProxy));
	}
	void PCameraComponent::SetCameraViewInfo(FLOAT InTopLeftX, FLOAT InTopLeftY, FLOAT InWidth, FLOAT InHeight, FLOAT InFovAngleY, FLOAT InFarDist, FLOAT InNearDist)
	{
		CameraViewInfo.Viewport.TopLeftX	= InTopLeftX;
		CameraViewInfo.Viewport.TopLeftY	= InTopLeftY;
		CameraViewInfo.Viewport.Width		= InWidth;
		CameraViewInfo.Viewport.Height		= InHeight;
		CameraViewInfo.Viewport.MinDepth	= RCommonSettings::RENDER_DEPTH_NEAR;
		CameraViewInfo.Viewport.MaxDepth	= RCommonSettings::RENDER_DEPTH_FAR;
		CameraViewInfo.FovAngleY	= InFovAngleY;
		CameraViewInfo.FarDist		= InFarDist;
		CameraViewInfo.NearDist		= InNearDist;

		RefreshCameraFrustum();
		CameraMatrix.GeneratePerspectiveProjectPart(CameraViewInfo.Viewport, CameraViewInfo.FovAngleY, CameraViewInfo.NearDist, CameraViewInfo.FarDist);
		CameraMatrix.GenerateFinalMatrix();

		MarkAsDirty(PCameraUpdateState::CAMERA_UPDATE_STATE_VIEW);
		MarkAsDirty(PCameraUpdateState::CAMERA_UPDATE_STATE_MATRIX);
	}
	void PCameraComponent::SetViewport(FLOAT InTopLeftX, FLOAT InTopLeftY, FLOAT InWidth, FLOAT InHeight)
	{
		CameraViewInfo.Viewport.TopLeftX	= InTopLeftX;
		CameraViewInfo.Viewport.TopLeftY	= InTopLeftY;
		CameraViewInfo.Viewport.Width		= InWidth;
		CameraViewInfo.Viewport.Height		= InHeight;
		CameraViewInfo.Viewport.MinDepth = RCommonSettings::RENDER_DEPTH_NEAR;
		CameraViewInfo.Viewport.MaxDepth = RCommonSettings::RENDER_DEPTH_FAR;

		RefreshCameraFrustum();
		CameraMatrix.GeneratePerspectiveProjectPart(CameraViewInfo.Viewport, CameraViewInfo.FovAngleY, CameraViewInfo.NearDist, CameraViewInfo.FarDist);
		CameraMatrix.GenerateFinalMatrix();

		MarkAsDirty(PCameraUpdateState::CAMERA_UPDATE_STATE_VIEW);
		MarkAsDirty(PCameraUpdateState::CAMERA_UPDATE_STATE_MATRIX);
	}
	void PCameraComponent::SetFov(FLOAT InFovAngleY)
	{
		CameraViewInfo.FovAngleY = InFovAngleY;

		RefreshCameraFrustum();
		CameraMatrix.GeneratePerspectiveProjectPart(CameraViewInfo.Viewport, CameraViewInfo.FovAngleY, CameraViewInfo.NearDist, CameraViewInfo.FarDist);
		CameraMatrix.GenerateFinalMatrix();

		MarkAsDirty(PCameraUpdateState::CAMERA_UPDATE_STATE_VIEW);
		MarkAsDirty(PCameraUpdateState::CAMERA_UPDATE_STATE_MATRIX);
	}
	void PCameraComponent::SetExposureValue100(FLOAT InExposureValue100)
	{
		CameraViewInfo.ExposureValue100 = InExposureValue100;

		// Exposure is a view parameter, not a matrix input: the projection is unchanged, only
		// the scalar the display pipeline applies moves.
		MarkAsDirty(PCameraUpdateState::CAMERA_UPDATE_STATE_VIEW);
	}
	void PCameraComponent::SetISO(FLOAT InISO)
	{
		CameraViewInfo.ISO = InISO;

		MarkAsDirty(PCameraUpdateState::CAMERA_UPDATE_STATE_VIEW);
	}
	void PCameraComponent::SetExposureCompensation(FLOAT InStops)
	{
		CameraViewInfo.ExposureCompensation = InStops;

		MarkAsDirty(PCameraUpdateState::CAMERA_UPDATE_STATE_VIEW);
	}
	Vector3 PCameraComponent::TransformScreenToWorld(const Vector3& InScreenCoordWithZ) const
	{
		Vector3 Result;
		CameraMatrix.TransformScreenPointToWorld(CameraViewInfo.Viewport, InScreenCoordWithZ, Result);
		return Result;
	}
	BOOL32 PCameraComponent::TransformWorldToScreen(const Vector3& InWorldLocation, Vector3& OutScreenCoordWithZ) const
	{
		return (CameraMatrix.TransformWorldPointToScreen(CameraViewInfo.Viewport, InWorldLocation, OutScreenCoordWithZ));
	}
	Vector3 PCameraComponent::TransformWorldToView(const Vector3& InWorldLocation) const
	{
		Vector3 Result;
		CameraMatrix.TransformWorldPointToView(InWorldLocation, Result);
		return Result;
	}
	Vector3 PCameraComponent::TransformViewToWorld(const Vector3& InViewLocation) const
	{
		Vector3 Result;
		CameraMatrix.TransformViewPointToWorld(InViewLocation, Result);
		return Result;
	}
	BOOL32 PCameraComponent::IsMainCamera() const
	{
		return bMainCamera;	//TODO
	}
	void PCameraComponent::UpdateCameraMatrix()
	{
		CameraMatrix.GenerateViewPart(GetComponentWorldLocation(), GetComponentWorldRotation());
		CameraMatrix.GenerateFinalMatrix();

		MarkRenderTransformAsDirty();
	}

	void PCameraComponent::SetIsMainCamera(BOOL32 IsMain)
	{
		this->bMainCamera = IsMain;
	}

	// Render proxy functions START
	UINT8 PCameraComponent::GetUpdateRenderState() const
	{
		return UpdateState;
	}
	RViewProxy* PCameraComponent::CreateSceneProxy()
	{
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Try creating mesh scene proxy, but already exist scene proxy."), (!ViewProxy));
		ViewProxy = new RViewProxy(this);
		return ViewProxy;
	}
	void PCameraComponent::CreateRenderState()
	{
		PSceneComponent::CreateRenderState();
		if (ShouldRender())
		{
			if (PWorld* World = this->GetWorld())
			{
				if (RSceneInterface* RenderScene = World->GetRenderScene())
				{
					// Refresh CameraMatrix from the component's current
					// world transform before AddCamera builds the initial
					// ERenderViewMatrices. Without this, the proxy's first
					// SetupProxy snapshots the ctor-time identity ViewPart
					// (PCameraComponent ctor ran before the owner actor had
					// a transform), and the first frame's view CB is identity
					// until the next UpdateCamera lambda is drained.
					UpdateCameraMatrix();
					RenderScene->AddCamera(this);
				}
			}
		}
	}
	void PCameraComponent::DestroyRenderState()
	{
		if (PWorld* World = this->GetWorld())
		{
			if (RSceneInterface* RenderScene = World->GetRenderScene())
			{
				RenderScene->RemoveCamera(this);
			}
		}
		PSceneComponent::DestroyRenderState();
	}
	void PCameraComponent::SendUpdateRenderState()
	{
		if (ShouldRender() && IsRenderStateDirty())
		{
			if (PWorld* World = this->GetWorld())
			{
				if (RSceneInterface* RenderScene = World->GetRenderScene())
				{
					RenderScene->UpdateCamera(this);
				}
			}
		}
		PSceneComponent::SendUpdateRenderState();
	}
	void PCameraComponent::RefreshCameraFrustum()
	{
		// The view space frustum is derived from the projection inputs, never patched in place:
		// the render side transforms its corner points into world space, and an input that kept
		// a previous generation would describe a camera that no longer exists.
		const EViewport& Viewport = CameraViewInfo.Viewport;
		const FLOAT AspectRatio = ((Viewport.Height != 0.f) ? (Viewport.Width / Viewport.Height) : 1.f);
		CameraFrustum.GeneratePerspectiveFrustumInfo(CameraViewInfo.FovAngleY, AspectRatio
			, CameraViewInfo.NearDist, CameraViewInfo.FarDist);
	}
	void PCameraComponent::MarkAsDirty(PCameraUpdateState InState)
	{
		if (InState == PCameraUpdateState::CAMERA_UPDATE_STATE_MATRIX)
		{
			MarkRenderTransformAsDirty();
		}
		else
		{
			UpdateState |= InState;
			MarkRenderStateAsDirty();
		}
	}
	void PCameraComponent::MarkRenderTransformAsDirty()
	{
		UpdateState |= PCameraUpdateState::CAMERA_UPDATE_STATE_MATRIX;
		PSceneComponent::MarkRenderTransformAsDirty();
	}
	void PCameraComponent::MarkRenderStateAsDirty()
	{
		PSceneComponent::MarkRenderStateAsDirty();
	}
	void PCameraComponent::CleanMarkRenderStateDirty()
	{
		UpdateState = PCameraUpdateState::CAMERA_UPDATE_STATE_NONE;
		PSceneComponent::CleanMarkRenderStateDirty();
	}

	void PCameraComponent::BeginAddedToScene(PWorld* World)
	{
		PSceneComponent::BeginAddedToScene(World);
	}

	void PCameraComponent::RemovedFromScene()
	{
		PSceneComponent::RemovedFromScene();
	}

	// Render proxy functions END

#if ENGINE_EDITOR_ONLY
	void PCameraComponent::GenerateComponentDetail()
	{
		PSceneComponent::GenerateComponentDetail();

		if (ImGui::TreeNodeEx("Exposure"))
		{
			// Exposure belongs to the display boundary: these three numbers decide which scene
			// luminance maps to display white, and none of them changes the luminance the scene
			// itself stores.
			FLOAT TempExposureValue100 = GetExposureValue100();
			if (ImGui::DragFloat("EV100", &TempExposureValue100, 0.01f, 0.f, 24.f, "%.2f"))
			{
				SetExposureValue100(TempExposureValue100);
			}

			FLOAT TempISO = GetISO();
			if (ImGui::DragFloat("ISO", &TempISO, 1.f, 25.f, 6400.f, "%.0f"))
			{
				SetISO(TempISO);
			}

			// In stops. Positive values brighten, which is the direction a photographer expects.
			FLOAT TempCompensation = GetExposureCompensation();
			if (ImGui::DragFloat("Compensation", &TempCompensation, 0.1f, -5.f, 5.f, "%.2f EV"))
			{
				SetExposureCompensation(TempCompensation);
			}

			ImGui::Text("Display white at %.1f cd/m2", GetSaturationLuminance());
			ImGui::Text("Gain %.6e", GetExposureGain());

			if (ImGui::Button("Reset"))
			{
				SetExposureValue100(EEngineSettings::ENGINE_CAMERA_EV100);
				SetISO(EEngineSettings::ENGINE_CAMERA_ISO);
				SetExposureCompensation(EEngineSettings::ENGINE_CAMERA_EXPOSURE_COMPENSATION);
			}
			ImGui::TreePop();
		}
	}
#endif

};
