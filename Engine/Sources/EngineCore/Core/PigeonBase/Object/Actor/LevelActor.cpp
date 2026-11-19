#include "LevelActor.h"
#include <Base/Timer/Timer.h>
#include <Demo/DemoObstacleMotionComponent.h>
#include <Demo/SponzaScene.h>
#include <Demo/WaterScene.h>
#include <MeshAsset/MeshAsset.h>
#include <TextureAsset/TextureAsset.h>
#include <SkeletonAsset/SkeletonAsset.h>
#if ENGINE_EDITOR_ONLY
#include <AnimationMontageAsset/AnimationMontageAsset.h>
#include <AnimationNotifyAsset/AnimationNotifyAsset.h>
#include <AnimationStateMachineAsset/AnimationStateMachineAsset.h>
#endif
#include <ShaderAsset/ShaderAsset.h>
#include <ShaderAsset/ShaderInstance.h>
#include <ShaderAsset/ShaderJobBuilder.h>
#include <Shader/ShaderAssetBinding.h>
#include <Shader/EngineShaderInstanceCache.h>
#include "../Component/SceneComponent.h"
#include "../World/World.h"
#include "../Controller/Controller.h"
#include <PigeonBase/Object/Component/CameraAndLight/DirectionalLightComponent.h>
#include <PigeonBase/Object/Component/CameraAndLight/PointLightComponent.h>
#include <PigeonBase/Object/Component/CameraAndLight/SpotLightComponent.h>
#include <PigeonBase/Object/Component/Primitive/SkyLightComponent.h>
#include <PigeonBase/Object/Component/Primitive/StaticMeshComponent.h>
#include <PigeonBase/Object/Component/Primitive/SkeletalMeshComponent.h>
#include <PigeonBase/Object/Component/Primitive/BezierGrassComponent.h>
#include <PigeonBase/Object/Component/Navigation/NavMeshComponent.h>
#include <PigeonBase/Object/Component/Navigation/NavAgentComponent.h>
#include <PigeonBase/Object/Component/Navigation/NavObstacleComponent.h>

//Physics Test
#include <PhysicsTest/PhysicsTest.h>
#include <PigeonBase/Object/Component/Physics/PhysicsComponent.h>
#include "../../../../../../EngineThirdParty/JoltPhysics/Headers/PhysicsManager.h"
#include "PigeonBase/Object/Controller/CharacterController.h"
#include "Editor/EditorLogManager.h"


namespace PigeonEngine
{
	// The demo's material, named by the path of its own folder. A material that lives in a folder is
	// asked for that way, and the builder resolves the json named after that folder
	// (Projects/Demo/M_DemoUnlit/M_DemoUnlit.json): the json, the hlsl and the textures of one
	// material stay in one directory.
	PE_CONSTEXPR_STATIC const CHAR* MATERIAL_DEMO_FOLDER			= "Projects/Demo/M_DemoUnlit";
	// The emission map the material samples, imported from its own folder on first use. The file
	// name is the asset's identity, which is why it carries the material's name, and the file has
	// to be a format the importer's WIC codecs understand (see BindMaterialDemoTexture).
	PE_CONSTEXPR_STATIC const CHAR* MATERIAL_DEMO_EMISSIVE_ASSET	= "M_DemoUnlit_Emissive";
	// The material's own names as the shader side declares them: the block its parameters live in
	// and the texture it reads. They are the compiler's vocabulary, so the demo spells exactly what
	// the material json declared.
	PE_CONSTEXPR_STATIC const CHAR* MATERIAL_DEMO_BLOCK				= "MaterialParams";
	PE_CONSTEXPR_STATIC const CHAR* MATERIAL_DEMO_EMISSIVE_TEXTURE	= "EmissiveMap";

	/*
	* The MaterialDemo material. The job builder generates and compiles the stage sources of
	* (material x program x vertex axis), then the instance holds the values; the component keeps
	* a non owning pointer, so the instance has to outlive every component that references it -
	* which is what the process wide cache is for. The editor will call exactly this function later.
	*
	* The axis comes from the mesh's own layout, so the request cannot be named before the mesh
	* exists: the caller asks for the mesh first and builds the request from it.
	*/
	static BOOL32 MakeMaterialDemoRequest(const EStaticMeshAsset* InMeshAsset, EShaderAssemblyRequest& OutRequest)
	{
		EShaderVertexFactoryType VertexFactory = EShaderVertexFactoryType::ENGINE_MESH;
		EShaderVertexProfile Profile;
		if (!EShaderJobBuilder::ResolveVertexAxis((InMeshAsset ? InMeshAsset->GetStoragedResource() : nullptr), VertexFactory, Profile))
		{
			return FALSE;
		}

		OutRequest = EShaderAssemblyRequest();
		// The asset is named after the material, the request is named by the material's folder: two
		// namespaces, one material. The shader name is what the compiled asset is stored and looked
		// up under, the material name is what the builder resolves the source files from.
		OutRequest.ShaderName = EName(PE_TEXT("M_DemoUnlit"));
		OutRequest.MaterialName = EString(PE_TEXT(MATERIAL_DEMO_FOLDER));
		OutRequest.ProgramName = EName(PE_TEXT("BasePass"));
		OutRequest.VertexFactory = VertexFactory;
		OutRequest.Profile = Profile;
		return TRUE;
	}

	/*
	* Any thread: the instance for the demo material when it is already assembled, otherwise null
	* after recording one assembly request for the load thread - the caller asks again on its next
	* tick. Resolved as a material rather than as a plain assembly, so that a material this mesh
	* asked for and did not get comes back as the error material: the mesh stays in the frame
	* wearing the marker, which can be found and fixed, where a null instance would leave it
	* invisible and indistinguishable from a mesh that was never assigned anything.
	*/
	static const EShaderInstance* RequireMaterialDemoShaderInstance(const EStaticMeshAsset* InMeshAsset)
	{
		EShaderAssemblyRequest Request;
		if (!MakeMaterialDemoRequest(InMeshAsset, Request))
		{
			return nullptr;
		}
		return REngineShaderInstanceCache::GetManagerSingleton()->RequireMaterialInstance(Request);
	}

	/*
	* The raw file an asset is imported from when its PAsset is not on disk yet. The load thread runs
	* that import as part of serving the request, so nothing here touches the disk on a frame - which
	* is the whole reason the demo asks for its assets instead of loading them.
	*/
	static FAssetImportSource MakeImportSource(const EString& InImportPath, const EString& InImportName, const EString& InImportFileType
		, const BOOL32 InIsFallbackSRGB = FALSE)
	{
		FAssetImportSource ImportSource;
		ImportSource.ImportPath = InImportPath;
		ImportSource.ImportName = InImportName;
		ImportSource.ImportFileType = InImportFileType;
		// Answers only for an image that declares no colour space of its own: a colour map wants
		// sRGB, a data map (roughness, a height) wants linear, and the importer cannot tell a
		// colour map from a data map by looking at the file.
		ImportSource.bFallbackSRGB = InIsFallbackSRGB;
		return ImportSource;
	}

	/*
	* The demo material's base colour texture, published into the instance's parameter set before
	* anything draws with that instance.
	*
	* Asked for rather than loaded, like every other asset here: the first call records a request
	* that imports the tga in the material's own folder on the load thread, and the caller tries
	* again on its next tick. The value goes into the parameter set because that is where a
	* material's runtime resources live - the render side reads them when it binds the material, and
	* a bindless build turns this texture into the descriptor heap index the material's constant
	* buffer carries.
	*
	* Ordering is what makes this safe: it runs before the instance is handed to a component, so the
	* set is never written while a draw reads it.
	*/
	static BOOL32 BindMaterialDemoTexture(const EShaderInstance* InInstance)
	{
		if (!InInstance)
		{
			return FALSE;
		}
		// The importer reads the image through WIC, whose built-in codecs cover jpg and png but not
		// tga: the file in the material's folder has to be one of those, and the same name is what
		// the asset ends up stored under.
		//
		// The fallback colour space stays linear, and that is a decision rather than an omission:
		// this map is the shape of an emission, not a colour to be displayed. The material states
		// the emission in physical units and the map only scales it, so a texel is a 0..1 level -
		// and a WIC export carries no colour space of its own for the importer to read anyway.
		const FAssetImportSource ImportSource = MakeImportSource(
			EString(EBaseSettings::ENGINE_SHADER_SOURCE_DIRECTORY) + EString(PE_TEXT(MATERIAL_DEMO_FOLDER)) + EString(PE_TEXT("/"))
			, EString(PE_TEXT(MATERIAL_DEMO_EMISSIVE_ASSET)), EString(PE_TEXT("jpg")), FALSE);
		const ETexture2DAsset* TextureAsset = nullptr;
		if (!ETextureAssetManager::GetManagerSingleton()->RequireLoadTexture2DAsset(
			EBaseSettings::ENGINE_TEXTURE_PATH, EString(PE_TEXT(MATERIAL_DEMO_EMISSIVE_ASSET)), TextureAsset, &ImportSource))
		{
			return FALSE;	// not imported yet: the component asks again on its next tick
		}
		const RTexture2DResource* TextureResource = (TextureAsset ? TextureAsset->GetRenderResource() : nullptr);
		if (!TextureResource)
		{
			// Imported, but the render thread has not built its gpu object yet: the creation runs as
			// a queued command, so this is the same "ask again" state as a missing asset.
			return FALSE;
		}
		// The instance arrives const because the process wide cache hands it out that way; what this
		// call owns is the value layer, and the render side const_casts the same object for the same
		// reason.
		EShaderParameterSet& Parameters = const_cast<EShaderParameterSet&>(InInstance->GetParameters());
		return Parameters.SetTexture(EName(PE_TEXT(MATERIAL_DEMO_BLOCK)), EName(PE_TEXT(MATERIAL_DEMO_EMISSIVE_TEXTURE))
			, TextureResource->GetResolvedRHITexture());
	}

	/*
	* The demo's sky box: the six faces of the engine's default sky, in the order the cube importer
	* expects. The data lives here because it is demo data - the component it is handed to knows only
	* that it was given an import source.
	*/
	static FAssetImportCubeSource MakeSkyCubeMapImportSource()
	{
		constexpr UINT32 SkyFaceNum = 6u;
		const EString AssetBasePath(EBaseSettings::ENGINE_ASSET_DIRECTORY);
		const EString ImportPath(AssetBasePath + EString(PE_TEXT("EngineTextures/DefaultSkyBox/Sky_001/")));

		FAssetImportCubeSource ImportSource;
		for (UINT32 FaceIndex = 0u; FaceIndex < SkyFaceNum; FaceIndex++)
		{
			ImportSource.ImportPaths.Add(ImportPath);
			ImportSource.ImportFileTypes.Add(EString(PE_TEXT("png")));
		}
		ImportSource.ImportNames.Add(EString(PE_TEXT("Sky_001_Right")));
		ImportSource.ImportNames.Add(EString(PE_TEXT("Sky_001_Left")));
		ImportSource.ImportNames.Add(EString(PE_TEXT("Sky_001_Top")));
		ImportSource.ImportNames.Add(EString(PE_TEXT("Sky_001_Bottom")));
		ImportSource.ImportNames.Add(EString(PE_TEXT("Sky_001_Forward")));
		ImportSource.ImportNames.Add(EString(PE_TEXT("Sky_001_Back")));
		// A sky box is colour, and these faces are 8 bit PNGs: when a face declares its own
		// colour space that statement decides, and this fallback answers for the ones that
		// declare nothing (the .tga siblings in the same folder carry no colour information).
		ImportSource.bFallbackSRGB = TRUE;
		return ImportSource;
	}

	PLevelActor::PLevelActor()
	{
		this->SetIsTickable(TRUE);
	}

	PLevelActor::~PLevelActor()
	{
		// this->Destroy();
	}

	void PLevelActor::BeginAddedToScene(PWorld* World)
	{
		PActor::BeginAddedToScene(World);
	}

	void PLevelActor::RemovedFromScene()
	{
		PActor::RemovedFromScene();
	}

	void PLevelActor::UserBeginPlay()
	{
		// Content is a level asset (Assets/Levels/Demo.PLevel); this actor is the world root.
	}

	void PLevelActor::UserEndPlay()
	{

	}

	void PLevelActor::UserTick(FLOAT deltaTime)
	{
		(void)deltaTime;
	}


}
