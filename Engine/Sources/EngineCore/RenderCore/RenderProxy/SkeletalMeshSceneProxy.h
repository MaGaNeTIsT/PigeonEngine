#pragma once

#include <CoreMinimal.h>
#include "MeshSceneProxy.h"
#include <SkeletonAsset/SkeletonAsset.h>
#include <PigeonBase/Object/Component/Primitive/SkeletalMeshComponent.h>

namespace PigeonEngine
{

	class ESkinnedMeshAsset;
	class ESkeletonAsset;

	/*
	* The skinned mesh primitive. Everything it does not state here comes from the mesh base and
	* the primitive base: the fallback material, the index buffer and the vertex streams resolved
	* from the shader's input layout, the submesh walk, the engine blocks and the draw sequence.
	*
	* What is left is the two asset handles it resolved from the report, the skin streams it adds
	* to the shared vertex binding, and the bones - the one kind of state that is neither an asset
	* nor a shader value, which is why it travels as snapshot payload.
	*/
	class RSkeletalMeshSceneProxy : public RMeshSceneProxy
	{
	public:
		RSkeletalMeshSceneProxy(const PSkeletalMeshComponent* InComponent);
	public:
		virtual BOOL32	IsRenderValid() const override;
		// The cull bound of a deformed mesh.
		//
		// By default this is the mesh asset's authored bound: the imported bind pose bound grown
		// by the asset's BoundExtend and BoundScale. The pair lives on the asset, so every
		// component referencing the mesh shares one conservative volume; it is the artist's
		// statement that no animation leaves it, which is what keeps the per bone cost off the
		// frame.
		//
		// ENGINE_SKELETAL_BOUND_FROM_BONES turns the measured variant on for validation: that
		// one unions every bone's skinned copy of the bound in mesh space, which is conservative
		// for linear blend skinning because a skinned vertex is a convex combination of the
		// bind pose corners under its bones, but costs one transform per bone.
		virtual BOOL32	GetWorldBound(Vector3& OutMin, Vector3& OutMax) const override;
	public:
		// Caches the typed handles the mesh hooks answer with, and sizes the bone buffers from
		// the mesh's bind pose count, which is an asset level fact rather than a per frame one.
		virtual void	ApplyAssets(const FPrimitiveRenderAssetReport& InAssets) override;
		// Recomputes the bone matrices from the reported bone to root matrices and uploads them.
		void			UpdateSkeletonRenderResource(const TArray<Matrix4x4>& InBoneToRootMatrices);
	protected:
		// Rebuilds SkinnedLocalBound from the mesh's adjusted bound and the current skinning
		// matrices. Only called when ENGINE_SKELETAL_BOUND_FROM_BONES is on.
		void			UpdateSkinnedLocalBoundFromBones(const ESkinnedMesh& InMesh);
	protected:
		// The two asset handles the shared mesh draw path needs, plus the skin pair it binds in
		// addition to the attribute streams.
		virtual const EMesh*				GetMeshColdData() const override;
		virtual const EMeshRenderResource*	GetMeshRenderResource() const override;
		virtual const RVertexBufferResource*	GetSkinVertexStream(const BOOL32 InIsIndexStream) const override;
		virtual BOOL32	HasSkinVertexStreams() const override;
		// The bones are bound here, after the shared material and geometry bindings. They live in
		// this proxy's own parameter set rather than in the material's: two proxies may share one
		// material instance, and two skeletons sharing one bone buffer would overwrite each
		// other's matrices.
		virtual BOOL32	BindExtraResources(IRCommandList* InCommandList, const FPrimitivePassData& InData) const override;
		// The bone matrices arrive as snapshot payload rather than as an asset or a value.
		virtual void	ApplyRuntimePayload(const FPrimitiveRenderSnapshot& InSnapshot) override;
	protected:
		const ESkinnedMeshAsset*		MeshAsset;
		const ESkeletonAsset*			SkeletonAsset;
		ESkeletonRenderResource			SkeletonRenderResource;
		TArray<Matrix4x4>				BoneValues;
		// The mesh's cull bound in mesh space. The authored adjustment is what fills it; the
		// measured-from-bones variant replaces it only when ENGINE_SKELETAL_BOUND_FROM_BONES is
		// on. Keeping it here makes GetWorldBound eight corner transforms either way.
		EBoundAABB						SkinnedLocalBound;
		// The bone streams as named values.
		EShaderParameterSet				SkeletonParameters;

		RENDER_PROXY_CLASS_BODY(RSkeletalMeshSceneProxy)

	};

};
