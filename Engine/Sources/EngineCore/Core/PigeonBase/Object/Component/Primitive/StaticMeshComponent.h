#pragma once

#include <CoreMinimal.h>
#include "MeshComponent.h"

namespace PigeonEngine
{

	class EStaticMeshAsset;

	/*
	* A static mesh primitive. Its material slots and its mesh handle live on the mesh base, and
	* everything the render side needs is reported through the primitive base's two report hooks,
	* so what is left here is the asset handle itself.
	*/
	class PStaticMeshComponent : public PMeshComponent
	{

		CLASS_VIRTUAL_NOCOPY_BODY(PStaticMeshComponent)

	public:
		const EStaticMeshAsset*     GetMeshAsset()const;
		void                        SetMeshAsset(const EStaticMeshAsset* InMeshAsset);
		/*
		* Game thread: binds a mesh to this component without waiting for it. The asset is asked for now,
		* so one that is already loaded is applied before this call returns and an unloaded one is
		* retried every frame until it exists. InImportSource is the editor's raw source, used by the
		* load thread when the PAsset has never been imported.
		*/
		void                        RequestMeshAsset(const EString& InAssetName, const FAssetImportSource* InImportSource = nullptr);
		// The name the render side builds this component's proxy under. It has to match the one
		// RStaticMeshSceneProxy registers itself with.
		PE_NODISCARD virtual const EName& GetRenderProxyTypeName() const override;

		// Render proxy functions START
		// The registration and the update of this primitive, driven by the scene's generic
		// primitive path: the component states what it is and the scene builds the proxy on the
		// render thread, so there is no proxy pointer to keep here.
	public:
		virtual void    CreateRenderState()override;
		virtual void    DestroyRenderState()override;
		virtual void    SendUpdateRenderState()override;
		// Render proxy functions END
	protected:
		const EStaticMeshAsset*     MeshAsset;
	};

};
