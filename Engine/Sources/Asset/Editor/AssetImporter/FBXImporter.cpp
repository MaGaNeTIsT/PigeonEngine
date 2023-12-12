#include "FBXImporter.h"
#include "MeshAsset/MeshAsset.h"
#if _EDITOR_ONLY
void PigeonEngine::EFBXImporter::CreateImportEditor(TArray<EString> Paths)
{
	//Handle Path.
	//TryLoadStaticMesh();
}

void PigeonEngine::EFBXImporter::UpdateImportEditor()
{
	ImGui::Begin("PigeonContent", FALSE, ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar);
	{

	}
	ImGui::End();
}
#endif