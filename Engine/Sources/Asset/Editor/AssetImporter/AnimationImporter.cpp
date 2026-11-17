#include "AnimationImporter.h"
#include "AnimationAsset/AnimationAsset.h"
#if ENGINE_EDITOR_ONLY
void PigeonEngine::EAnimationImporter::CreateImportEditor(TArray<EString> InPaths)
{
	NeedUpdate = TRUE;
	bInitializeEditor = FALSE;
	Paths.Empty();
	for (UINT32 PathIndex = 0u, PathNum = InPaths.Num<UINT32>(); PathIndex < PathNum; PathIndex++)
	{
		Paths.Add(InPaths[PathIndex].Replace("\\", "/"));
	}
}

void PigeonEngine::EAnimationImporter::UpdateImportEditor()
{
	static CHAR CopyFileFolder[PigeonEngine::EPath::MAX_PATH_LENGTH] = "";
	static BOOL8 bCopyFile = FALSE;
	static TArray<EString> SkeletonNames;
	static INT32 SelectedSkeletonIndex = -1;
	static CHAR RootBoneName[EEngineSettings::ENGINE_BONE_NAME_LENGTH_MAX] = "";

	if (!bInitializeEditor)
	{
		::memset(CopyFileFolder, 0, EPath::MAX_PATH_LENGTH);
		::memset(RootBoneName, 0, EEngineSettings::ENGINE_BONE_NAME_LENGTH_MAX);
		bCopyFile = FALSE;
		SkeletonNames.Empty();
		SelectedSkeletonIndex = -1;

		// The imported clips belong to one of the skeleton assets the engine already knows.
		// Scanning the skeleton folder once when the importer opens keeps the UI responsive;
		// a skeleton imported afterwards is picked up by reopening the importer.
		TArray<EString> FolderPaths;
		TArray<EString> FilePaths;
		if (EFileHelper::ScanDirectory(EBaseSettings::ENGINE_SKELETON_PATH, FolderPaths, FilePaths, TRUE))
		{
			for (INT32 FileIndex = 0; FileIndex < FilePaths.Num(); FileIndex++)
			{
				if (FilePaths[FileIndex].Contains(EEngineSettings::ENGINE_ASSET_NAME_TYPE))
				{
					SkeletonNames.Add(EPath::GetFileNameWithoutExtension(FilePaths[FileIndex]));
				}
			}
		}
		// Default to "None": the importer cannot know which skeleton an animation-only FBX
		// belongs to, and a wrong hint would only produce a mismatch warning at runtime.
		SelectedSkeletonIndex = -1;
		bInitializeEditor = TRUE;
	}

	ImGui::Begin("Animation Importer", &NeedUpdate, ImGuiWindowFlags_::ImGuiWindowFlags_None);
	{
		for (UINT32 PathIndex = 0u, PathNum = Paths.Num<UINT32>(); PathIndex < PathNum; PathIndex++)
		{
			ImGui::Text("FileName:");
			ImGui::SameLine();
			{
				EString FileName = EPath::GetFileNameWithExtension(Paths[PathIndex]);
				ImGui::Text(*FileName);
			}
		}

		const CHAR* CurrentSkeletonLabel = (SelectedSkeletonIndex >= 0) ? *SkeletonNames[SelectedSkeletonIndex] : "None";
		if (ImGui::BeginCombo("Skeleton", CurrentSkeletonLabel))
		{
			if (ImGui::Selectable("None", (SelectedSkeletonIndex < 0)))
			{
				SelectedSkeletonIndex = -1;
			}
			for (INT32 SkeletonIndex = 0; SkeletonIndex < SkeletonNames.Num(); SkeletonIndex++)
			{
				const BOOL8 bSelected = (SelectedSkeletonIndex == SkeletonIndex);
				if (ImGui::Selectable(*SkeletonNames[SkeletonIndex], bSelected))
				{
					SelectedSkeletonIndex = SkeletonIndex;
				}
				if (bSelected)
				{
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
		ImGui::PushItemWidth(250);
		ImGui::InputText("Root Bone", RootBoneName, EEngineSettings::ENGINE_BONE_NAME_LENGTH_MAX, ImGuiInputTextFlags_CharsNoBlank);
		ImGui::PopItemWidth();

		ImGui::Checkbox("CopyFile", &bCopyFile);
		if (bCopyFile)
		{
			ImGui::PushItemWidth(250);
			ImGui::InputText("Copy File Path", CopyFileFolder, EPath::MAX_PATH_LENGTH, ImGuiInputTextFlags_CharsNoBlank);
			ImGui::PopItemWidth();
		}

		if (ImGui::Button("Import"))
		{
			const EString SkeletonName = ((SelectedSkeletonIndex >= 0) ? SkeletonNames[SelectedSkeletonIndex] : EString());
			const EString RootBone = EString(RootBoneName);
			BOOL32 bAnyImported = FALSE;
			for (UINT32 PathIndex = 0u, PathNum = Paths.Num<UINT32>(); PathIndex < PathNum; PathIndex++)
			{
				//Import Asset
				if (EAnimationManager::GetManagerSingleton()->ImportSkeletonAnimation(
					EPath::GetFileNameWithoutExtension(Paths[PathIndex]), Paths[PathIndex],
					EBaseSettings::ENGINE_ANIMATION_PATH, SkeletonName, RootBone) != FALSE)
				{
					bAnyImported = TRUE;
				}

				//CopyFile
				if (/*Asset &&*/ bCopyFile)
				{
					EString CopyPath = EPath::Combine(EString(EBaseSettings::ENGINE_ASSET_DIRECTORY), EString(CopyFileFolder), EPath::GetFileNameWithExtension(Paths[PathIndex]));
					if (!EFileHelper::CopyFileToNewPath(Paths[PathIndex], CopyPath))
					{
						PE_CHECK((ENGINE_ASSET_ERROR), ("Animation Importer : copy file error!"), (FALSE));
					}
				}
			}
			if (bAnyImported != FALSE)
			{
				NotifyImportFinished();
			}
			NeedUpdate = FALSE;
		}
	}
	ImGui::End();
}
#endif
