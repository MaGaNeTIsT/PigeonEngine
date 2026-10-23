#pragma once

#include <BaseConfig.h>
#include <Base/DataStructure/BuiltInType.h>
#include <Config/EngineShaderConfig.h>

namespace PigeonEngine
{

	class EEngineSettings final
	{
	public:
		PE_CONSTEXPR_STATIC		FLOAT			ENGINE_SCREEN_WIDTH													= 1600.f;
		PE_CONSTEXPR_STATIC		FLOAT			ENGINE_SCREEN_HEIGHT												= 900.f;
		PE_CONSTEXPR_STATIC		FLOAT			ENGINE_FIXED_UPDATE_FRAME											= 60.f;
		PE_CONSTEXPR_STATIC		FLOAT			ENGINE_UPDATE_FRAME													= 90.f;
		// Tick rate assumed for an animation clip whose source did not declare one. Some FBX
		// files report 0 in mTicksPerSecond; 30 is the convention the old animation component
		// and the editor preview used, and it keeps the clip playable instead of rejected.
		PE_CONSTEXPR_STATIC		FLOAT			ENGINE_ANIMATION_DEFAULT_TICKS_PER_SECOND							= 30.f;
		PE_CONSTEXPR_STATIC		BOOL32			ENGINE_WINDOWED														= TRUE;

		// 1 = D3D11, 2 = D3D12 (matches ERHIBackendType enum)
		PE_CONSTEXPR_STATIC		UINT8			ENGINE_GRAPHICS_API													= 1u;

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
		PE_CONSTEXPR_STATIC		UINT32			ENGINE_ANIMATION_NOTIFY_NAME_LENGTH_MAX								= 256u;
		PE_CONSTEXPR_STATIC		UINT32			ENGINE_ANIMATION_NOTIFY_CLASS_NAME_LENGTH_MAX						= 256u;
		PE_CONSTEXPR_STATIC		UINT32			ENGINE_ANIMATION_NOTIFY_PAYLOAD_LENGTH_MAX							= 2048u;

		PE_CONSTEXPR_STATIC		FLOAT			ENGINE_VISIBLE_CULLING_DISTANCE										= 200000.f;
		PE_CONSTEXPR_STATIC		FLOAT			ENGINE_CAMERA_FOV													= 60.f;

		// Physical exposure. EV100 is the exposure value at ISO 100 and it is what collapses
		// aperture, shutter and sensitivity into one number; Sunny-16 outdoors is EV100 14.64,
		// which puts display white at a scene luminance of about 25 320 cd/m2. Exposure
		// compensation is in stops and shifts the result by 2^compensation.
		PE_CONSTEXPR_STATIC		FLOAT			ENGINE_CAMERA_EV100													= 14.64f;
		PE_CONSTEXPR_STATIC		FLOAT			ENGINE_CAMERA_ISO													= 100.f;
		PE_CONSTEXPR_STATIC		FLOAT			ENGINE_CAMERA_EXPOSURE_COMPENSATION									= 0.f;

		// Sky luminance in cd/m2. The environment cube is sampled as radiance, so this is what
		// puts it on the same physical scale as the lights: a clear sky is around 8000 cd/m2
		// and an overcast one around 2000.
		PE_CONSTEXPR_STATIC		FLOAT			ENGINE_SKY_LUMINANCE												= 8000.f;

		// Light defaults. These are physical quantities and not multipliers, so a light and a
		// camera only look right together: the default exposure is Sunny-16 outdoors, which is
		// why the default directional light is a sun and the default local light is a strong
		// studio lamp rather than something you would put in a room.
		//
		// The sun's disc is 0.53 degrees across. It is what gives a specular highlight its real
		// width - a mirror reflecting the sun returns a disc, not a point - and it becomes the
		// penumbra size when soft shadows land.
		PE_CONSTEXPR_STATIC		FLOAT			ENGINE_DEFAULT_ILLUMINANCE_LUX										= 100000.f;
		PE_CONSTEXPR_STATIC		FLOAT			ENGINE_DEFAULT_SOURCE_ANGLE_DEGREES									= 0.53f;
		PE_CONSTEXPR_STATIC		FLOAT			ENGINE_DEFAULT_LUMINOUS_INTENSITY_CANDELA							= 1000.f;
		PE_CONSTEXPR_STATIC		FLOAT			ENGINE_DEFAULT_ATTENUATION_RADIUS_METERS							= 1000.f;
		PE_CONSTEXPR_STATIC		FLOAT			ENGINE_DEFAULT_SPOT_INNER_ANGLE_DEGREES								= 20.f;
		PE_CONSTEXPR_STATIC		FLOAT			ENGINE_DEFAULT_SPOT_OUTER_ANGLE_DEGREES								= 30.f;

		PE_CONSTEXPR_STATIC		const CHAR*		ENGINE_DEFAULT_NAME													= "PE_DEFAULT_NAME";
		PE_CONSTEXPR_STATIC		const CHAR*		ENGINE_ASSET_NAME_TYPE												= ".PAsset";
		PE_CONSTEXPR_STATIC		const CHAR*		ENGINE_IMPORT_SHADER_NAME_TYPE										= ".cso";
		PE_CONSTEXPR_STATIC		const CHAR*		ENGINE_IMPORT_VERTEX_SHADER_NAME_TYPE								= MC_SHADER_VS_SUFFIX;
		PE_CONSTEXPR_STATIC		const CHAR*		ENGINE_IMPORT_PIXEL_SHADER_NAME_TYPE								= MC_SHADER_PS_SUFFIX;
		PE_CONSTEXPR_STATIC		const CHAR*		ENGINE_IMPORT_COMPUTE_SHADER_NAME_TYPE								= MC_SHADER_CS_SUFFIX;

		PE_CONSTEXPR_STATIC		UINT32			ENGINE_SHADER_NAME_LENGTH_MAX										= 1024u;
		PE_CONSTEXPR_STATIC		const CHAR*		ENGINE_ROOT_DIR														= EBaseSettings::ENGINE_ROOT_DIRECTORY;
		PE_CONSTEXPR_STATIC		const CHAR*		ENGINE_SHADER_OUTPUT_ROOT_DIR										= EBaseSettings::ENGINE_SHADER_BUILD_DIRECTORY;
		PE_CONSTEXPR_STATIC		const CHAR*		ENGINE_SHADER_SOURCE_DIR											= EBaseSettings::ENGINE_SHADER_SOURCE_DIRECTORY;
		PE_CONSTEXPR_STATIC		const CHAR*		ENGINE_SHADER_GENERATED_ROOT_DIR									= EBaseSettings::ENGINE_SHADER_GENERATED_DIRECTORY;

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

#define ENGINE_DEFAULT_TEXTURE2D_WHITE						("./Assets/EngineTextures/Default/EngineDefaultWhite.tga")
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
