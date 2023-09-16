#pragma once

#include <CoreMinimal.h>
#include "RenderScene.h"
#include <RenderResource.h>
#include <RenderProxy/RenderSingletonObject.h>

namespace PigeonEngine
{

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
	protected:
		RScene*					Scene;
		TArray<RViewProxy*>		ViewProxies;
		RFullScreenTriangle		FullScreenTriangle;
	protected:
		BOOL32					NeedStencil;
		RViewSceneTextureType	ViewSceneTextures;
		RViewShadowMapType		ViewShadowMaps;

		CLASS_MANAGER_SINGLETON_BODY(RSceneRenderer)

	};

};