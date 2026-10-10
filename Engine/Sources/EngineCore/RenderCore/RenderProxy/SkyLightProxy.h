#pragma once

#include <CoreMinimal.h>
#include "BaseSceneProxy.h"
#include <Shader/ShaderParameter.h>
#include <Shader/ShaderEngineParameters.h>
#include <RenderProxy/RenderSingletonObject.h>
#include <PigeonBase/Object/Component/Primitive/SkyLightComponent.h>

namespace PigeonEngine
{

	class ETextureCubeAsset;
	class EStaticMeshAsset;
	class EShaderProgram;
	class IRCommandList;
	class IRRHIPipelineState;

	/*
	* The sky. It draws the engine's fullscreen triangle with the engine sky program, so unlike a
	* primitive it resolves no material: its program, its geometry and its per object block are
	* all engine owned, and the block is bound directly rather than through a material's
	* reflection.
	*
	* The geometry is the fullscreen triangle and not a dome mesh, because the program's vertex
	* stage is the fullscreen factory: it reconstructs its clip position from one UV coordinate
	* and declares no other attribute. A dome mesh here would bind five streams and its own index
	* count against a shader that reads one attribute.
	*
	* Every binding goes through IRCommandList, including the pipeline state. That state is
	* the one thing the sky has to build for itself: the pass it draws in is per primitive
	* and therefore carries no shaders, so the sky asks the frame's cache for a state made
	* of the pass's fixed function settings and its own program. The cache owns the result,
	* which is why nothing here releases it.
	*/
	class RSkyLightSceneProxy : public RBaseSceneProxy
	{
	public:
		RSkyLightSceneProxy(PSkyLightComponent* InComponent);
		void			SetupProxy(const ERenderSkyLightMatrices& InMatrices, const ERenderSkyLightParams& InSkyLightParams, const ETextureCubeAsset* InTextureCubeAsset);
	public:
		void			UpdateTextureCubeAsset(const ETextureCubeAsset* InTextureCubeAsset);
		void			UpdateSkyLightParams(const ERenderSkyLightParams& InSkyLightParams);
		void			UpdateMatrices(const ERenderSkyLightMatrices& InMatrices);
		void			UpdateRenderResource();
	public:
		// The environment cube, resolved down to the back-end texture. The indirect lighting bake
		// reads it, and the proxy is where an asset becomes a resource, so the renderer asks the
		// proxy rather than the component - which belongs to the game thread. Null while the sky
		// has no cubemap or its resource is not up yet.
		const IRRHITexture*	GetEnvironmentTexture() const noexcept;
		// The sky's tint and its mean luminance in cd/m2. The indirect lighting publishes them
		// beside the environment coefficients, and the sky pass multiplies the cube by the same
		// pair - so what a surface receives from the environment and what the camera sees of it
		// cannot disagree about how bright the sky is.
		const Color3&	GetAdjustColor() const noexcept;
		FLOAT			GetAdjustIntensity() const noexcept;
	public:
		BOOL32			IsRenderValid()const;
		// The renderer calls this before Draw. The dome binds everything inside Draw, right
		// after the pipeline state that would otherwise void it, so there is nothing to do
		// here; the hook stays because the renderer calls it first.
		void			BindRenderResource()const;
		void			Draw();
		/*
		* Render thread: the sky shader was replaced, so the process wide program is re-derived here
		* and every proxy drops the state it built from the previous one (each proxy notices by
		* comparing its own PipelineStateProgram against SkyProgram).
		*
		* It asks for the instance rather than resolving it: this runs inside a draw-side command,
		* where a load may not happen. The instance is the stable handle and was already re-pointed
		* to the new asset, so the ask is a cache hit.
		*/
		static void		OnShaderAssetReplaced();
	protected:
		// Resolves the sky's pipeline state on first use and keeps it for the following
		// frames. FALSE means there is nothing to draw with.
		BOOL32			EnsurePipelineState();
		// Drops what this proxy built from a program that has since been replaced: the pipeline
		// state and the parameter block both had their layout read from it. The program is process
		// wide (see SkyProgram), so the comparison is one pointer against one pointer.
		void			DropStateForReplacedProgram();
		// The engine's fullscreen triangle, which is the geometry this pass draws. The buffers
		// behind it are process wide statics, so an instance per sky costs nothing.
		RFullScreenTriangle			FullScreenTriangle;
		// Binds the environment cube at the register the sky program's pixel stage reports,
		// which is how the slot stays correct when the shader's reflection changes.
		void			BindEnvironmentCube(IRCommandList* InCommandList)const;
	protected:
		// The block the sky shader reads its own values from: the tint and the sky luminance in
		// cd/m2. Named at construction, like every other block object - the name is its identity
		// and an unnamed block could neither be initialized nor bound.
		//
		// This replaced a per instance constant block that declared a world matrix and the two
		// adjust values under underscored names. That block was written every frame and read by
		// nothing: no shader source declares those names, and the adjust values the sky actually
		// uses travel in this one.
		RShaderParameterBlock		SkyLightParameterBlock{ EName(SHADER_BLOCK_SKY_LIGHT_PARAMS) };
		const ETextureCubeAsset*	TextureCubeAsset;
		Color3						AdjustColor;
		FLOAT						AdjustIntensity;
		// Non-owning: the frame's pipeline state cache created it and invalidates it with the
		// rest at shutdown.
		IRRHIPipelineState*			PipelineState;
		// The program PipelineState was built from. A shader reload replaces it, and a pipeline
		// state carries the root signature of the program it was created for, so the pair has to
		// move together: a state whose program moved on is dropped rather than used.
		const EShaderProgram*		PipelineStateProgram = nullptr;
	protected:
		const PSkyLightComponent*	Component;
	protected:
		// Resolves the sky program once per process. Kept as a static because every sky light
		// draws the same program, and no sky light carries per instance state.
		static void		CreateSkyAssets();
	protected:
		// The engine sky program: resolved from the process wide sky shader instance.
		static const EShaderProgram*		SkyProgram;

		RENDER_PROXY_CLASS_BODY(RSkyLightSceneProxy)

	};

};
