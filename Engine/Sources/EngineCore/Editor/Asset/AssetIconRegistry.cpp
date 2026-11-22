#include <Editor/Asset/AssetIconRegistry.h>

#if ENGINE_EDITOR_ONLY

#include <Base/DataStructure/Text/Path.h>
#include <Config/EngineConfig.h>
#include <TextureAsset/TextureAsset.h>
#include <imGUI/Headers/imGUIManager.h>

namespace PigeonEngine
{
	namespace
	{
		ImTextureID LoadEditorIcon(const CHAR* InIconName)
		{
			EString ImportPath(EBaseSettings::ENGINE_ASSET_DIRECTORY);
			ImportPath += "EngineTextures/Editor/";
			const EString LoadName(InIconName);
			EString ImportName(InIconName);
			EString ImportFileType("PNG");

			const ETexture2DAsset* Asset = nullptr;
			TryLoadTexture2D(EBaseSettings::ENGINE_TEXTURE_PATH, LoadName, Asset
				, &ImportPath, &ImportName, &ImportFileType);
			return (Asset ? EngineTextureToImgui(Asset) : static_cast<ImTextureID>(0));
		}
	}

	void FAssetIconRegistry::Load()
	{
		ShutDown();

		FolderIcon = LoadEditorIcon("Icon_ContentBrowser_Folder");

		const ImTextureID MeshIcon = LoadEditorIcon("Icon_ContentBrowser_Mesh");
		const ImTextureID ShaderIcon = LoadEditorIcon("Icon_ContentBrowser_Shader");
		const ImTextureID SkeletonIcon = LoadEditorIcon("Icon_ContentBrowser_Skeleton");
		const ImTextureID AnimationIcon = LoadEditorIcon("Icon_ContentBrowser_Animation");
		const ImTextureID UnknownIcon = LoadEditorIcon("Icon_ContentBrowser_No");

		MapType(ASSET_TYPE_MESH, MeshIcon);
		// A navigation mesh is geometry; no dedicated icon exists, so it reuses the mesh one.
		MapType(ASSET_TYPE_NAV_MESH, MeshIcon);
		MapType(ASSET_TYPE_SHADER, ShaderIcon);
		MapType(ASSET_TYPE_SKELETON, SkeletonIcon);
		MapType(ASSET_TYPE_ANIMATION, AnimationIcon);
		// Montage/state machine/notify share the animation icon until dedicated editor
		// icons exist; without this the asset grids render a blank image.
		MapType(ASSET_TYPE_ANIM_MONTAGE, AnimationIcon);
		MapType(ASSET_TYPE_ANIM_STATE_MACHINE, AnimationIcon);
		MapType(ASSET_TYPE_ANIM_NOTIFY, AnimationIcon);
		MapType(ASSET_TYPE_TEXTURE, UnknownIcon);
		MapType(ASSET_TYPE_UNKNOWN, UnknownIcon);
	}

	void FAssetIconRegistry::ShutDown()
	{
		FolderIcon = static_cast<ImTextureID>(0);
		TypeIcons.Empty();
	}

	ImTextureID FAssetIconRegistry::GetFolderIcon() const
	{
		return FolderIcon;
	}

	ImTextureID FAssetIconRegistry::GetTypeIcon(const EAssetType& InType) const
	{
		if (const ImTextureID* Found = TypeIcons.FindValueAsPtr(InType))
		{
			return *Found;
		}
		if (const ImTextureID* Unknown = TypeIcons.FindValueAsPtr(ASSET_TYPE_UNKNOWN))
		{
			return *Unknown;
		}
		return static_cast<ImTextureID>(0);
	}

	void FAssetIconRegistry::MapType(const EAssetType& InType, const ImTextureID& InIcon)
	{
		// Keep a zero entry too: the lookup then never inserts a default in the middle of
		// a draw, and the panel can test for 0 before issuing a texture draw.
		TypeIcons.Add(InType, InIcon);
	}
}

#endif
