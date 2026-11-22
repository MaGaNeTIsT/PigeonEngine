#pragma once

#include <CoreMinimal.h>

#if ENGINE_EDITOR_ONLY

#include <imgui.h>
#include <BaseAsset.h>

namespace PigeonEngine
{
	class ETexture2DAsset;

	/*
	* One icon's asset and its render-side handle. The asset is loaded during editor
	* init, but the texture proxy is created on the render thread; EngineTextureToImgui
	* only answers once that proxy is valid, so the id is resolved by Tick() instead of
	* being cached during Load().
	*/
	struct FAssetIcon
	{
		const ETexture2DAsset*	Asset = nullptr;
		ImTextureID				TextureId = static_cast<ImTextureID>(0);
	};

	/*
	* Editor-only icon table for the content browser. The PNGs live under
	* Assets/EngineTextures/Editor and are loaded once through the texture asset manager;
	* every type without a dedicated image falls back to the unknown icon, and a missing
	* texture yields 0 so the panel can draw a placeholder instead of calling Image(0, ...).
	*/
	class FAssetIconRegistry final
	{
	public:
		// Loads the texture assets. Does not resolve ImTextureID: that waits for the render
		// proxy created by ETexture2DAsset::InitResource.
		void Load();
		// Resolves pending icon handles. Cheap once every icon has a valid texture id.
		void Tick();
		void ShutDown();

		PE_NODISCARD ImTextureID GetFolderIcon() const;
		PE_NODISCARD ImTextureID GetTypeIcon(const EAssetType& InType) const;

	private:
		void MapType(const EAssetType& InType, const ETexture2DAsset* InAsset);
		// Returns TRUE when the icon has a usable id (already cached or resolved now).
		PE_NODISCARD BOOL32 ResolveIcon(FAssetIcon& InIcon);

	private:
		FAssetIcon						FolderIcon;
		TMap<EAssetType, FAssetIcon>	TypeIcons;
		// Frames spent waiting for the render thread to publish the texture proxies.
		UINT32							PendingFrameNum = 0u;
		BOOL32							bReportedPending = FALSE;
	};
}

#endif
