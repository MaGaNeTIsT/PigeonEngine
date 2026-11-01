#include "SponzaScene.h"

#include <Asset/AssetLoadManager.h>
#include <MeshAsset/MeshAsset.h>
#include <TextureAsset/TextureAsset.h>
#include <ShaderAsset/ShaderInstance.h>
#include <ShaderAsset/ShaderJobBuilder.h>
#include <Shader/EngineShaderInstanceCache.h>
#include <Base/DataStructure/Text/Path.h>
#include <Config/EngineConfig.h>
#include <Editor/EditorLogManager.h>
#include <IO/FileHelper.h>
#include <PigeonBase/Object/Actor.h>
#include <PigeonBase/Object/World/World.h>
#include <PigeonBase/Object/Component/SceneComponent.h>
#include <PigeonBase/Object/Component/Primitive/StaticMeshComponent.h>
#include <RapidJson/Headers/JsonManager.h>

namespace PigeonEngine
{

	namespace
	{
		// Where the scene comes from, and what it is drawn with. The mesh importer resolves the
		// source's own material and texture names into a manifest beside the assets; these are the
		// parts this side has to name itself.
		PE_CONSTEXPR_STATIC const CHAR*	SPONZA_MESH_FOLDER				= PE_TEXT("EngineModels/SceneModels/Sponza/");
		PE_CONSTEXPR_STATIC const CHAR*	SPONZA_TEXTURE_FOLDER			= PE_TEXT("EngineModels/SceneModels/Sponza/textures/");
		PE_CONSTEXPR_STATIC const CHAR*	SPONZA_SOURCE_NAME				= PE_TEXT("sponza");
		PE_CONSTEXPR_STATIC const CHAR*	SPONZA_SOURCE_TYPE				= PE_TEXT("obj");
		PE_CONSTEXPR_STATIC const CHAR*	SPONZA_ASSET_NAME				= PE_TEXT("Sponza");
		PE_CONSTEXPR_STATIC const CHAR*	SPONZA_MANIFEST_SUFFIX			= PE_TEXT(".Scene.json");
		PE_CONSTEXPR_STATIC const CHAR*	SPONZA_MATERIAL_FOLDER			= PE_TEXT("Projects/Demo/M_Sponza");
		PE_CONSTEXPR_STATIC const CHAR*	SPONZA_MATERIAL_NAME			= PE_TEXT("M_Sponza");
		// The second material: the same program and the same maps, compiled in the Mask surface
		// mode. It is chosen per source material, from the map_d marker the importer recorded.
		PE_CONSTEXPR_STATIC const CHAR*	SPONZA_MASK_MATERIAL_FOLDER		= PE_TEXT("Projects/Demo/M_SponzaMask");
		PE_CONSTEXPR_STATIC const CHAR*	SPONZA_MASK_MATERIAL_NAME		= PE_TEXT("M_SponzaMask");
		PE_CONSTEXPR_STATIC const CHAR*	SPONZA_PROGRAM_NAME				= PE_TEXT("BasePass");
		PE_CONSTEXPR_STATIC const CHAR*	SPONZA_MATERIAL_BLOCK			= PE_TEXT("MaterialParams");
		PE_CONSTEXPR_STATIC const CHAR*	SPONZA_BASE_COLOR_MAP_FIELD		= PE_TEXT("BaseColorMap");
		PE_CONSTEXPR_STATIC const CHAR*	SPONZA_NORMAL_MAP_FIELD			= PE_TEXT("NormalMap");
		PE_CONSTEXPR_STATIC const CHAR*	SPONZA_MASK_MAP_FIELD			= PE_TEXT("MaskMap");
		PE_CONSTEXPR_STATIC const CHAR*	SPONZA_BASE_COLOR_TINT_FIELD	= PE_TEXT("BaseColorTint");
		PE_CONSTEXPR_STATIC const CHAR*	SPONZA_HAS_BASE_COLOR_MAP_FIELD	= PE_TEXT("HasBaseColorMap");
		PE_CONSTEXPR_STATIC const CHAR*	SPONZA_HAS_NORMAL_MAP_FIELD		= PE_TEXT("HasNormalMap");
		PE_CONSTEXPR_STATIC const CHAR*	SPONZA_HAS_MASK_MAP_FIELD		= PE_TEXT("HasMaskMap");
		PE_CONSTEXPR_STATIC const CHAR*	SPONZA_MASK_THRESHOLD_FIELD		= PE_TEXT("MaskThreshold");
		// The source's mask maps are missing files; its albedo alpha carries the same cutout, so
		// the threshold is the one that separates that alpha. It is a scene choice, not a material
		// constant, which is why the panel can still override the value on the instance.
		PE_CONSTEXPR_STATIC FLOAT		SPONZA_MASK_THRESHOLD			= 0.5f;

		// The building is authored in world space with its floor near y = 0, which is exactly where
		// the test level's ground plane puts its top face. A few units up - centimetres at this
		// model's scale - orders the two surfaces without moving anything an eye would notice.
		PE_CONSTEXPR_STATIC FLOAT		SPONZA_SCENE_OFFSET_Y			= 3.f;

		// One object the source file authored, as the import recorded it.
		struct FSponzaSceneObject
		{
		public:
			EString	MeshAssetName;
			EString	MeshName;
			EString	MaterialName;
			Color3	DiffuseColor	= Color3(1.f, 1.f, 1.f);
			// The source's spelling of the texture files, folder and extension included; empty when
			// the material names none.
			EString	DiffuseMapPath;
			EString	NormalMapPath;
			// The source's coverage map. The files are missing in this asset, so the value is used
			// as the marker of a masked material and the albedo alpha is the actual coverage.
			EString	MaskMapPath;
			BOOL32	bMasked			= FALSE;
		};

		// One material instance, shared by every object the source assigned that material to. The
		// program belongs to the material asset, so only the values differ - which is what an
		// instance is for, and why the scene costs one compiled material rather than one per object.
		struct FSponzaSceneMaterial
		{
		public:
			EString				MaterialName;
			// Owned by the instance cache (CreateOwnedInstance), so it lives exactly as long as the
			// session and is released with it: a component holds a non owning pointer to it.
			EShaderInstance*	Instance		= nullptr;
			Color3				DiffuseColor	= Color3(1.f, 1.f, 1.f);
			EString				DiffuseMapPath;
			EString				NormalMapPath;
			// Which material asset the instance was cloned from. A masked source material uses the
			// Mask surface mode material; the rest use the opaque one.
			BOOL32				bMasked			= FALSE;
			BOOL32				bReady			= FALSE;
		};

		// The build spans frames - the import, one asset load per object and per texture, and the
		// material assembly all arrive asynchronously - so its state lives at file scope. There is
		// one Sponza scene per process; the material instances it uses are the cache's, not this
		// state's, which is what keeps them out of the shutdown residue check.
		struct FSponzaSceneBuild
		{
		public:
			BOOL32							bBuilderStarted	= FALSE;
			BOOL32							bDone			= FALSE;
			BOOL32							bFailed			= FALSE;
			TArray<FSponzaSceneObject>		Objects;
			TArray<const EStaticMeshAsset*>	MeshAssets;
			TArray<FSponzaSceneMaterial>	Materials;
		};
		static FSponzaSceneBuild _GSponzaSceneState;

		static const FSponzaSceneMaterial* FindSponzaSceneMaterial(const EString& InMaterialName)
		{
			for (UINT32 MaterialIndex = 0u; MaterialIndex < _GSponzaSceneState.Materials.Num<UINT32>(); MaterialIndex++)
			{
				const FSponzaSceneMaterial& Material = _GSponzaSceneState.Materials[MaterialIndex];
				if (Material.MaterialName == InMaterialName)
				{
					return &Material;
				}
			}
			return nullptr;
		}

		// Reads the object list the mesh importer wrote beside the assets. Called once, after the base
		// mesh has been imported - the manifest is written by that same import.
		static BOOL32 LoadSponzaSceneManifest(TArray<FSponzaSceneObject>& OutObjects)
		{
			EString ManifestPath(EBaseSettings::ENGINE_MESH_PATH);
			ManifestPath += EString(PE_TEXT(SPONZA_ASSET_NAME));
			ManifestPath += EString(PE_TEXT(SPONZA_MANIFEST_SUFFIX));

			EString ManifestText;
			if (EFileHelper::ReadFileAsString(ManifestPath, ManifestText) == FALSE)
			{
				PE_LOG_ERROR(EString(PE_TEXT("[SponzaScene] No scene manifest at '")) + ManifestPath
					+ EString(PE_TEXT("'; the mesh importer writes one beside the assets it saves.")));
				return FALSE;
			}

			CJsonObject RootObject(ManifestText);
			TArray<CJsonObject*> MeshObjects;
			if (RootObject.GetObjectArrayField(EString(PE_TEXT("Meshes")), MeshObjects) == FALSE)
			{
				PE_LOG_ERROR(EString(PE_TEXT("[SponzaScene] The scene manifest at '")) + ManifestPath
					+ EString(PE_TEXT("' carries no mesh list.")));
				return FALSE;
			}

			OutObjects.Empty();
			for (UINT32 MeshIndex = 0u; MeshIndex < MeshObjects.Num<UINT32>(); MeshIndex++)
			{
				CJsonObject* MeshObject = MeshObjects[MeshIndex];
				FSponzaSceneObject& NewObject = OutObjects.AddDefaultGetRef();
				MeshObject->GetStringField(EString(PE_TEXT("Asset")), NewObject.MeshAssetName);
				MeshObject->GetStringField(EString(PE_TEXT("Mesh")), NewObject.MeshName);
				MeshObject->GetStringField(EString(PE_TEXT("Material")), NewObject.MaterialName);
				MeshObject->GetStringField(EString(PE_TEXT("DiffuseMap")), NewObject.DiffuseMapPath);
				MeshObject->GetStringField(EString(PE_TEXT("NormalMap")), NewObject.NormalMapPath);
				MeshObject->GetStringField(EString(PE_TEXT("MaskMap")), NewObject.MaskMapPath);
				// A source material that names a coverage map is the one that wants the Mask
				// surface mode. The file is not loaded: this asset's masks are missing and its
				// albedo alpha is the coverage.
				NewObject.bMasked = ((NewObject.MaskMapPath.Length() > 0u) ? TRUE : FALSE);

				TArray<FLOAT> DiffuseColor;
				if ((MeshObject->GetFloatArrayField(EString(PE_TEXT("DiffuseColor")), DiffuseColor) != FALSE)
					&& (DiffuseColor.Num<UINT32>() >= 3u))
				{
					NewObject.DiffuseColor = Color3(DiffuseColor[0u], DiffuseColor[1u], DiffuseColor[2u]);
				}
				// The importer hands back its own objects; the manifest copies everything into the
				// strings above, so these are this function's to free.
				delete MeshObject;
			}
			return (OutObjects.Num<UINT32>() > 0u);
		}

		// The GPU texture of one source image, or null while it is still being imported or its device
		// object is still queued. The name the asset is saved under is the file name without its
		// folder and extension, which is also what the loader looks the .PAsset up under.
		static const IRRHITexture* RequireSponzaTexture(const EString& InMapPath, const BOOL32 InIsSRGB)
		{
			if (InMapPath.Length() == 0u)
			{
				return nullptr;
			}
			const EString TextureName = EPath::GetFileNameWithoutExtension(InMapPath);
			const EString TextureType = EPath::GetExtension(InMapPath);
			if ((TextureName.Length() == 0u) || (TextureType.Length() == 0u))
			{
				return nullptr;
			}

			FAssetImportSource ImportSource;
			ImportSource.ImportPath = EString(EBaseSettings::ENGINE_ASSET_DIRECTORY) + EString(PE_TEXT(SPONZA_TEXTURE_FOLDER));
			ImportSource.ImportName = TextureName;
			ImportSource.ImportFileType = TextureType;
			// A colour map is read as colour and a normal map as data. A TGA declares no colour space
			// of its own, which is exactly the case this fallback exists for.
			ImportSource.bFallbackSRGB = InIsSRGB;

			const ETexture2DAsset* TextureAsset = nullptr;
			if (ETextureAssetManager::GetManagerSingleton()->RequireLoadTexture2DAsset(
				EBaseSettings::ENGINE_TEXTURE_PATH, TextureName, TextureAsset, &ImportSource) == FALSE)
			{
				return nullptr;
			}
			const RTexture2DResource* TextureResource = ((TextureAsset != nullptr) ? TextureAsset->GetRenderResource() : nullptr);
			if (!TextureResource)
			{
				return nullptr;
			}
			return TextureResource->GetResolvedRHITexture();
		}

		// The one shared instance of one scene material. Asking for it is what assembles the material -
		// resolve the source, compile it, import the artifacts - and every per material instance below
		// is cloned from the program it resolves. The masked and the opaque material are two assets
		// with the same program, so the request names which one this group of objects needs.
		static const EShaderInstance* RequireSponzaMaterialInstance(const EStaticMeshAsset* InMeshAsset, const BOOL32 InIsMasked)
		{
			// The vertex axis comes from the mesh layout, so the request cannot be named before the
			// mesh exists.
			EShaderVertexFactoryType VertexFactory = EShaderVertexFactoryType::ENGINE_MESH;
			EShaderVertexProfile Profile;
			if (EShaderJobBuilder::ResolveVertexAxis((InMeshAsset ? InMeshAsset->GetStoragedResource() : nullptr)
				, VertexFactory, Profile) == FALSE)
			{
				return nullptr;
			}

			EShaderAssemblyRequest Request;
			Request.ShaderName = EName((InIsMasked != FALSE) ? SPONZA_MASK_MATERIAL_NAME : SPONZA_MATERIAL_NAME);
			Request.MaterialName = EString((InIsMasked != FALSE) ? SPONZA_MASK_MATERIAL_FOLDER : SPONZA_MATERIAL_FOLDER);
			Request.ProgramName = EName(PE_TEXT(SPONZA_PROGRAM_NAME));
			Request.VertexFactory = VertexFactory;
			Request.Profile = Profile;
			return REngineShaderInstanceCache::GetManagerSingleton()->RequireMaterialInstance(Request);
		}

		// Binds one material's textures and values. FALSE while a texture is still arriving, which is
		// what keeps a half bound set from ever being handed to a component.
		static BOOL32 BindSponzaMaterial(FSponzaSceneMaterial& InOutMaterial)
		{
			if (!InOutMaterial.Instance)
			{
				return FALSE;
			}

			const IRRHITexture* BaseColorTexture = nullptr;
			if (InOutMaterial.DiffuseMapPath.Length() > 0u)
			{
				BaseColorTexture = RequireSponzaTexture(InOutMaterial.DiffuseMapPath, TRUE);
				if (!BaseColorTexture)
				{
					return FALSE;
				}
			}
			const IRRHITexture* NormalTexture = nullptr;
			if (InOutMaterial.NormalMapPath.Length() > 0u)
			{
				NormalTexture = RequireSponzaTexture(InOutMaterial.NormalMapPath, FALSE);
				if (!NormalTexture)
				{
					return FALSE;
				}
			}

			EShaderParameterSet& Parameters = InOutMaterial.Instance->GetParameters();
			const EName BlockName(PE_TEXT(SPONZA_MATERIAL_BLOCK));
			if (BaseColorTexture)
			{
				Parameters.SetTexture(BlockName, EName(PE_TEXT(SPONZA_BASE_COLOR_MAP_FIELD)), BaseColorTexture);
			}
			if (NormalTexture)
			{
				Parameters.SetTexture(BlockName, EName(PE_TEXT(SPONZA_NORMAL_MAP_FIELD)), NormalTexture);
			}

			// A mapped material is drawn with its map as authored; the source material's own colour
			// stands in only where there is no map to carry it.
			const Color3 Tint = ((BaseColorTexture != nullptr) ? Color3(1.f, 1.f, 1.f) : InOutMaterial.DiffuseColor);
			const FLOAT TintScalars[4] = { Tint.r, Tint.g, Tint.b, 1.f };
			Parameters.SetFloatArray(BlockName, EName(PE_TEXT(SPONZA_BASE_COLOR_TINT_FIELD)), TintScalars, 4u);
			// The two switches that keep an absent map from being sampled as if it were there: a black
			// base colour map would make the object black, and a black normal map would turn every
			// face away from the light.
			Parameters.SetScalar(BlockName, EName(PE_TEXT(SPONZA_HAS_BASE_COLOR_MAP_FIELD)), ((BaseColorTexture != nullptr) ? 1.f : 0.f));
			Parameters.SetScalar(BlockName, EName(PE_TEXT(SPONZA_HAS_NORMAL_MAP_FIELD)), ((NormalTexture != nullptr) ? 1.f : 0.f));

			// The mask material's coverage. The source's map_d names a *_mask.tga that is not on
			// disk, while this asset's albedo alpha carries the same cutout, so the albedo map is
			// bound as the coverage map and its alpha is what the shader thresholds. HasMaskMap
			// keeps an absent albedo from discarding the whole surface.
			if (InOutMaterial.bMasked != FALSE)
			{
				if (BaseColorTexture)
				{
					Parameters.SetTexture(BlockName, EName(PE_TEXT(SPONZA_MASK_MAP_FIELD)), BaseColorTexture);
				}
				Parameters.SetScalar(BlockName, EName(PE_TEXT(SPONZA_HAS_MASK_MAP_FIELD)), ((BaseColorTexture != nullptr) ? 1.f : 0.f));
				Parameters.SetScalar(BlockName, EName(PE_TEXT(SPONZA_MASK_THRESHOLD_FIELD)), SPONZA_MASK_THRESHOLD);
			}

			InOutMaterial.bReady = TRUE;
			return TRUE;
		}

		// One step of the build. TRUE means the scene is complete, or has been reported as impossible
		// and is not worth asking about again; FALSE means something it needs is still being produced,
		// and the next frame asks again.
		static BOOL32 StepSponzaSceneBuild(PWorld& InWorld)
		{
			if ((_GSponzaSceneState.bDone != FALSE) || (_GSponzaSceneState.bFailed != FALSE))
			{
				return TRUE;
			}

			// ---- The base mesh. Asking for it is what runs the import, and the import writes every
			// sibling asset and the manifest in one go.
			if (_GSponzaSceneState.Objects.Num() == 0u)
			{
				FAssetImportSource ImportSource;
				ImportSource.ImportPath = EString(EBaseSettings::ENGINE_ASSET_DIRECTORY) + EString(PE_TEXT(SPONZA_MESH_FOLDER));
				ImportSource.ImportName = EString(PE_TEXT(SPONZA_SOURCE_NAME));
				ImportSource.ImportFileType = EString(PE_TEXT(SPONZA_SOURCE_TYPE));
				// One object per authored object is the whole point of this scene, so the importer must
				// neither fold the source's submeshes nor merge meshes that share a material.
				ImportSource.bIsCombineSubmeshes = FALSE;
				ImportSource.bIsOptimizeMeshes = FALSE;

				const EStaticMeshAsset* BaseMeshAsset = nullptr;
				if (EMeshAssetManager::GetManagerSingleton()->RequireLoadStaticMeshAsset(
					EBaseSettings::ENGINE_MESH_PATH, EString(PE_TEXT(SPONZA_ASSET_NAME)), BaseMeshAsset, &ImportSource) == FALSE)
				{
					return FALSE;
				}
				if (LoadSponzaSceneManifest(_GSponzaSceneState.Objects) == FALSE)
				{
					_GSponzaSceneState.bFailed = TRUE;
					return TRUE;
				}
			}

			// ---- One asset per object. The import above already wrote them all to disk, so these
			// only load; asking in the manifest's order keeps asset and object aligned by index.
			if (_GSponzaSceneState.MeshAssets.Num<UINT32>() != _GSponzaSceneState.Objects.Num<UINT32>())
			{
				TArray<const EStaticMeshAsset*> MeshAssets;
				for (UINT32 ObjectIndex = 0u; ObjectIndex < _GSponzaSceneState.Objects.Num<UINT32>(); ObjectIndex++)
				{
					const EStaticMeshAsset* MeshAsset = nullptr;
					if ((EMeshAssetManager::GetManagerSingleton()->RequireLoadStaticMeshAsset(
						EBaseSettings::ENGINE_MESH_PATH, _GSponzaSceneState.Objects[ObjectIndex].MeshAssetName, MeshAsset) == FALSE)
						|| (!MeshAsset))
					{
						return FALSE;
					}
					MeshAssets.Add(MeshAsset);
				}
				_GSponzaSceneState.MeshAssets = MeshAssets;
			}

			// ---- One instance per source material, cloned from the assembled material: the program
			// is shared, the values are per instance. The cache owns the copies, which is what keeps
			// the constant buffers they allocate from outliving the engine's shutdown.
			//
			// The loop runs until every source material has an instance rather than only on the first
			// pass: the masked material is a second asset and may still be assembling, and returning
			// early must not strand the materials that were not created yet.
			for (UINT32 ObjectIndex = 0u; ObjectIndex < _GSponzaSceneState.Objects.Num<UINT32>(); ObjectIndex++)
			{
				const FSponzaSceneObject& Object = _GSponzaSceneState.Objects[ObjectIndex];
				if (FindSponzaSceneMaterial(Object.MaterialName) != nullptr)
				{
					continue;
				}
				const EShaderInstance* SharedInstance = RequireSponzaMaterialInstance(_GSponzaSceneState.MeshAssets[0u], Object.bMasked);
				if (!SharedInstance)
				{
					return FALSE;
				}
				EShaderInstance* MaterialInstance = REngineShaderInstanceCache::GetManagerSingleton()->CreateOwnedInstance(*SharedInstance);
				if (!MaterialInstance)
				{
					PE_LOG_ERROR(EString(PE_TEXT("[SponzaScene] A material instance could not be created for '"))
						+ Object.MaterialName + EString(PE_TEXT("'.")));
					_GSponzaSceneState.bFailed = TRUE;
					return TRUE;
				}
				FSponzaSceneMaterial& NewMaterial = _GSponzaSceneState.Materials.AddDefaultGetRef();
				NewMaterial.MaterialName = Object.MaterialName;
				NewMaterial.Instance = MaterialInstance;
				NewMaterial.DiffuseColor = Object.DiffuseColor;
				NewMaterial.DiffuseMapPath = Object.DiffuseMapPath;
				NewMaterial.NormalMapPath = Object.NormalMapPath;
				NewMaterial.bMasked = Object.bMasked;
			}

			// ---- Each material's textures and values.
			for (UINT32 MaterialIndex = 0u; MaterialIndex < _GSponzaSceneState.Materials.Num<UINT32>(); MaterialIndex++)
			{
				FSponzaSceneMaterial& Material = _GSponzaSceneState.Materials[MaterialIndex];
				if (Material.bReady != FALSE)
				{
					continue;
				}
				if (BindSponzaMaterial(Material) == FALSE)
				{
					return FALSE;
				}
			}

			// ---- The geometry. One actor and one component per object, at the source's own transform:
			// the file is authored in world space, so only the scene offset is applied.
			for (UINT32 ObjectIndex = 0u; ObjectIndex < _GSponzaSceneState.Objects.Num<UINT32>(); ObjectIndex++)
			{
				const FSponzaSceneObject& Object = _GSponzaSceneState.Objects[ObjectIndex];

				PActor* NewActor = new PActor();
				NewActor->SetIsTickable(TRUE);
				POBJ_DEBUGNAME_SET(NewActor, Object.MeshName);
				InWorld.AddActor(NewActor);

				PStaticMeshComponent* NewMeshComponent = new PStaticMeshComponent();
				NewMeshComponent->SetIsTickable(TRUE);
				NewActor->AddComponent(NewMeshComponent);
				NewMeshComponent->SetMeshAsset(_GSponzaSceneState.MeshAssets[ObjectIndex]);
				if (const FSponzaSceneMaterial* Material = FindSponzaSceneMaterial(Object.MaterialName))
				{
					NewMeshComponent->SetMaterialInstance(Material->Instance);
				}
				NewActor->SetActorLocation(Vector3(0.f, SPONZA_SCENE_OFFSET_Y, 0.f));
			}

			_GSponzaSceneState.bDone = TRUE;
			PE_LOG_LOG(EString(PE_TEXT("[SponzaScene] Built ")) + ToString(_GSponzaSceneState.Objects.Num<UINT32>())
				+ EString(PE_TEXT(" objects with ")) + ToString(_GSponzaSceneState.Materials.Num<UINT32>())
				+ EString(PE_TEXT(" material instances.")));
#if _EDITOR_ONLY
			// The mapping the draws actually use, one line per material with the number of objects
			// that ended up on it. The manifest is the import's answer; this is the runtime's, and a
			// texture that lands on the wrong object is normally a difference between the two.
			for (UINT32 MaterialIndex = 0u; MaterialIndex < _GSponzaSceneState.Materials.Num<UINT32>(); MaterialIndex++)
			{
				const FSponzaSceneMaterial& Material = _GSponzaSceneState.Materials[MaterialIndex];
				UINT32 ObjectNum = 0u;
				for (UINT32 ObjectIndex = 0u; ObjectIndex < _GSponzaSceneState.Objects.Num<UINT32>(); ObjectIndex++)
				{
					if (_GSponzaSceneState.Objects[ObjectIndex].MaterialName == Material.MaterialName)
					{
						ObjectNum++;
					}
				}
				PE_LOG_LOG(EString(PE_TEXT("[SponzaScene] material '")) + Material.MaterialName
					+ EString(PE_TEXT("' on ")) + ToString(ObjectNum)
					+ EString(PE_TEXT(" object(s), base colour '")) + Material.DiffuseMapPath
					+ EString(PE_TEXT("', normal '")) + Material.NormalMapPath + EString(PE_TEXT("'.")));
			}
#endif	// _EDITOR_ONLY
			return TRUE;
		}
	}

	void BuildDemoSponzaScene(PWorld& InWorld)
	{
		if ((_GSponzaSceneState.bBuilderStarted != FALSE) || (_GSponzaSceneState.bDone != FALSE) || (_GSponzaSceneState.bFailed != FALSE))
		{
			return;
		}
		_GSponzaSceneState.bBuilderStarted = TRUE;

		// The build waits on assets, so it cannot complete inside this call. It runs as a deferred step
		// on a component of its own: that is the engine's "react when this asset exists" mechanism, it
		// is retried every frame until it reports done, and it dies with the component.
		PActor* BuilderActor = new PActor();
		BuilderActor->SetIsTickable(TRUE);
		POBJ_DEBUGNAME_SET(BuilderActor, "SponzaSceneBuilder");
		InWorld.AddActor(BuilderActor);

		PSceneComponent* BuilderComponent = new PSceneComponent();
		BuilderComponent->SetIsTickable(TRUE);
		BuilderActor->AddComponent(BuilderComponent);

		// The world outlives the component it owns, so the pointer is safe for as long as the step can
		// run - and a step that outlived its component could not run at all.
		PWorld* WorldPointer = &InWorld;
		BuilderComponent->AddDeferredApply([WorldPointer]() -> BOOL32
		{
			return StepSponzaSceneBuild(*WorldPointer);
		});
	}

}	// namespace PigeonEngine
