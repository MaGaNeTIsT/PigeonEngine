#pragma once

namespace PigeonEngine
{

#if _EDITOR_ONLY
#define ENGINE_SHADER_ASSET_MANAGER_NAME		("ENGINE_SHADER_ASSET_MANAGER")
#define ENGINE_TEXTURE_ASSET_MANAGER_NAME		("ENGINE_TEXTURE_ASSET_MANAGER")
#define ENGINE_MESH_ASSET_MANAGER_NAME			("ENGINE_MESH_ASSET_MANAGER")
#define ENGINE_SKELETON_ASSET_MANAGER_NAME		("ENGINE_SKELETON_ASSET_MANAGER")
#define ENGINE_ANIMATION_ASSET_MANAGER_NAME		("ENGINE_ANIMATION_ASSET_MANAGER")

#define ENGINE_ASSIMP_MANAGER_NAME				("ENGINE_ASSIMP_MANAGER")
#define ENGINE_IMGUI_MANAGER_NAME				("ENGINE_IMGUI_MANAGER")
#endif

};