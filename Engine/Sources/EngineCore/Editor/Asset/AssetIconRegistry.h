#pragma once

#include <CoreMinimal.h>

#if ENGINE_EDITOR_ONLY

#include <imgui.h>
#include <BaseAsset.h>

namespace PigeonEngine
{
	/*
	* Editor-only icon table for the content browser. The PNGs live under
	* Assets/EngineTextures/Editor and are loaded once through the texture asset manager;
	* every type without a dedicated image falls back to the unknown icon, and a missing
	* texture yields 0 so the panel can draw a placeholder instead of calling Image(0, ...).
	*/
	class FAssetIconRegistry final
	{
	public:
		void Load();
		void ShutDown();

		PE_NODISCARD ImTextureID GetFolderIcon() const;
		PE_NODISCARD ImTextureID GetTypeIcon(const EAssetType& InType) const;

	private:
		void MapType(const EAssetType& InType, const ImTextureID& InIcon);

	private:
		ImTextureID						FolderIcon = static_cast<ImTextureID>(0);
		TMap<EAssetType, ImTextureID>	TypeIcons;
	};
}

#endif
