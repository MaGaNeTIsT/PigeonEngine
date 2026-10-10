#pragma once

#include <CoreMinimal.h>
#include "MeshComponent.h"
#include <SkeletonAsset/SkeletonAsset.h>

namespace PigeonEngine
{

	class ESkinnedMeshAsset;
	class ESkeletonAsset;

	/*
	* A skinned mesh primitive. Its material slots and its mesh handle live on the mesh base;
	* what is left here is the mesh and skeleton handles, the bone pool the animation writes
	* into, and the reporting that turns those into a snapshot.
	*/
	class PSkeletalMeshComponent : public PMeshComponent
	{

		CLASS_VIRTUAL_NOCOPY_BODY(PSkeletalMeshComponent)

	public:
		const ESkinnedMeshAsset*        GetMeshAsset()const;
		void                            SetMeshAsset(const ESkinnedMeshAsset* InMeshAsset);
		const ESkeletonAsset*           GetSkeletonAsset()const;
		void                            SetSkeletonAsset(const ESkeletonAsset* InSkeletonAsset);
		const ESkeletonBoneMemoryPool&  GetBoneMemoryPool()const;
		ESkeletonBoneMemoryPool&        GetBoneMemoryPool();
		// Call this after writing the bone matrices, which is what the animation update does.
		// Nothing else can see that they moved, so without this call the render side keeps the
		// previous pose.
		void                            MarkBoneMemoryPoolDirty();
		/*
		* Game thread: binds a skinned mesh and its skeleton to this component without waiting for them.
		* Both are asked for now - an already loaded pair is applied before this call returns - and the
		* binding is retried every frame until both exist, because the mesh's skin stream is indexed
		* against the skeleton and a half applied pair could not be drawn anyway. The bone pool is
		* rebuilt from the skeleton by SetSkeletonAsset, which is why the mesh goes in first.
		*/
		void                            RequestSkeletalMeshAssets(const EString& InMeshAssetName, const EString& InSkeletonAssetName
			, const FAssetImportSource* InImportSource = nullptr);
	protected:
		const ESkinnedMeshAsset*        MeshAsset;
		const ESkeletonAsset*           SkeletonAsset;
		ESkeletonBoneMemoryPool         BoneMemoryPool;

		// What this primitive reports to the render tier START
	public:
		// The name the render side builds this component's proxy under. It has to match the one
		// RSkeletalMeshSceneProxy registers itself with.
		PE_NODISCARD virtual const EName& GetRenderProxyTypeName() const override;
		// The mesh base reports the mesh and the material slots; a skinned primitive adds its
		// skeleton.
		virtual void CollectRenderAssets(FPrimitiveRenderAssetReport& InOutReport) const override;
		// The base fills identity, transform, flags and the two reports. A skinned primitive
		// adds the one thing that is neither an asset nor a shader value: where its bones are.
		PE_NODISCARD virtual BOOL32 BuildRenderSnapshot(FPrimitiveRenderSnapshot& OutSnapshot) const override;
		// What this primitive reports to the render tier END

	protected:
		virtual void TickRenderImpl() override;

		// Render proxy functions START
	public:
		virtual void                CreateRenderState()override;
		virtual void                DestroyRenderState()override;
		virtual void                SendUpdateRenderState()override;
		// Render proxy functions END
	};

};
