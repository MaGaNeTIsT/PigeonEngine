#pragma once

#include <Base/DataStructure/BuiltInType.h>

namespace PigeonEngine
{

	class EEngineSettings final
	{
	public:
		PE_CONSTEXPR_STATIC		FLOAT			ENGINE_SCREEN_WIDTH													= 1600.f;
		PE_CONSTEXPR_STATIC		FLOAT			ENGINE_SCREEN_HEIGHT												= 900.f;
		PE_CONSTEXPR_STATIC		FLOAT			ENGINE_FIXED_UPDATE_FRAME											= 60.f;
		PE_CONSTEXPR_STATIC		FLOAT			ENGINE_UPDATE_FRAME													= 90.f;
		PE_CONSTEXPR_STATIC		BOOL32			ENGINE_WINDOWED														= TRUE;

		PE_CONSTEXPR_STATIC		FLOAT			ENGINE_BOUND_MINIMUM												= 1.0f;
		PE_CONSTEXPR_STATIC		FLOAT			ENGINE_BOUND_MINIMUM_HALF											= ENGINE_BOUND_MINIMUM * 0.5f;
		PE_CONSTEXPR_STATIC		FLOAT			ENGINE_BOUND_MINIMUM_LENGTH											= 1.73205080f;
		PE_CONSTEXPR_STATIC		FLOAT			ENGINE_BOUND_MINIMUM_LENGTH_HALF									= ENGINE_BOUND_MINIMUM_LENGTH * 0.5f;

		PE_CONSTEXPR_STATIC		UINT32			ENGINE_BONE_WEIGHT_NUM_MAXIMUM										= 16u;

		PE_CONSTEXPR_STATIC		UINT32			ENGINE_MESH_NAME_LENGTH_MAX											= 1024u;
		PE_CONSTEXPR_STATIC		UINT32			ENGINE_SKELETON_NAME_LENGTH_MAX										= 1024u;
		PE_CONSTEXPR_STATIC		UINT32			ENGINE_ANIMATION_NAME_LENGTH_MAX									= 1024u;
		PE_CONSTEXPR_STATIC		UINT32			ENGINE_BONE_NAME_LENGTH_MAX											= 512u;
		PE_CONSTEXPR_STATIC		UINT32			ENGINE_ANIMATION_CLIP_NAME_LENGTH_MAX								= 512u;

		PE_CONSTEXPR_STATIC		FLOAT			ENGINE_VISIBLE_CULLING_DISTANCE										= 200000.f;
		PE_CONSTEXPR_STATIC		FLOAT			ENGINE_CAMERA_FOV													= 60.f;

		PE_CONSTEXPR_STATIC		const CHAR*		ENGINE_DEFAULT_NAME													= "PE_DEFAULT_NAME";
		PE_CONSTEXPR_STATIC		const CHAR*		ENGINE_ASSET_NAME_TYPE												= ".PAsset";
		PE_CONSTEXPR_STATIC		const CHAR*		ENGINE_IMPORT_SHADER_NAME_TYPE										= ".cso";
		PE_CONSTEXPR_STATIC		const CHAR*		ENGINE_IMPORT_VERTEX_SHADER_NAME_TYPE								= "PEVS";
		PE_CONSTEXPR_STATIC		const CHAR*		ENGINE_IMPORT_PIXEL_SHADER_NAME_TYPE								= "PEPS";
		PE_CONSTEXPR_STATIC		const CHAR*		ENGINE_IMPORT_COMPUTE_SHADER_NAME_TYPE								= "PECS";

		PE_CONSTEXPR_STATIC		const CHAR*     PROJECT_CONTENT_PATH                                                = "UNDEFINED";

		PE_CONSTEXPR_STATIC		UINT32			ENGINE_SHADOW_MAP_SIZE												= 2048u;
		PE_CONSTEXPR_STATIC		UINT32			ENGINE_CASCADE_SHADOW_LAYER_NUM										= 3u;
		PE_CONSTEXPR_STATIC		FLOAT			ENGINE_CASCADE_SHADOW_LAYER[ENGINE_CASCADE_SHADOW_LAYER_NUM]		= { 1000.f, 3000.f, 5000.f };
		PE_CONSTEXPR_STATIC		FLOAT			ENGINE_CASCADE_SHADOW_BORDER[ENGINE_CASCADE_SHADOW_LAYER_NUM - 1u]	= { 100.f, 200.f };

		PE_CONSTEXPR_STATIC		UINT32			ENGINE_PROFILER_AVERAGE_NUM											= 4u;
	public:
		static EEngineSettings* GetSingleton()
		{
			static EEngineSettings _StaticSingletonObject;
			return (&_StaticSingletonObject);
		}
		EEngineSettings(const EEngineSettings&) = delete;
		EEngineSettings& operator=(const EEngineSettings&) = delete;
	private:
		EEngineSettings() {}
		~EEngineSettings() {}
	};

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
#define ENGINE_DEFAULT_LIT_SHADER_FORWARD_SKELETON_MESH_VS	("./Engine/Assets/EngineShaders/EngineDefaultLitForwardSkeletalMeshVS.cso")
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