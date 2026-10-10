#pragma once

#include <CoreMinimal.h>
#include "MeshSceneProxy.h"
#include <PigeonBase/Object/Component/Primitive/StaticMeshComponent.h>

namespace PigeonEngine
{

	class EStaticMeshAsset;

	/*
	* The static mesh primitive. Everything it does not state here comes from the mesh base
	* and the primitive base: the fallback material, the vertex and index streams resolved
	* from the shader's input layout, the submesh walk, the engine blocks and the draw
	* sequence. What is left is the one asset handle it resolved from the report.
	*/
	class RStaticMeshSceneProxy : public RMeshSceneProxy
	{
	public:
		RStaticMeshSceneProxy(const PStaticMeshComponent* InComponent);
	public:
		virtual BOOL32	IsRenderValid()const override;
	public:
		// Installs the reported assets and caches the typed handle the mesh hooks answer with.
		// The base half stores the report itself and rebuilds the pass slots.
		virtual void	ApplyAssets(const FPrimitiveRenderAssetReport& InAssets) override;
	protected:
		virtual const EMesh*				GetMeshColdData()const override;
		virtual const EMeshRenderResource*	GetMeshRenderResource()const override;
	protected:
		const EStaticMeshAsset*	MeshAsset;

		RENDER_PROXY_CLASS_BODY(RStaticMeshSceneProxy)

	};

};
