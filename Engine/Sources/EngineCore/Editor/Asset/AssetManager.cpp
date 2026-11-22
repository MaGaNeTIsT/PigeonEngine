#include <Editor/Asset/AssetManager.h>

#if ENGINE_EDITOR_ONLY

#include <Editor/Asset/AssetIconRegistry.h>
#include <Editor/Asset/ContentBrowserPanel.h>
#include <Editor/AssetImporter/ImportManager.h>
#include <Editor/AnimationEditorCore/AnimationWorkspace.h>
#include <Editor/Manager/EditorWindowRegistry.h>
#include <Editor/MeshEditor/MeshEditor.h>

namespace PigeonEngine
{
	static void RegisterClassTypes()
	{
		RegisterClassType<EAssetManager, EManagerBase>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	EAssetManager::EAssetManager()
	{
	}

	EAssetManager::~EAssetManager()
	{
	}

	void EAssetManager::Initialize()
	{
	}

	void EAssetManager::ShutDown()
	{
		if (BrowserPanel)
		{
			BrowserPanel->ShutDown();
			delete BrowserPanel;
			BrowserPanel = nullptr;
		}
		if (AnimationWorkspace)
		{
			AnimationWorkspace->ShutDown();
			delete AnimationWorkspace;
			AnimationWorkspace = nullptr;
		}
		if (MeshEditor)
		{
			delete MeshEditor;
			MeshEditor = nullptr;
		}
		if (Icons)
		{
			Icons->ShutDown();
			delete Icons;
			Icons = nullptr;
		}
		Catalog.ShutDown();
	}

	void EAssetManager::Init()
	{
	}

	void EAssetManager::Uninit()
	{
	}

	void EAssetManager::EditorInit()
	{
		if (!AnimationWorkspace)
		{
			AnimationWorkspace = new EAnimationWorkspace();
			AnimationWorkspace->EditorInit();
		}
		if (!MeshEditor)
		{
			MeshEditor = new EMeshEditor();
			MeshEditor->EditorInit();
		}

		Catalog.Initialize();

		if (!Icons)
		{
			Icons = new FAssetIconRegistry();
		}
		Icons->Load();

		if (!BrowserPanel)
		{
			BrowserPanel = new FContentBrowserPanel();
		}
		BrowserPanel->Initialize(this);
	}

	void EAssetManager::EditorUpdate()
	{
		// A refresh request may come from an importer (through EImportManager), from the
		// panel's explicit Refresh, or from a family manager that wrote an asset. All are
		// consumed here, before anything draws, so the tree is never rebuilt while the
		// panel iterates it.
		const BOOL32 bImportRefreshRequested = EImportManager::GetManagerSingleton()->ConsumeContentBrowserRefreshRequest();
		const BOOL32 bCatalogRefreshRequested = Catalog.ConsumeRefreshRequest();
		if ((bImportRefreshRequested != FALSE) || (bCatalogRefreshRequested != FALSE))
		{
			Catalog.Refresh();
			if (BrowserPanel)
			{
				BrowserPanel->PruneSelectionAfterCatalogRefresh();
			}
		}

		if (BrowserPanel)
		{
			EEditorWindowRegistry::DrawWindow(EString(PE_TEXT("ContentBrowser")));
		}
		if (AnimationWorkspace)
		{
			AnimationWorkspace->EditorUpdate();
		}
		if (MeshEditor)
		{
			MeshEditor->EditorUpdate();
		}
	}

	void EAssetManager::DrawContentBrowserPanel()
	{
		if (BrowserPanel)
		{
			BrowserPanel->DrawContents();
		}
	}

	void EAssetManager::OpenSkeletonSlotGroups(const EString& InAssetFullPath)
	{
		if ((AnimationWorkspace) && (InAssetFullPath.Length() > 0u))
		{
			AnimationWorkspace->OpenSlotGroupAsset(InAssetFullPath);
		}
	}

	void EAssetManager::RequestContentRefresh()
	{
		Catalog.RequestRefresh();
	}

	FAssetCatalog& EAssetManager::GetCatalog()
	{
		return Catalog;
	}

	const FAssetCatalog& EAssetManager::GetCatalog() const
	{
		return Catalog;
	}

	FAssetIconRegistry& EAssetManager::GetIcons()
	{
		return *Icons;
	}

	EAnimationWorkspace* EAssetManager::GetAnimationWorkspace() const
	{
		return AnimationWorkspace;
	}

	EMeshEditor* EAssetManager::GetMeshEditor() const
	{
		return MeshEditor;
	}
}

#endif
