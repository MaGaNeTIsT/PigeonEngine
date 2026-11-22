#pragma once

#include <CoreMinimal.h>

#if ENGINE_EDITOR_ONLY

#include <BaseAsset.h>

namespace PigeonEngine
{
	/*
	* Asset type helpers shared by the content browser and the animation/mesh preview
	* modules. Reading the type is a four-byte header read; every copy of that rule used
	* to live in its own editor module.
	*/
	namespace EAssetTypeUtil
	{
		PE_NODISCARD EAssetType ReadTypeFromFile(const EString& InPath);
		PE_NODISCARD const CHAR* ToString(const EAssetType& InType);
		PE_NODISCARD const CHAR* GetEditorIconName(const EAssetType& InType);
	}
}

#endif
