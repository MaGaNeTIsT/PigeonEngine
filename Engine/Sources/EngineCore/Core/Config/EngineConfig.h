#pragma once

#include <Base/DataStructure/BuiltInType.h>

namespace PigeonEngine
{

#define ENGINE_CUSTOM_ARRAYSIZE(_ARR)						(static_cast<UINT>(sizeof(_ARR) / sizeof(*(_ARR))))

	class ESettings
	{
	public:
		constexpr static FLOAT	ENGINE_SCREEN_WIDTH					= 1600.f;
		constexpr static FLOAT	ENGINE_SCREEN_HEIGHT				= 900.f;
		constexpr static FLOAT	ENGINE_FIXED_UPDATE_FRAME			= 60.f;
		constexpr static FLOAT	ENGINE_UPDATE_FRAME					= 90.f;
		constexpr static BOOL	ENGINE_WINDOWED						= TRUE;

		constexpr static FLOAT	ENGINE_BOUND_MINIMUM				= 0.1f;
		constexpr static FLOAT	ENGINE_BOUND_MINIMUM_HALF			= 0.05f;

		constexpr static UINT	ENGINE_BONE_WEIGHT_NUM_MAXIMUM		= 16u;

		constexpr static UINT	ENGINE_MESH_NAME_LENGTH_MAX			= 1024u;
		constexpr static UINT	ENGINE_SKELETON_NAME_LENGTH_MAX		= 1024u;
		constexpr static UINT	ENGINE_ANIMATION_NAME_LENGTH_MAX	= 1024u;
		constexpr static UINT	ENGINE_BONE_NAME_LENGTH_MAX			= 512u;

	public:
		static ESettings* GetSingleton()
		{
			static ESettings _StaticSingletonObject;
			return (&_StaticSingletonObject);
		}
		ESettings(const ESettings&) = delete;
		ESettings& operator=(const ESettings&) = delete;
	private:
		ESettings() {}
		~ESettings() {}
	};

#define ENGINE_NOT_EXIST_STRING								("ENGINE_NOT_EXIST")
#define ENGINE_DEFAULT_NAME									("PIGEON_ENGINE_DEFAULT_NAME")

#define ENGINE_ASSET_NAME_TYPE								(".PAsset")

#define ENGINE_IMPORT_SHADER_NAME_TYPE						(".cso")

#define ENGINE_IMPORT_VERTEX_SHADER_NAME_TYPE				("PEVS")
#define ENGINE_IMPORT_PIXEL_SHADER_NAME_TYPE				("PEPS")
#define ENGINE_IMPORT_COMPUTE_SHADER_NAME_TYPE				("PECS")

#define ENGINE_DEFAULT_TEXTURE2D_WHITE						("./Engine/Assets/EngineTextures/Default/EngineDefaultWhite.tga")
#define ENGINE_DEFAULT_TEXTURE2D_BLACK						("./Engine/Assets/EngineTextures/Default/EngineDefaultBlack.tga")
#define ENGINE_DEFAULT_TEXTURE2D_GRAY						("./Engine/Assets/EngineTextures/Default/EngineDefaultGray.tga")
#define ENGINE_DEFAULT_TEXTURE2D_RED						("./Engine/Assets/EngineTextures/Default/EngineDefaultRed.tga")
#define ENGINE_DEFAULT_TEXTURE2D_GREEN						("./Engine/Assets/EngineTextures/Default/EngineDefaultGreen.tga")
#define ENGINE_DEFAULT_TEXTURE2D_BLUE						("./Engine/Assets/EngineTextures/Default/EngineDefaultBlue.tga")
#define ENGINE_DEFAULT_TEXTURE2D_BUMP						("./Engine/Assets/EngineTextures/Default/EngineDefaultBump.tga")
#define ENGINE_DEFAULT_TEXTURE2D_PROPERTY					("./Engine/Assets/EngineTextures/Default/EngineDefaultProperty.tga")

#define ENGINE_SHADER_NONE									("EngineBase/Shader/None")
#define ENGINE_DEFAULT_SHADER_VS							("./Engine/Assets/EngineShaders/EngineDefaultVS.cso")

#define ENGINE_SHADER_EMPTY_PS								("./Engine/Assets/EngineShaders/EngineEmptyPS.cso")
#define ENGINE_SHADER_GBUFFER_WRITE_PS						("./Engine/Assets/EngineShaders/EngineGBufferWritePS.cso")
#define ENGINE_SHADER_DIRECT_LIGHT_PS						("./Engine/Assets/EngineShaders/EngineDirectLightPS.cso")

#define ENGINE_DEFAULT_LIT_SHADER_FORWARD_VS				("./Engine/Assets/EngineShaders/EngineDefaultLitForwardVS.cso")
#define ENGINE_DEFAULT_LIT_SHADER_FORWARD_SKELETON_MESH_VS	("./Engine/Assets/EngineShaders/EngineDefaultLitForwardSkeletonMeshVS.cso")
#define ENGINE_DEFAULT_LIT_SHADER_FORWARD_PS				("./Engine/Assets/EngineShaders/EngineDefaultLitForwardPS.cso")
#define ENGINE_DEFAULT_ANISOTROPIC_SHADER_FORWARD_VS		("./Engine/Assets/EngineShaders/EngineDefaultAnisotropicForwardVS.cso")
#define ENGINE_DEFAULT_ANISOTROPIC_SHADER_FORWARD_PS		("./Engine/Assets/EngineShaders/EngineDefaultAnisotropicForwardPS.cso")
#define ENGINE_DEFAULT_CLEARCOAT_SHADER_FORWARD_VS			("./Engine/Assets/EngineShaders/EngineDefaultClearCoatForwardVS.cso")
#define ENGINE_DEFAULT_CLEARCOAT_SHADER_FORWARD_PS			("./Engine/Assets/EngineShaders/EngineDefaultClearCoatForwardPS.cso")
#define ENGINE_DEFAULT_SHADER_CLOTH_VS						("./Engine/Assets/EngineShaders/EngineDefaultClothVS.cso")
#define ENGINE_DEFAULT_SHADER_CLOTH_PS						("./Engine/Assets/EngineShaders/EngineDefaultClothPS.cso")
#define ENGINE_SHADER_CLOTH_ANISOTROPIC_VS					("./Engine/Assets/EngineShaders/EngineClothAnisotropicVS.cso")
#define ENGINE_SHADER_CLOTH_ANISOTROPIC_PS					("./Engine/Assets/EngineShaders/EngineClothAnisotropicPS.cso")
#define ENGINE_DEFAULT_SHADER_TERRAIN_VS					("./Engine/Assets/EngineShaders/EngineDefaultTerrainVS.cso")
#define ENGINE_DEFAULT_SHADER_TERRAIN_PS					("./Engine/Assets/EngineShaders/EngineDefaultTerrainPS.cso")
#define ENGINE_SHADER_SCREEN_POLYGON_2D_VS					("./Engine/Assets/EngineShaders/EngineScreenPolygon2DVS.cso")
#define ENGINE_SHADER_SCREEN_POLYGON_2D_PS					("./Engine/Assets/EngineShaders/EngineScreenPolygon2DPS.cso")
#define ENGINE_SHADER_SKY_BOX_VS							("./Engine/Assets/EngineShaders/EngineSkyBoxVS.cso")
#define ENGINE_SHADER_SKY_BOX_PS							("./Engine/Assets/EngineShaders/EngineSkyBoxPS.cso")

#define ENGINE_MESH_MATERIAL_SPHERE_NAME					("./Engine/Assets/EngineModels/SceneModels/MaterialSphere/MaterialSphere.obj")
#define ENGINE_MESH_MONKEY_NAME								("./Engine/Assets/EngineModels/SceneModels/Monkey/Monkey.obj")
#define ENGINE_MESH_NORMAL_CUBE_NAME						("./Engine/Assets/EngineModels/BaseShapes/Cube.obj")
#define ENGINE_MESH_ROUNDED_CUBE_NAME						("./Engine/Assets/EngineModels/BaseShapes/RoundedCube.obj")
#define ENGINE_MESH_SMOOTH_SPHERE_NAME						("./Engine/Assets/EngineModels/BaseShapes/SphereSmooth.obj")
#define ENGINE_MESH_UV_SPHERE_NAME							("./Engine/Assets/EngineModels/BaseShapes/SphereUV.obj")
#define ENGINE_MESH_TORUS_NAME								("./Engine/Assets/EngineModels/BaseShapes/Torus.obj")
#define ENGINE_MESH_PRISM_NAME								("./Engine/Assets/EngineModels/BaseShapes/Prism.obj")
#define ENGINE_MESH_CUBE_NAME								("EngineBase/Mesh/CubeMesh")
#define ENGINE_MESH_PLANE_NAME								("EngineBase/Mesh/PlaneMesh")
#define ENGINE_MESH_POLYGON_NAME							("EngineBase/Mesh/PolygonMesh")
#define ENGINE_MESH_POLYGON_2D_NAME							("EngineBase/Mesh/Polygon2DMesh")

};