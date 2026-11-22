#pragma once

#include <CoreMinimal.h>

#if ENGINE_EDITOR_ONLY

#include <BaseAsset.h>

namespace PigeonEngine
{
	/*
	* Best-effort reference scan for rename/delete confirmation. It walks the loaded asset
	* caches and the open world; it is not a dependency graph. Asset families without an
	* introspection path are deliberately not returned here, which is why the content
	* browser only enables file operations for the types this function understands.
	*/
	namespace EAssetUsage
	{
		void CollectUsers(const EString& InAssetPath, const EAssetType& InType, TArray<EString>& OutUsers);
	}
}

#endif
