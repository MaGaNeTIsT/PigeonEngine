#include <Editor/Asset/AssetIconRegistry.h>

#if ENGINE_EDITOR_ONLY

#include <Base/DataStructure/Text/Path.h>
#include <Config/EngineConfig.h>
#include <Development/EditorLogManager.h>
#include <TextureAsset/TextureAsset.h>
#include <imGUI/Headers/imGUIManager.h>

namespace PigeonEngine
{
	namespace
	{
		// Long enough for the render thread to finish the first frame's queued texture
		// creations; a warning after this is a real readiness failure, not a startup race.
		PE_CONSTEXPR_STATIC UINT32 EDITOR_ICON_PENDING_WARN_FRAMES = 120u;

		const ETexture2DAsset* LoadEditorIcon(const CHAR* InIconName)
		{
			EString ImportPath(EBaseSettings::ENGINE_ASSET_DIRECTORY);
			ImportPath += "EngineTextures/Editor/";
			const EString LoadName(InIconName);
			EString ImportName(InIconName);
			EString ImportFileType("PNG");

			const ETexture2DAsset* Asset = nullptr;
			TryLoadTexture2D(EBaseSettings::ENGINE_TEXTURE_PATH, LoadName, Asset
				, &ImportPath, &ImportName, &ImportFileType);
			if (!Asset)
			{
				PE_LOG_WARN(EString("AssetIconRegistry: failed to load icon texture: ") + InIconName);
			}
			return Asset;
		}
	}

	void FAssetIconRegistry::Load()
	{
		ShutDown();

		FolderIcon.Asset = LoadEditorIcon("Icon_ContentBrowser_Folder");

		const ETexture2DAsset* MeshAsset = LoadEditorIcon("Icon_ContentBrowser_Mesh");
		const ETexture2DAsset* ShaderAsset = LoadEditorIcon("Icon_ContentBrowser_Shader");
		const ETexture2DAsset* SkeletonAsset = LoadEditorIcon("Icon_ContentBrowser_Skeleton");
		const ETexture2DAsset* AnimationAsset = LoadEditorIcon("Icon_ContentBrowser_Animation");
		const ETexture2DAsset* UnknownAsset = LoadEditorIcon("Icon_ContentBrowser_No");

		MapType(ASSET_TYPE_MESH, MeshAsset);
		// A navigation mesh is geometry; no dedicated icon exists, so it reuses the mesh one.
		MapType(ASSET_TYPE_NAV_MESH, MeshAsset);
		MapType(ASSET_TYPE_SHADER, ShaderAsset);
		MapType(ASSET_TYPE_SKELETON, SkeletonAsset);
		MapType(ASSET_TYPE_ANIMATION, AnimationAsset);
		// Montage/state machine/notify share the animation icon until dedicated editor
		// icons exist.
		MapType(ASSET_TYPE_ANIM_MONTAGE, AnimationAsset);
		MapType(ASSET_TYPE_ANIM_STATE_MACHINE, AnimationAsset);
		MapType(ASSET_TYPE_ANIM_NOTIFY, AnimationAsset);
		MapType(ASSET_TYPE_TEXTURE, UnknownAsset);
		MapType(ASSET_TYPE_UNKNOWN, UnknownAsset);
	}

	void FAssetIconRegistry::Tick()
	{
		UINT32 PendingNum = 0u;
		if ((FolderIcon.Asset) && (ResolveIcon(FolderIcon) == FALSE))
		{
			PendingNum++;
		}
		for (TMap<EAssetType, FAssetIcon>::TIterator Iterator = TypeIcons.Begin()
			; Iterator != TypeIcons.End(); Iterator++)
		{
			FAssetIcon& Icon = Iterator->second;
			if ((Icon.Asset) && (ResolveIcon(Icon) == FALSE))
			{
				PendingNum++;
			}
		}

		if (PendingNum == 0u)
		{
			PendingFrameNum = 0u;
			return;
		}

		PendingFrameNum++;
		if ((bReportedPending == FALSE) && (PendingFrameNum >= EDITOR_ICON_PENDING_WARN_FRAMES))
		{
			bReportedPending = TRUE;
			PE_LOG_WARN(EString("AssetIconRegistry: ") + EString::FromInt(PendingNum)
				+ EString(" icon texture(s) still have no render resource; the Content Browser shows placeholders."));
		}
	}

	void FAssetIconRegistry::ShutDown()
	{
		FolderIcon = FAssetIcon();
		TypeIcons.Empty();
		PendingFrameNum = 0u;
		bReportedPending = FALSE;
	}

	ImTextureID FAssetIconRegistry::GetFolderIcon() const
	{
		return FolderIcon.TextureId;
	}

	ImTextureID FAssetIconRegistry::GetTypeIcon(const EAssetType& InType) const
	{
		if (const FAssetIcon* Found = TypeIcons.FindValueAsPtr(InType))
		{
			return Found->TextureId;
		}
		if (const FAssetIcon* Unknown = TypeIcons.FindValueAsPtr(ASSET_TYPE_UNKNOWN))
		{
			return Unknown->TextureId;
		}
		return static_cast<ImTextureID>(0);
	}

	void FAssetIconRegistry::MapType(const EAssetType& InType, const ETexture2DAsset* InAsset)
	{
		FAssetIcon Icon;
		Icon.Asset = InAsset;
		TypeIcons.Add(InType, Icon);
	}

	BOOL32 FAssetIconRegistry::ResolveIcon(FAssetIcon& InIcon)
	{
		if (InIcon.TextureId != static_cast<ImTextureID>(0))
		{
			return TRUE;
		}
		if (!InIcon.Asset)
		{
			return FALSE;
		}
		InIcon.TextureId = EngineTextureToImgui(InIcon.Asset);
		return (InIcon.TextureId != static_cast<ImTextureID>(0)) ? TRUE : FALSE;
	}
}

#endif
