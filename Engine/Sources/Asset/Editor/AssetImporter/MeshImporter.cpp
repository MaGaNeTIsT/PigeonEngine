#include "MeshImporter.h"
#include "MeshAsset/MeshAsset.h"
#if _EDITOR_ONLY
void PigeonEngine::EMeshImporter::CreateImportEditor(TArray<EString> Paths)
{
	//Handle Path.
	//TryLoadStaticMesh();
}

void PigeonEngine::EMeshImporter::UpdateImportEditor()
{
	ImGui::Begin("PigeonContent", FALSE, ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar);
	{

	}
	ImGui::End();
}
#endif