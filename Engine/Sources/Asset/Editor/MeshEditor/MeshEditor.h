#pragma once

#if _EDITOR_ONLY

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
		BOOL8					m_ShowEditor = FALSE;
		const EStaticMeshAsset*	m_StaticMeshAsset = nullptr;
		const ESkinnedMeshAsset*	m_SkinnedMeshAsset = nullptr;
		const EMesh*			m_MeshResource = nullptr;
		UINT32					m_MeshType = static_cast<UINT32>(-1);
		EString					m_OpenedAssetFullPath;
		EString					m_LastOpenError;
		EAssetPreviewScene		m_PreviewScene;
		TArray<EString>			m_AvailableSkeletonAssetPaths;
		const ESkeletonAsset*	m_PreviewSkeletonAsset = nullptr;
		INT32					m_SelectedPreviewSkeletonIndex = -1;
		BOOL8					m_PreviewAutoRotate = TRUE;
	};
}

#endif