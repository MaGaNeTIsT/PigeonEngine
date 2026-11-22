#pragma once

#include "Base/DataStructure/ObjectBase.h"

#if ENGINE_EDITOR_ONLY

#include "Base/DataStructure/Container/Array.h"
#include <Editor/Asset/AssetCatalog.h>

namespace PigeonEngine
{
	class EMeshEditor;
	class EAnimationWorkspace;
	class FAssetIconRegistry;
	class FContentBrowserPanel;

	/*
	* Host for the editor's asset tooling: the filesystem catalog, the editor icon table,
	* the animation/mesh document editors, and the Content Browser panel. The browsing UI
	* lives in FContentBrowserPanel; this class only owns lifetime and routes refresh and
	* open requests.
	*/
	class EAssetManager : public EManagerBase
	{
		CLASS_MANAGER_VIRTUAL_SINGLETON_BODY(EAssetManager)
	public:
		void Initialize() override;
		void ShutDown() override;

		void Init();
		void Uninit();
	public:
		void EditorInit();
		void EditorUpdate();
		// Content of the Content Browser window. The window registry owns Begin/End.
		void DrawContentBrowserPanel();
		// Opens the skeleton's SlotGroup document in the animation workspace. Montage
		// sections resolve their slot against this skeleton-owned order, so the montage
		// editor links here instead of maintaining a second group list.
		void OpenSkeletonSlotGroups(const EString& InAssetFullPath);

	public:
		// Requests a catalog rescan for the next editor frame, so several writes in one
		// frame coalesce into a single rebuild of the tree.
		void RequestContentRefresh();

	public:
		PE_NODISCARD FAssetCatalog& GetCatalog();
		PE_NODISCARD const FAssetCatalog& GetCatalog() const;
		PE_NODISCARD FAssetIconRegistry& GetIcons();
		PE_NODISCARD EAnimationWorkspace* GetAnimationWorkspace() const;
		PE_NODISCARD EMeshEditor* GetMeshEditor() const;

	private:
		FAssetCatalog			Catalog;
		FAssetIconRegistry*		Icons = nullptr;
		FContentBrowserPanel*	BrowserPanel = nullptr;
		EAnimationWorkspace*	AnimationWorkspace = nullptr;
		EMeshEditor*			MeshEditor = nullptr;
	};
}

#endif
