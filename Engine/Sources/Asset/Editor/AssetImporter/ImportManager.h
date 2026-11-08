#include <CoreMinimal.h>
#include <Asset/BaseAsset.h>
#include <Base/DataStructure/ObjectBase.h>
#include <Base/DataStructure/Container/Array.h>
#include <Base/DataStructure/Container/Map.h>
#include <Base/DataStructure/Pointer/SharedPtr.h>
#if ENGINE_EDITOR_ONLY
#include "IImporter.h"
#include <imgui.h>
namespace PigeonEngine
{
	class EImportManager : public EManagerBase
	{
		CLASS_MANAGER_VIRTUAL_SINGLETON_BODY(EImportManager)
	public:
		virtual void Initialize();
		virtual void ShutDown();
		virtual void ShowImporterButton();
	public:
		void EditorInit();
	public:
		void EditorUpdate();
		// Marks the content browser's folder tree as stale. Importers call this after writing
		// their assets; multiple calls in one frame coalesce into one rescan. Game thread only:
		// the importer UI and the content browser both run there.
		void NotifyImportFinished();
		// Consumed by the content browser on the next editor frame.
		PE_NODISCARD BOOL32 ConsumeContentBrowserRefreshRequest();
		/// <summary>
		/// Add a Importer to handle file import,one importer one file extension.
		/// </summary>
		/// <param name="FileExtension">File extension that importer could handle with.</param>
		/// <param name="Importer">Importer</param>
		void AddImporter(EString FileExtension,IImporter* Importer);
	private:
		void ShowEditor();
	private:
		TMap<EString, IImporter*> Importers;
		BOOL8 bShowEditor = FALSE;
		BOOL32 bContentBrowserRefreshRequested = FALSE;
	};
};
#endif
