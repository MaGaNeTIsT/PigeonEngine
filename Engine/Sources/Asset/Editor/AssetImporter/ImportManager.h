#include <CoreMinimal.h>
#include <Asset/BaseAsset.h>
#include <Base/DataStructure/ObjectBase.h>
#include <Base/DataStructure/Container/Array.h>
#include <Base/DataStructure/Container/Map.h>
#include <Base/DataStructure/Pointer/SharedPtr.h>
#if _EDITOR_ONLY
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
		/// <summary>
		/// Add a Importer to handle file import,one importer one file extension.
		/// </summary>
		/// <param name="FileExtension">File extension that importer could handle with.</param>
		/// <param name="Importer">Importer</param>
		void AddImporter(EString FileExtension,IImporter* Importer);
	private:
		const CHAR* GetFileFilterList();
	private:
		TMap<EString, IImporter*> m_Importers;
	};
};
#endif