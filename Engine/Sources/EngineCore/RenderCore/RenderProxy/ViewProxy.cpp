#include "ViewProxy.h"
#include <RenderDevice/DeviceD3D11.h>
#include <RHI/IRCommandList.h>
#include <RHI/IRRHIDevice.h>
#include <Renderer/RenderScene.h>
#include <Main/MainManager.h>
#include <ShaderAsset/ShaderProgram.h>

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<RViewProxy, RBaseSceneProxy>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	RViewProxy::RViewProxy(const PCameraComponent* InComponent)
		: bParameterValuesDirty(TRUE), IsMainView(FALSE), Component(InComponent)
	{
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Create view proxy failed"), (!!Component));
	}
	RViewProxy::RViewProxy()
		: bParameterValuesDirty(TRUE), IsMainView(FALSE), Component(nullptr)
	{
	}
	RViewProxy::RViewProxy(const RViewProxy& Other)
		: RBaseSceneProxy(Other), ViewParameterBlock(Other.ViewParameterBlock), bParameterValuesDirty(TRUE), IsMainView(FALSE), VisibilityMap(Other.VisibilityMap)
		, CameraViewInfo(Other.CameraViewInfo), ViewDomainInfo(Other.ViewDomainInfo), Component(Other.Component)
	{
	}
	RViewProxy::~RViewProxy()
	{
	}
	void RViewProxy::SetupProxy(const BOOL32 InIsMainView, const ERenderViewMatrices& InMatrices, const ERenderViewParams& InParams)
	{
		UpdateViewSettings(InIsMainView);
		UpdateViewParams(InParams);
		UpdateMatrices(InMatrices);

		// The block has no layout yet: it comes from the first program that declares it, so
		// the values are recorded here and turn into gpu bytes on the first draw of the view.
		UpdateRenderResource();
	}
	BOOL32 RViewProxy::IsMainRenderView()const
	{
		return IsMainView;
	}
	RViewProxy::RVisibilityMapType& RViewProxy::GetVisibilityMap()
	{
		return VisibilityMap;
	}
	const RViewProxy::RVisibilityMapType& RViewProxy::GetVisibilityMap()const
	{
		return VisibilityMap;
	}
	ECameraViewInfo& RViewProxy::GetViewInfo()
	{
		return CameraViewInfo;
	}
	const ECameraViewInfo& RViewProxy::GetViewInfo()const
	{
		return CameraViewInfo;
	}
	EViewport& RViewProxy::GetRenderViewport()
	{
		return (ViewDomainInfo.RenderViewport);
	}
	const EViewport& RViewProxy::GetRenderViewport()const
	{
		return (ViewDomainInfo.RenderViewport);
	}
	EViewMatrix& RViewProxy::GetViewMatrix()
	{
		return (ViewDomainInfo.ViewMatrix);
	}
	const EViewMatrix& RViewProxy::GetViewMatrix()const
	{
		return (ViewDomainInfo.ViewMatrix);
	}
	EFrustum& RViewProxy::GetViewFrustum()
	{
		return (ViewDomainInfo.ViewFrustum);
	}
	const EFrustum& RViewProxy::GetViewFrustum()const
	{
		return (ViewDomainInfo.ViewFrustum);
	}
	void RViewProxy::ResetVisibilityMap()
	{
		VisibilityMap.Empty();
	}
	void RViewProxy::UpdateViewSettings(const BOOL32 InIsMainView)
	{
		IsMainView = InIsMainView;
	}
	void RViewProxy::UpdateMatrices(const ERenderViewMatrices& InMatrices)
	{
		SetProxyWorldTransform(InMatrices.WorldLocation, InMatrices.WorldRotation, Vector3::One());
		ViewDomainInfo.ViewMatrix = InMatrices.ViewMatrix;

		// Params need update by transform changed
		ViewDomainInfo.ViewFrustum.GeneratePlaneWorldSpace(WorldLocation, WorldRotation);
		ViewDomainInfo.ViewFrustum.GenerateSeparatingProjectionWorldSpace();

		bParameterValuesDirty = TRUE;
	}
	void RViewProxy::UpdateViewParams(const ERenderViewParams& InParams)
	{
		ViewDomainInfo.ViewFrustum	= InParams.ViewFrustum;
		CameraViewInfo				= InParams.CameraViewInfo;
		{
			ViewDomainInfo.RenderViewport.TopLeftX	= 0.f;
			ViewDomainInfo.RenderViewport.TopLeftY	= 0.f;
			ViewDomainInfo.RenderViewport.Width		= CameraViewInfo.Viewport.Width;
			ViewDomainInfo.RenderViewport.Height	= CameraViewInfo.Viewport.Height;
			ViewDomainInfo.RenderViewport.MinDepth	= RCommonSettings::RENDER_DEPTH_NEAR;
			ViewDomainInfo.RenderViewport.MaxDepth	= RCommonSettings::RENDER_DEPTH_FAR;
		}

		bParameterValuesDirty = TRUE;
	}
	BOOL32 RViewProxy::EnsureParameterBlock(IRRHIDevice* InRenderDevice, const EShaderProgram& InProgram)
	{
		if (ViewParameterBlock.IsInitialized())
		{
			return TRUE;
		}
		if (!ViewParameterBlock.EnsureInitialized(InRenderDevice, InProgram))
		{
			return FALSE;
		}
		// The values recorded before the layout existed have never reached a gpu buffer;
		// this is the call that turns them into bytes for the first time.
		bParameterValuesDirty = TRUE;
		UpdateRenderResource();
		return TRUE;
	}
	void RViewProxy::UpdateRenderResource()
	{
		if (!bParameterValuesDirty)
		{
			return;
		}
		bParameterValuesDirty = FALSE;

		const EName BlockName(SHADER_BLOCK_VIEW_PARAMS);
		EShaderParameterSet& Parameters = ViewParameterBlock.GetParameters();

		// Matrices are written as they are, never transposed: the shader side matrix is the
		// transpose of the cpu matrix, which is what the library's mul(Matrix, Vector)
		// expects. See the convention note in ShaderEngineParameters.h.
		WriteShaderMatrix(Parameters, BlockName, EName(SHADER_FIELD_VIEW_MATRIX), ViewDomainInfo.ViewMatrix.ViewPart.ViewMatrix);
		WriteShaderMatrix(Parameters, BlockName, EName(SHADER_FIELD_VIEW_INV_MATRIX), ViewDomainInfo.ViewMatrix.ViewPart.InverseViewMatrix);
		WriteShaderMatrix(Parameters, BlockName, EName(SHADER_FIELD_PROJECTION_MATRIX), ViewDomainInfo.ViewMatrix.ProjectionPart.ProjectionMatrix);
		WriteShaderMatrix(Parameters, BlockName, EName(SHADER_FIELD_PROJECTION_INV_MATRIX), ViewDomainInfo.ViewMatrix.ProjectionPart.InverseProjectionMatrix);
		WriteShaderMatrix(Parameters, BlockName, EName(SHADER_FIELD_VIEW_PROJECTION_MATRIX), ViewDomainInfo.ViewMatrix.ViewProjectionMatrix);
		WriteShaderMatrix(Parameters, BlockName, EName(SHADER_FIELD_VIEW_PROJECTION_INV_MATRIX), ViewDomainInfo.ViewMatrix.InverseViewProjectionMatrix);

		const EViewport& RenderViewport = ViewDomainInfo.RenderViewport;
		const FLOAT InvViewportWidth	= ((RenderViewport.Width > 0.f) ? (1.f / RenderViewport.Width) : 0.f);
		const FLOAT InvViewportHeight	= ((RenderViewport.Height > 0.f) ? (1.f / RenderViewport.Height) : 0.f);

		// The camera forward axis is the third row of the inverse view matrix: for a row
		// vector view matrix the inverse view is the camera's world matrix, whose rows are
		// the right, up and forward axes.
		const Matrix4x4& InverseView = ViewDomainInfo.ViewMatrix.ViewPart.InverseViewMatrix;
		WriteShaderVector4(Parameters, BlockName, EName(SHADER_FIELD_VIEW_POSITION), Vector4(WorldLocation.x, WorldLocation.y, WorldLocation.z, 1.f));
		WriteShaderVector4(Parameters, BlockName, EName(SHADER_FIELD_VIEW_DIRECTION)
			, Vector4(InverseView(2u, 0u), InverseView(2u, 1u), InverseView(2u, 2u), 0.f));
		// ScreenSize and InvScreenSize are declared float2, so each write states two components.
		// Handing them to WriteShaderVector4 would supply two values the field has no room for,
		// which the parameter set drops while reporting a mismatched write.
		const FLOAT ScreenSizeScalars[2] = { RenderViewport.Width, RenderViewport.Height };
		const FLOAT InvScreenSizeScalars[2] = { InvViewportWidth, InvViewportHeight };
		WriteShaderFloatArray(Parameters, BlockName, EName(SHADER_FIELD_SCREEN_SIZE), ScreenSizeScalars, 2u);
		WriteShaderFloatArray(Parameters, BlockName, EName(SHADER_FIELD_INV_SCREEN_SIZE), InvScreenSizeScalars, 2u);
		WriteShaderVector4(Parameters, BlockName, EName(SHADER_FIELD_NEAR_FAR_PARAMS)
			, Vector4(ViewDomainInfo.ViewMatrix.ProjectionPart.DeviceZToViewZMulAdd.x, ViewDomainInfo.ViewMatrix.ProjectionPart.DeviceZToViewZMulAdd.y, 0.f, 0.f));
		WriteShaderVector4(Parameters, BlockName, EName(SHADER_FIELD_VIEWPORT_RECT)
			, Vector4(RenderViewport.TopLeftX, RenderViewport.TopLeftY, RenderViewport.Width, RenderViewport.Height));

		// Exposure belongs to this view: two cameras on the same scene can be exposed
		// differently, so the gain is published here rather than in the per frame block. The
		// gain is computed on the cpu and carried in .w so the display pipeline is one
		// multiply, while EV100 and ISO ride along for a debug display to read.
		WriteShaderVector4(Parameters, BlockName, EName(SHADER_FIELD_EXPOSURE_PARAMS)
			, Vector4(CameraViewInfo.ExposureValue100, CameraViewInfo.ISO
				, CameraViewInfo.ExposureCompensation, CameraViewInfo.GetExposureGain()));

		IRRHIDevice* RenderDevice = EMainManager::GetManagerSingleton()->GetRenderDevice();
		ViewParameterBlock.Upload(RenderDevice);
	}
	void RViewProxy::BindParameters(IRCommandList* InCommandList, const EShaderProgram& InProgram)const
	{
		// Must run after the pipeline state is set: a root signature switch voids every root
		// binding, so a view bound once per frame would be lost by the next material's PSO.
		ViewParameterBlock.Bind(InCommandList, InProgram);
	}

};
