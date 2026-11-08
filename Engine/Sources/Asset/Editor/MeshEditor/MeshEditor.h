#pragma once

#if ENGINE_EDITOR_ONLY

#include <Base/DataStructure/BuiltInType.h>
#include <Base/DataStructure/Container/Array.h>
#include <Base/DataStructure/Text/String.h>

#include "../PreviewScene/AssetPreviewScene.h"

namespace PigeonEngine
{
	class EStaticMeshAsset;
	class ESkinnedMeshAsset;
	class ESkeletonAsset;
	class EMesh;

	class EMeshEditor
	{
	public:
		void EditorInit();
		void EditorUpdate();
		void OpenMeshAsset(const EString& InAssetFullPath);

	private:
		void ResetLoadedAsset();
		void RefreshPreviewSkeletonOptions();
		void UpdatePreviewScene();
		void DrawMeshSummary() const;
		void DrawIndexSection(const EMesh& InMesh) const;
		void DrawVertexSection(const EMesh& InMesh) const;
		void DrawSubmeshSection(const EMesh& InMesh) const;
		void DrawSkinnedMeshSection() const;

	private:
		BOOL8					bShowEditor = FALSE;
		const EStaticMeshAsset*	StaticMeshAsset = nullptr;
		const ESkinnedMeshAsset*	SkinnedMeshAsset = nullptr;
		const EMesh*			MeshResource = nullptr;
		UINT32					MeshType = static_cast<UINT32>(-1);
		EString					OpenedAssetFullPath;
		EString					LastOpenError;
		EAssetPreviewScene		PreviewScene;
		TArray<EString>			AvailableSkeletonAssetPaths;
		const ESkeletonAsset*	PreviewSkeletonAsset = nullptr;
		INT32					SelectedPreviewSkeletonIndex = -1;
		BOOL8					bPreviewAutoRotate = TRUE;
	};
}

#endif