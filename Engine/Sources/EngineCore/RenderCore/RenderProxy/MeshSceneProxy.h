#pragma once

#include <CoreMinimal.h>
#include <RenderCommon.h>
#include "PrimitiveSceneProxy.h"
#include <MeshAsset/MeshAsset.h>

namespace PigeonEngine
{

	class EShaderInstance;

	/*
	* The engine fallback material of a mesh: what a slot with no material assignment resolves
	* to, so geometry is never dropped merely because nothing was authored.
	*
	* The cache is keyed on the vertex axis the mesh resolves, because the axis shapes the
	* varying struct and one instance cannot serve two axes. Static and skinned meshes share
	* the asset and differ only in variant, which is what the unified engine mesh factory is
	* for. The instances live for the process on purpose: components only hold non owning
	* pointers, so they have to outlive every component that uses them.
	*/
	PE_NODISCARD extern const EShaderInstance*	GetFallbackMeshShader(const EMesh* InMesh);

	/*
	* What the mesh primitives share on the render side: the fallback material, the vertex and
	* index streams resolved from the shader's own input layout, and the submesh walk.
	*
	* A concrete mesh kind supplies only the asset it resolved (two accessors) and, when its
	* vertex data is skinned, the two extra streams and the flag that makes the pipeline state
	* declare the index stream's real format. Both hooks default to "no mesh", so a mesh proxy
	* that has not been migrated yet draws nothing rather than drawing wrongly.
	*/
	class RMeshSceneProxy : public RPrimitiveSceneProxy
	{
	protected:
		// The two pieces only a concrete mesh kind can supply. The shared draw path needs
		// nothing else from the asset.
		PE_NODISCARD virtual const EMesh*				GetMeshColdData()const;
		PE_NODISCARD virtual const EMeshRenderResource*	GetMeshRenderResource()const;
		// The skin streams, or null for a mesh without them. InIsIndexStream selects the blend
		// index stream, otherwise the blend weight stream.
		PE_NODISCARD virtual const RVertexBufferResource*	GetSkinVertexStream(const BOOL32 InIsIndexStream)const;
		// TRUE when this mesh carries the two skin streams. It is what decides whether the
		// pipeline state has to declare the index stream's data format.
		PE_NODISCARD virtual BOOL32	HasSkinVertexStreams()const;
	protected:
		virtual const EShaderInstance*	ResolveFallbackInstance(const UINT32 InSlotIndex)const override;
		virtual const RInputLayoutDesc*	GetPipelineStateInputLayouts(UINT32& OutCount)const override;
		virtual UINT32	GetDrawGroupNum()const override;
		virtual BOOL32	GetDrawGroup(const UINT32 InGroupIndex, UINT32& OutSlotIndex, UINT32& OutIndexCount, UINT32& OutStartIndex)const override;
		virtual BOOL32	BindGeometry(IRCommandList* InCommandList, const RShaderPassSlot& InSlot)const override;

		RENDER_PROXY_CLASS_BODY(RMeshSceneProxy)

	};

	extern EVertexResourceType TranslateInputLayoutToVertexResource(const RInputLayoutDesc& InLayout, UINT32& OutSlot);

};
