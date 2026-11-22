#include <Editor/Asset/AssetTypeUtil.h>

#if ENGINE_EDITOR_ONLY

#include <fstream>

namespace PigeonEngine
{
	namespace EAssetTypeUtil
	{
		EAssetType ReadTypeFromFile(const EString& InPath)
		{
			if (InPath.Length() == 0u)
			{
				return ASSET_TYPE_UNKNOWN;
			}
			std::ifstream File(*InPath, std::ios::binary);
			if (!File.is_open())
			{
				return ASSET_TYPE_UNKNOWN;
			}
			UINT32 StoredType = static_cast<UINT32>(ASSET_TYPE_UNKNOWN);
			File.read(reinterpret_cast<CHAR*>(&StoredType), sizeof(StoredType));
			if (File.gcount() != static_cast<std::streamsize>(sizeof(StoredType)))
			{
				return ASSET_TYPE_UNKNOWN;
			}
			if (StoredType >= static_cast<UINT32>(ASSET_TYPE_COUNT))
			{
				return ASSET_TYPE_UNKNOWN;
			}
			return static_cast<EAssetType>(StoredType);
		}

		const CHAR* ToString(const EAssetType& InType)
		{
			switch (InType)
			{
			case ASSET_TYPE_TEXTURE:
				return "Texture";
			case ASSET_TYPE_MESH:
				return "Mesh";
			case ASSET_TYPE_SKELETON:
				return "Skeleton";
			case ASSET_TYPE_ANIMATION:
				return "Animation";
			case ASSET_TYPE_SHADER:
				return "Shader";
			case ASSET_TYPE_ANIM_STATE_MACHINE:
				return "Anim State Machine";
			case ASSET_TYPE_ANIM_MONTAGE:
				return "Anim Montage";
			case ASSET_TYPE_ANIM_NOTIFY:
				return "Anim Notify";
			case ASSET_TYPE_NAV_MESH:
				return "Nav Mesh";
			case ASSET_TYPE_UNKNOWN:
				return "Unknown";
			default:
				return "Invalid";
			}
		}

		const CHAR* GetEditorIconName(const EAssetType& InType)
		{
			switch (InType)
			{
			case ASSET_TYPE_MESH:
			case ASSET_TYPE_NAV_MESH:
				return "Icon_ContentBrowser_Mesh";
			case ASSET_TYPE_SHADER:
				return "Icon_ContentBrowser_Shader";
			case ASSET_TYPE_SKELETON:
				return "Icon_ContentBrowser_Skeleton";
			case ASSET_TYPE_ANIMATION:
			case ASSET_TYPE_ANIM_STATE_MACHINE:
			case ASSET_TYPE_ANIM_MONTAGE:
			case ASSET_TYPE_ANIM_NOTIFY:
				return "Icon_ContentBrowser_Animation";
			default:
				return "Icon_ContentBrowser_No";
			}
		}
	}
}

#endif
