#include "ImportManager.h"
#include "Asset/Editor/FileDialogHelper/FileDialogHelper.h"
#include <Base/DataStructure/Text/Path.h>
#include <Base/RTTI/RTTIManager.h>
#if _EDITOR_ONLY
//Importer
#include "FBXImporter.h"
#include "PNGImporter.h"

namespace PigeonEngine
{
	static void RegisterClassTypes()
	{
		PigeonEngine::RegisterClassType<PigeonEngine::EImportManager, PigeonEngine::EManagerBase>();
	}
	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);
};

PigeonEngine::EImportManager::EImportManager()
{
	
}
PigeonEngine::EImportManager::~EImportManager()
{
	
}

void PigeonEngine::EImportManager::Initialize()
{
}

void PigeonEngine::EImportManager::ShutDown()
{
	//delete managed data.
	for (auto Item = m_Importers.Begin(); Item != m_Importers.End(); Item++)
	{
		if (Item->second)
			delete Item->second;
	}
}

void PigeonEngine::EImportManager::ShowImporterButton()
{
	if (ImGui::Button("Import"))
	{
		PathSet PathSet;
		OpenDialogMultiSelect(GetFileFilterList(), NULL, &PathSet);

		TMap<EString,TArray<EString>> PathMap;
		for (INT32 i = 0; i < PathSet.count; i++)
		{
			const CHAR* Path = GetPath(&PathSet, i);
			EString PathString(Path);
			EString FileExtension = EPath::GetExtension(PathString);
			FileExtension = ToUpper(FileExtension);
			TArray<EString> OutPath;
			if (!PathMap.FindValue(FileExtension, OutPath))
			{
				OutPath.Add(PathString);
			}
			PathMap[FileExtension] = OutPath;
		}
		for(auto Item = PathMap.Begin();Item != PathMap.End();Item++)
		{
			IImporter* Importer;
			if(m_Importers.FindValue(Item->first, Importer))
			Importer->CreateImportEditor(Item->second);
		}

		FreePathSet(&PathSet);
	}
}

void PigeonEngine::EImportManager::EditorInit()
{
	AddImporter("FBX", new EFBXImporter());
	AddImporter("PNG", new EPNGImporter());
}

void PigeonEngine::EImportManager::EditorUpdate()
{
	for (auto Item = m_Importers.Begin(); Item != m_Importers.End(); ++Item)
	{
		Item->second->UpdateEditor();
	}
}

void PigeonEngine::EImportManager::AddImporter(EString FileExtension, IImporter* Importer)
{
	m_Importers.Add(FileExtension, Importer);
}

const CHAR* PigeonEngine::EImportManager::GetFileFilterList()
{
	TArray<EString> KeyArray;
	m_Importers.GenerateKeyArray(KeyArray);
	if (KeyArray.Length())
	{
		EString FileFilterList = KeyArray[0];
		for (UINT32 i = 1; i < KeyArray.Length(); i++)
		{
			FileFilterList += ":";
			FileFilterList += KeyArray[i];
		}
		return *FileFilterList;
	}
	return "";
}
#endif