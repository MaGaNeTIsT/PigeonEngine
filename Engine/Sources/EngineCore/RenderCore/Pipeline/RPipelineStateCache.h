#pragma once

#include <CoreMinimal.h>
#include <EngineCommon.h>
#include <Pipeline/RPipelineLayout.h>
#include <Pipeline/RPipelineStateDesc.h>

namespace PigeonEngine
{

	class EShaderAsset;
	class EShaderProgram;
	class IRCommandList;
	class IRRHIDevice;
	class IRRHIPipelineState;

	/*
	* RenderCore/Pipeline - the pipeline states a pass draws with.
	*
	* A caller describes what it wants (RGraphicsPipelineStateDesc) and gets a pipeline state
	* back; the desc names a program, never a root signature or a root index. The cache is what
	* turns one into the other: it asks the layout cache for the program's parameter layout,
	* hands the program's shaders and that signature to the device, and keys the result so the
	* same desc is created once.
	*
	* The cache owns what it created: InvalidateAll releases every pipeline state through the
	* device (which defers the actual destruction until the GPU is done with it). Callers only
	* ever hold non-owning pointers, which stay valid until the cache is invalidated.
	*
	* Lifetime contract: the cache has to be invalidated before the device shuts down, because
	* the states it releases live in the device. Its owner (the renderer that holds it) is the
	* one that can guarantee that order, so it is the owner's shutdown that calls InvalidateAll
	* first and tears the device down after.
	*/
	class RPipelineStateCache
	{
	public:
		RPipelineStateCache();
		~RPipelineStateCache();

		RPipelineStateCache(const RPipelineStateCache&) = delete;
		RPipelineStateCache& operator=(const RPipelineStateCache&) = delete;

	public:
		void	SetDevice(IRRHIDevice* InDevice);
		PE_NODISCARD IRRHIDevice*	GetDevice() const { return Device; }
		PE_NODISCARD RPipelineLayoutCache&			GetLayoutCache() { return LayoutCache; }
		PE_NODISCARD const RPipelineLayoutCache&	GetLayoutCache() const { return LayoutCache; }

		// Graphics pipeline for a desc. Null when the program is missing a stage, when its
		// layout cannot be generated, or when the back-end refused the create; the caller then
		// has nothing to bind and should skip the draw.
		PE_NODISCARD IRRHIPipelineState*	GetOrCreateGraphics(const RGraphicsPipelineStateDesc& InDesc);

		// Compute pipeline for a program. Same contract as the graphics entry point; a compute
		// program carries one stage, so there is no fixed-function state to decide.
		PE_NODISCARD IRRHIPipelineState*	GetOrCreateCompute(const EShaderProgram& InProgram);

		// Drops every pipeline state that was built for a program of this asset, which is what
		// the hot reload path calls once the asset's shaders changed. The layouts survive: they
		// are shared by hash, and a rebuilt program with the same reflection gets the same one
		// back. Programs are addressed by the asset name, which is how the compile layer builds
		// them (see ShaderAssetBinding).
		void	InvalidateForAsset(const EShaderAsset* InAsset);

		// The same drop, addressed by shader name. A reload publishes a NEW asset object for a
		// name, so the name is what survives it - and unlike the asset pointer above, a name needs
		// nothing alive to be read from.
		void	InvalidateForShaderName(const EName& InShaderName);

		// Releases every pipeline state this cache created. Signature objects stay with the
		// device, which shares them by layout hash.
		void	InvalidateAll();

		PE_NODISCARD UINT32	GetPipelineStateNum() const { return States.Num<UINT32>(); }

	private:
		// One cached pipeline state with the program it was built for, so invalidation can tell
		// which asset a state belongs to without walking the shaders again.
		struct FPipelineStateEntry
		{
			IRRHIPipelineState*		State	= nullptr;
			const EShaderProgram*	Program	= nullptr;
		};

		// Key of one graphics desc: the program and its layout identity plus the
		// fixed-function state. Everything that reaches the back-end takes part, so two
		// entries can never describe two different pipelines.
		PE_NODISCARD static UINT64	HashGraphicsDesc(const RGraphicsPipelineStateDesc& InDesc, const UINT64 InLayoutHash);

	private:
		IRRHIDevice*						Device;
		RPipelineLayoutCache				LayoutCache;
		TMap<UINT64, FPipelineStateEntry>	States;

	};

}	// PigeonEngine
