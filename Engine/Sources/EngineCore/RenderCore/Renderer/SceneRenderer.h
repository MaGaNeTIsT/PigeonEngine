#pragma once

#include <CoreMinimal.h>
#include "RenderScene.h"
#include <RenderResource.h>
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
		enum RRasterizerType : UINT8
		{
			RASTERIZER_TYPE_WIREFRAME	= 0,
			RASTERIZER_TYPE_SOLID_NONE,
			RASTERIZER_TYPE_SOLID_BACK,
			RASTERIZER_TYPE_SOLID_FRONT,
			RASTERIZER_TYPE_COUNT
		};
		enum RBlendType : UINT8
		{
			BLEND_TYPE_BLEND_OFF		= 0,
			BLEND_TYPE_OPAQUE_BASEPASS,
			BLEND_TYPE_LIGHTING,
			BLEND_TYPE_COUNT
		};
		enum RDepthStencilType : UINT8
		{
			DEPTH_STENCIL_TYPE_DEPTH_NOP_STENCIL_NOP = 0,
			DEPTH_STENCIL_TYPE_DEPTH_LESS_STENCIL_NOP,
			DEPTH_STENCIL_TYPE_DEPTH_LESS_EQUAL_STENCIL_NOP,
			DEPTH_STENCIL_TYPE_DEPTH_EQUAL_STENCIL_NOP,
			DEPTH_STENCIL_TYPE_COUNT
		};
	protected:
		RScene*						Scene;
		RFullScreenTriangle			FullScreenTriangle;
	protected:
		RSamplerResource			Samplers[RSamplerType::SAMPLER_TYPE_COUNT];
		RRasterizerResource			Rasterizer[RRasterizerType::RASTERIZER_TYPE_COUNT];
		RBlendResource				Blend[RBlendType::BLEND_TYPE_COUNT];
		RDepthStencilResource		DepthStencil[RDepthStencilType::DEPTH_STENCIL_TYPE_COUNT];
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