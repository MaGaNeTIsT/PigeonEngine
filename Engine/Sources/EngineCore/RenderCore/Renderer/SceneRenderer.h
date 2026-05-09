#pragma once

#include <CoreMinimal.h>
#include "RenderScene.h"
#include <RenderResource.h>
#include <RHI/IRRHIDevice.h>
#include <RenderProxy/RenderSingletonObject.h>
#include <RenderProxy/LightSceneProxy.h>

namespace PigeonEngine
{

	class EVertexShaderAsset;
	class EPixelShaderAsset;
	class RViewProxy;

	class RViewLightCommonMaterialParameter : public EMaterialParameter
	{
	public:
		PE_CONSTEXPR_STATIC		UINT32		DirectionalLightParameterIndex	= 0u;
		PE_CONSTEXPR_STATIC		UINT32		PointLightParameterIndex		= 1u;
		PE_CONSTEXPR_STATIC		UINT32		SpotLightParameterIndex			= 2u;

		PE_CONSTEXPR_STATIC		UINT32		RenderDirectionalLightNumMax	= 4u;
		PE_CONSTEXPR_STATIC		UINT32		RenderPointLightNumMax			= 16u;
		PE_CONSTEXPR_STATIC		UINT32		RenderSpotLightNumMax			= 8u;
	public:
		virtual void	SetupParameters()override;
		void			UpdateRenderResource(const UINT32 InDirectionalLightNum, const UINT32 InPointLightNum, const UINT32 InSpotLightNum);
	public:
		void			SetLightNum(const UINT32 InDirectionalLightNum, const UINT32 InPointLightNum, const UINT32 InSpotLightNum) { DirectionalLightNum = InDirectionalLightNum; PointLightNum = InPointLightNum; SpotLightNum = InSpotLightNum; }
		UINT32			GetDirectionalLightNum()const { return DirectionalLightNum; }
		UINT32			GetPointLightNum()const { return PointLightNum; }
		UINT32			GetSpotLightNum()const { return SpotLightNum; }
	protected:
		UINT32	DirectionalLightNum;
		UINT32	PointLightNum;
		UINT32	SpotLightNum;
	public:
		RViewLightCommonMaterialParameter() : DirectionalLightNum(0u), PointLightNum(0u), SpotLightNum(0u) {}
		virtual ~RViewLightCommonMaterialParameter() {}
		RViewLightCommonMaterialParameter(const RViewLightCommonMaterialParameter& Other) : DirectionalLightNum(0u), PointLightNum(0u), SpotLightNum(0u) {}
		RViewLightCommonMaterialParameter& operator=(const RViewLightCommonMaterialParameter& Other) { DirectionalLightNum = 0u; PointLightNum = 0u; SpotLightNum = 0u; return (*this); }
	};

	class RSceneRenderer : public EManagerBase
	{
	public:
		using RVisibilityMapType		= TMap<ObjectIdentityType, BOOL32>;
		using RViewSceneTextureType		= TMap<ObjectIdentityType, RSceneTextures*>;
		using RShadowMapType			= TMap<ObjectIdentityType, RShadowTexture*>;
		using RViewShadowMapType		= TMap<ObjectIdentityType, RShadowMapType>;
		using RLightCommonParamsType	= TMap<ObjectIdentityType, RViewLightCommonMaterialParameter>;
		using RDLightParamsType			= TMap<ObjectIdentityType, RDirectionalLightMaterialParameter>;
	public:
		virtual void	Initialize()override;
		virtual void	ShutDown()override;
	public:
		RScene*			GetRenderScene();
		void			InitNewFrame();
		void			Render();
	protected:
		void			InitViews();
		void			BasePass();
		void			FinalOutputPass();
	protected:
		void			RenderBasePass(RSceneTextures* InSceneTextures);
		void			RenderLighting(const RViewProxy* InViewProxy, RSceneTextures* InSceneTextures);
		void			RenderSky(const RViewProxy* InViewProxy);
		void			RenderForward(const RViewProxy* InViewProxy, RSceneTextures* InSceneTextures);
	protected:
		void			InitLights(RViewProxy* InViewProxy);
		void			ProcessOcclusionCull(RViewProxy* InViewProxy);
		void			InitRenderPasses(RViewProxy* InViewProxy);
	protected:
		UINT32			InitDirectionalLights(RViewProxy* InViewProxy);
		void			InitShadowTextures(RViewProxy* InViewProxy);
		void			OctreeCull(const EFrustum& InViewFrustum, RVisibilityMapType& InOutVisibilityMap);
		void			PrimitiveCull(const EFrustum& InViewFrustum, RVisibilityMapType& InOutVisibilityMap);
	protected:
		void			InitRendererSettings();
		BOOL32			IsNeedStencil()const;
	public:
		enum RSamplerType : UINT8
		{
			SAMPLER_TYPE_POINT_CLAMP	= 0,
			SAMPLER_TYPE_POINT_WRAP,
			SAMPLER_TYPE_LINEAR_CLAMP,
			SAMPLER_TYPE_LINEAR_WRAP,
			SAMPLER_TYPE_COUNT
		};
		// Pre-built pipeline states. Each one bundles (raster + blend + depth-
		// stencil + render-target formats + topology) for a specific render
		// pass. PSOs marked "(env)" leave VS/PS empty; the mesh proxy that
		// runs in that pass binds its own shaders. PSOs marked "(full)"
		// include shaders too because the pass is fixed-function (full-screen
		// lighting / final blit).
		enum RPipelineStateType : UINT8
		{
			PIPELINE_STATE_BASE_PASS_MRT			= 0,	// (env) Static / skeletal mesh into 4x GBuffer + DSV
			PIPELINE_STATE_BASE_PASS_GRASS_SOLID,			// (env) BezierGrass solid (cull NONE)
			PIPELINE_STATE_BASE_PASS_GRASS_WIREFRAME,		// (env) BezierGrass wireframe
			PIPELINE_STATE_LIGHTING,						// (full) Full-screen lighting
			PIPELINE_STATE_SKY,								// (env) Sky (cull FRONT, depth LESS_EQUAL)
			PIPELINE_STATE_FORWARD,							// (env) Forward / debug primitives
			PIPELINE_STATE_FINAL_OUTPUT,					// (full) Full-screen blit to back buffer
			PIPELINE_STATE_COUNT
		};
	protected:
		RScene*						Scene;
		RFullScreenTriangle			FullScreenTriangle;
#if _EDITOR_ONLY
		RDebugWireframePrimitiveManager* DebugWireframePrimitiveManager;
#endif
	protected:
		RSamplerResource			Samplers[RSamplerType::SAMPLER_TYPE_COUNT];
		IRRHIPipelineState*			PipelineStates[RPipelineStateType::PIPELINE_STATE_COUNT];
		// Active command list for the frame currently being recorded.
		// Acquired in Render() and submitted at the end; sub-passes share it.
		IRCommandList*				CurrentCommandList;
		const EVertexShaderAsset*	SimpleFullScreenVertexShader;
		const EPixelShaderAsset*	SimpleFullScreenPixelShader;
		const EPixelShaderAsset*	SceneLightingPixelShader;
	protected:
		ObjectIdentityType			FinalOutputView;
		BOOL32						NeedStencil;
		RViewSceneTextureType		ViewSceneTextures;
		RViewShadowMapType			ViewShadowMaps;
		RLightCommonParamsType		ViewLightCommonParams;
		RDLightParamsType			ViewDLightParams;
	public:
		RSceneRenderer();
		virtual ~RSceneRenderer();
	public:
		RSceneRenderer(const RSceneRenderer&) = delete;
		RSceneRenderer& operator=(const RSceneRenderer&) = delete;
	};

};