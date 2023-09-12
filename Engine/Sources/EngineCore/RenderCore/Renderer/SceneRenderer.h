#pragma once

#include <CoreMinimal.h>
#include "RenderScene.h"
#include <RenderResource.h>
#include <RenderProxy/RenderSingletonObject.h>

namespace PigeonEngine
{

	class RViewProxy;

	class RSceneTextures final
	{
	public:
		void	InitResources(const Vector2Int& InTextureSize);
		void	ClearResources();
		void	ReleaseResources();
	public:
		RRenderTexture2D	SceneDepthStencil;
		RRenderTexture2D	SceneColor;
		RRenderTexture2D	GBufferA;
		RRenderTexture2D	GBufferB;
		RRenderTexture2D	GBufferC;
	private:
		Vector2Int	TextureSize;
	public:
		RSceneTextures();
		~RSceneTextures();
		RSceneTextures(const RSceneTextures&) = delete;
		RSceneTextures& operator=(const RSceneTextures&) = delete;
	};

	class RSceneRenderer : public EManagerBase
	{
	public:
		using RVisibilityMapType = TMap<ObjectIdentityType, BOOL32>;
	public:
		virtual void	Initialize()override;
		virtual void	ShutDown()override;
	public:
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
		void			OctreeCull(const EFrustum& InViewFrustum, RVisibilityMapType& InOutVisibilityMap);
		void			PrimitiveCull(const EFrustum& InViewFrustum, RVisibilityMapType& InOutVisibilityMap);
	protected:
		RScene*					Scene;
		TArray<RViewProxy*>		ViewProxies;
		RFullScreenTriangle		FullScreenTriangle;
		RSceneTextures			SceneTextures;

		CLASS_MANAGER_SINGLETON_BODY(RSceneRenderer)

	};

};