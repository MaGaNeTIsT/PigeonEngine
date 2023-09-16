#pragma once

#include <CoreMinimal.h>
#include "RenderScene.h"
#include <RenderResource.h>
#include <RenderProxy/RenderSingletonObject.h>

namespace PigeonEngine
{

	class EVertexShaderAsset;
	class EPixelShaderAsset;
	class RViewProxy;

	class RSceneRenderer : public EManagerBase
	{
	public:
		using RVisibilityMapType	= TMap<ObjectIdentityType, BOOL32>;
		using RViewSceneTextureType	= TMap<ObjectIdentityType, RSceneTextures*>;
		using RShadowMapType		= TMap<ObjectIdentityType, RShadowTexture*>;
		using RViewShadowMapType	= TMap<ObjectIdentityType, RShadowMapType>;
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
		void			InitLights(RViewProxy* InViewProxy);
		void			ProcessOcclusionCull(RViewProxy* InViewProxy);
		void			InitRenderPasses(RViewProxy* InViewProxy);
	protected:
		void			InitDirectionalLights(RViewProxy* InViewProxy);
		void			InitShadowTextures(RViewProxy* InViewProxy);
		void			OctreeCull(const EFrustum& InViewFrustum, RVisibilityMapType& InOutVisibilityMap);
		void			PrimitiveCull(const EFrustum& InViewFrustum, RVisibilityMapType& InOutVisibilityMap);
	protected:
		void			InitRendererSettings();
		BOOL32			IsNeedStencil()const;
	public:
		enum RDefaultSamplerType : UINT8
		{
			SAMPLER_TYPE_POINT_CLAMP	= 0,
			SAMPLER_TYPE_POINT_WRAP,
			SAMPLER_TYPE_LINEAR_CLAMP,
			SAMPLER_TYPE_LINEAR_WRAP,
			SAMPLER_TYPE_COUNT
		};
	protected:
		RScene*						Scene;
		RFullScreenTriangle			FullScreenTriangle;
	protected:
		RSamplerResource			DefaultSamplers[RDefaultSamplerType::SAMPLER_TYPE_COUNT];
		const EVertexShaderAsset*	SimpleFullScreenVertexShader;
		const EPixelShaderAsset*	SimpleFullScreenPixelShader;
	protected:
		ObjectIdentityType			FinalOutputView;
		BOOL32						NeedStencil;
		RViewSceneTextureType		ViewSceneTextures;
		RViewShadowMapType			ViewShadowMaps;
	public:
		RSceneRenderer();
		virtual ~RSceneRenderer();
	public:
		RSceneRenderer(const RSceneRenderer&) = delete;
		RSceneRenderer& operator=(const RSceneRenderer&) = delete;
	};

};