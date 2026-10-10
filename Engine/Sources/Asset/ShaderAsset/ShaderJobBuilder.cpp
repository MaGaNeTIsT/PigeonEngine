#include "ShaderJobBuilder.h"

#include <Config/EngineConfig.h>
#include <Config/ErrorCaption.h>
#include <IO/FileHelper.h>
#include <Editor/EditorLogManager.h>
#include <MeshAsset/MeshAsset.h>
#include <ShaderAsset/ShaderAsset.h>

#include <document.h>
#include <prettywriter.h>
#include <stringbuffer.h>

namespace PigeonEngine
{

	// ---------------------------------------------------------------------------
	// Constants
	// ---------------------------------------------------------------------------

	// The generated component every material bearing program references by token. The
	// json is generated (it is engine sampler configuration), the hlsl next to it is hand
	// written source: there is no code to generate, so a missing one is an error and
	// never a reason to write into the source tree.
	PE_CONSTEXPR_STATIC const CHAR* SHADER_JOB_GENERATED_COMPONENT_DIRECTORY	= "Components/";
	PE_CONSTEXPR_STATIC const CHAR* SHADER_JOB_MATERIAL_SAMPLERS_JSON		= "C_MaterialSamplers.json";
	PE_CONSTEXPR_STATIC const CHAR* SHADER_JOB_MATERIAL_SAMPLERS_HLSL		= "Components/C_MaterialSamplers.hlsl";
	PE_CONSTEXPR_STATIC const CHAR* SHADER_JOB_NEUTRAL_MATERIAL_JSON		= "Materials/M_EngineNeutralVertex.json";
	PE_CONSTEXPR_STATIC const CHAR* SHADER_JOB_MATERIAL_FUNCTIONS_DEFINE	= "MATERIAL_HAS_FUNCTIONS";

	// The `$` tokens a program descriptor may use (design Section10.3, plus the vertex variant
	// the validation driver introduced). Every one of them expands to a RootDir relative
	// path, because that is what the compiler resolves paths inside an entry json against.
	PE_CONSTEXPR_STATIC const CHAR* SHADER_JOB_TOKEN_MATERIAL			= "$Material";
	PE_CONSTEXPR_STATIC const CHAR* SHADER_JOB_TOKEN_MATERIAL_VERTEX	= "$MaterialVertex";
	PE_CONSTEXPR_STATIC const CHAR* SHADER_JOB_TOKEN_DOMAIN				= "$Domain";
	PE_CONSTEXPR_STATIC const CHAR* SHADER_JOB_TOKEN_GENERATED_COMPONENT = "$GeneratedComponents/";

	// Stage level binding blocks a program descriptor may declare. The entry json carries
	// them at its top level, where they pack after the components (C5).
	PE_CONSTEXPR_STATIC const CHAR* SHADER_JOB_BINDING_BLOCKS[] =
	{
		"ConstantBuffers", "Samplers", "Textures", "Resources", "RWTextures", "RWResources"
	};

	// ---------------------------------------------------------------------------
	// Paths
	// ---------------------------------------------------------------------------

	// Defined with the assembly pipeline below; declared here because MakeAssembly, which
	// sits between the two, already needs the target's shader model.
	static EShaderModelType GetShaderModelForActiveBackend();

	static EString MakeTrailingSlash(const EString& InPath)
	{
		if ((InPath.Length() == 0u) || (InPath[InPath.Length() - 1u] == '/'))
		{
			return InPath;
		}
		return (InPath + EString(PE_TEXT("/")));
	}

	static EString GetShaderSourceRoot()	{ return MakeTrailingSlash(EString(EEngineSettings::ENGINE_SHADER_SOURCE_DIR)); }
	static EString GetShaderOutputRoot()	{ return MakeTrailingSlash(EString(EEngineSettings::ENGINE_SHADER_OUTPUT_ROOT_DIR)); }
	static EString GetShaderGeneratedRoot()	{ return MakeTrailingSlash(EString(EEngineSettings::ENGINE_SHADER_GENERATED_ROOT_DIR)); }

	static void SplitPathParts(const EString& InPath, TArray<EString>& OutParts)
	{
		OutParts.Empty();
		EString CurrentPart;
		for (UINT32 Index = 0u; Index < InPath.Length(); Index++)
		{
			const CHAR Character = InPath[Index];
			if ((Character == '/') || (Character == '\\'))
			{
				if (CurrentPart.Length() > 0u)
				{
					OutParts.Add(CurrentPart);
					CurrentPart = EString();
				}
			}
			else
			{
				CurrentPart += EString::FromChar(Character);
			}
		}
		if (CurrentPart.Length() > 0u)
		{
			OutParts.Add(CurrentPart);
		}
	}

	/*
	* The walk from RootDir down to an absolute path. The compiler resolves every path
	* inside an entry json against RootDir, and generated files live under Build/, so the
	* entry json has to spell that walk out; nothing here is hardcoded to a depth, which is
	* what makes the generated and the source root relocatable independently.
	*/
	static EString MakePathRelativeTo(const EString& InBaseDirectory, const EString& InTargetPath)
	{
		TArray<EString> BaseParts;
		TArray<EString> TargetParts;
		SplitPathParts(InBaseDirectory, BaseParts);
		SplitPathParts(InTargetPath, TargetParts);

		const UINT32 BaseNum = BaseParts.Num<UINT32>();
		const UINT32 TargetNum = TargetParts.Num<UINT32>();
		UINT32 CommonNum = 0u;
		while ((CommonNum < BaseNum) && (CommonNum < TargetNum) && (BaseParts[CommonNum] == TargetParts[CommonNum]))
		{
			CommonNum++;
		}

		EString Result;
		for (UINT32 Index = CommonNum; Index < BaseNum; Index++)
		{
			Result += EString(PE_TEXT("../"));
		}
		for (UINT32 Index = CommonNum; Index < TargetNum; Index++)
		{
			if (Index > CommonNum)
			{
				Result += EString(PE_TEXT("/"));
			}
			Result += TargetParts[Index];
		}
		return Result;
	}

	// ---------------------------------------------------------------------------
	// Json reading
	// ---------------------------------------------------------------------------

	static BOOL32 ReadJsonDocument(const EString& InAbsolutePath, rapidjson::Document& OutDocument)
	{
		EString JsonText;
		if (!EFileHelper::ReadFileAsString(InAbsolutePath, JsonText))
		{
			PE_LOG_ERROR(EString(PE_TEXT("[ShaderJobBuilder] Cannot read: ")) + InAbsolutePath);
			return FALSE;
		}
		OutDocument.Parse(*JsonText);
		if (OutDocument.HasParseError())
		{
			PE_LOG_ERROR(EString(PE_TEXT("[ShaderJobBuilder] Json parse error: ")) + InAbsolutePath);
			return FALSE;
		}
		return TRUE;
	}

	static const CHAR* ReadJsonString(const rapidjson::Value& InValue, const CHAR* InKeyName, const CHAR* InDefaultValue)
	{
		if ((!InValue.HasMember(InKeyName)) || (!InValue[InKeyName].IsString()))
		{
			return InDefaultValue;
		}
		return InValue[InKeyName].GetString();
	}

	static INT32 ReadJsonInt(const rapidjson::Value& InValue, const CHAR* InKeyName, const INT32 InDefaultValue)
	{
		if ((!InValue.HasMember(InKeyName)) || (!InValue[InKeyName].IsInt()))
		{
			return InDefaultValue;
		}
		return InValue[InKeyName].GetInt();
	}

	// ---------------------------------------------------------------------------
	// Material and domain
	// ---------------------------------------------------------------------------

	struct EShaderJobMaterialInfo
	{
	public:
		EString							DomainName;
		INT32							SurfaceMode		= 0;
		BOOL32							bVertexOps		= FALSE;
		TArray<EShaderVariantDefine>	VariantDefines;
	};

	static BOOL32 TranslateSurfaceModeFromString(const EString& InModeName, INT32& OutSurfaceMode)
	{
		// Mirrors the surface modes the shader library and the validation driver use;
		// append only, the value ends up in the variant name and the axis defines.
		if (InModeName == EString(PE_TEXT("Opaque")))		{ OutSurfaceMode = 0; return TRUE; }
		if (InModeName == EString(PE_TEXT("Mask")))			{ OutSurfaceMode = 1; return TRUE; }
		if (InModeName == EString(PE_TEXT("Translucent")))	{ OutSurfaceMode = 2; return TRUE; }
		PE_LOG_ERROR(EString(PE_TEXT("[ShaderJobBuilder] Unknown surface mode: ")) + InModeName);
		return FALSE;
	}

	static BOOL32 TranslateDomainToNumber(const EString& InDomainName, INT32& OutDomainNumber)
	{
		// Mirrors Public/Material/MaterialDomainTypes.hlsl, whose ids start at Unlit = 0.
		// The number only shows up in the variant name today (the domain json declares
		// MATERIAL_DOMAIN itself), but the two tables still have to move together.
		OutDomainNumber = 0;
		if (InDomainName.Length() == 0u)
		{
			// A program with no $Domain token (the fullscreen copy pass) has no domain.
			return TRUE;
		}
		if (InDomainName == EString(PE_TEXT("Unlit")))		{ OutDomainNumber = 0; return TRUE; }
		if (InDomainName == EString(PE_TEXT("DefaultLit")))	{ OutDomainNumber = 1; return TRUE; }
		PE_LOG_ERROR(EString(PE_TEXT("[ShaderJobBuilder] Unknown material domain: ")) + InDomainName);
		return FALSE;
	}

	static EString MakeDomainJsonPath(const EString& InDomainName)
	{
		return (EString(PE_TEXT("Domains/D_")) + InDomainName + EString(PE_TEXT("/D_")) + InDomainName + EString(PE_TEXT(".json")));
	}

	// The material json is the single source of the domain, the surface mode and the
	// material variant bits (design Section10.2), so it is read rather than repeated by callers.
	static BOOL32 ReadMaterialInfo(const EString& InMaterialJsonPath, EShaderJobMaterialInfo& OutInfo)
	{
		rapidjson::Document MaterialDocument;
		if (!ReadJsonDocument(GetShaderSourceRoot() + InMaterialJsonPath, MaterialDocument))
		{
			return FALSE;
		}
		OutInfo = EShaderJobMaterialInfo();
		if ((!MaterialDocument.HasMember("PEMaterial")) || (!MaterialDocument["PEMaterial"].IsObject()))
		{
			PE_LOG_ERROR(EString(PE_TEXT("[ShaderJobBuilder] Material json has no PEMaterial section: ")) + InMaterialJsonPath);
			return FALSE;
		}
		const rapidjson::Value& PEMaterialValue = MaterialDocument["PEMaterial"];
		OutInfo.DomainName = EString(ReadJsonString(PEMaterialValue, "Domain", ""));
		if (!TranslateSurfaceModeFromString(EString(ReadJsonString(PEMaterialValue, "SurfaceMode", "Opaque")), OutInfo.SurfaceMode))
		{
			return FALSE;
		}
		if ((PEMaterialValue.HasMember("VertexOps")) && (PEMaterialValue["VertexOps"].IsBool()))
		{
			OutInfo.bVertexOps = (PEMaterialValue["VertexOps"].GetBool() ? TRUE : FALSE);
		}
		if ((PEMaterialValue.HasMember("VariantDefines")) && (PEMaterialValue["VariantDefines"].IsArray()))
		{
			for (const auto& DefineValue : PEMaterialValue["VariantDefines"].GetArray())
			{
				if (!DefineValue.IsString())
				{
					continue;
				}
				EShaderVariantDefine Define;
				Define.Name = EName(DefineValue.GetString());
				Define.Value = 1;
				OutInfo.VariantDefines.Add(Define);
			}
		}
		return TRUE;
	}

	static BOOL32 ResolveMaterialJson(const EString& InMaterialName, EString& OutMaterialJsonPath)
	{
		// Artist templates live in Materials/Templates/, engine materials directly in
		// Materials/; the first one that exists wins, which is how the validation driver
		// resolved them too.
		const EString SourceRoot = GetShaderSourceRoot();
		const EString TemplateJsonPath = EString(PE_TEXT("Materials/Templates/")) + InMaterialName + EString(PE_TEXT(".json"));
		if (EFileHelper::IsFileExists(SourceRoot + TemplateJsonPath))
		{
			OutMaterialJsonPath = TemplateJsonPath;
			return TRUE;
		}
		const EString MaterialJsonPath = EString(PE_TEXT("Materials/")) + InMaterialName + EString(PE_TEXT(".json"));
		if (EFileHelper::IsFileExists(SourceRoot + MaterialJsonPath))
		{
			OutMaterialJsonPath = MaterialJsonPath;
			return TRUE;
		}
		PE_LOG_ERROR(EString(PE_TEXT("[ShaderJobBuilder] No material json for: ")) + InMaterialName);
		return FALSE;
	}

	// ---------------------------------------------------------------------------
	// Program reading
	// ---------------------------------------------------------------------------

	struct EShaderJobStageTokens
	{
	public:
		BOOL32	bMaterial		= FALSE;
		BOOL32	bMaterialVertex	= FALSE;
		BOOL32	bDomain			= FALSE;
	};

	static void ReadStageTokens(const rapidjson::Value& InStageValue, EShaderJobStageTokens& OutTokens)
	{
		OutTokens = EShaderJobStageTokens();
		if ((!InStageValue.HasMember("Components")) || (!InStageValue["Components"].IsArray()))
		{
			return;
		}
		for (const auto& ComponentValue : InStageValue["Components"].GetArray())
		{
			if (!ComponentValue.IsObject())
			{
				continue;
			}
			const EString JsonFile(ReadJsonString(ComponentValue, "JsonFile", ""));
			if (JsonFile == EString(SHADER_JOB_TOKEN_MATERIAL))					{ OutTokens.bMaterial = TRUE; }
			else if (JsonFile == EString(SHADER_JOB_TOKEN_MATERIAL_VERTEX))		{ OutTokens.bMaterialVertex = TRUE; }
			else if (JsonFile == EString(SHADER_JOB_TOKEN_DOMAIN))				{ OutTokens.bDomain = TRUE; }
		}
	}

	// The stage the assembly is generating, out of the program descriptor's stage list.
	static const rapidjson::Value* FindProgramStage(const rapidjson::Document& InProgramDocument, const EShaderStageType InStage)
	{
		if ((!InProgramDocument.HasMember("Stages")) || (!InProgramDocument["Stages"].IsArray()))
		{
			return nullptr;
		}
		for (const auto& StageValue : InProgramDocument["Stages"].GetArray())
		{
			if (!StageValue.IsObject())
			{
				continue;
			}
			EShaderStageType Stage = EShaderStageType::SHADER_STAGE_VERTEX;
			if (!ParseShaderStageFromString(EString(ReadJsonString(StageValue, "Stage", "")), Stage))
			{
				continue;
			}
			if (Stage == InStage)
			{
				return &StageValue;
			}
		}
		return nullptr;
	}

	static BOOL32 ReadProgramDocument(const EName& InProgramName, rapidjson::Document& OutProgramDocument)
	{
		const EString ProgramJsonPath = EString(PE_TEXT("Programs/P_")) + InProgramName.ToString() + EString(PE_TEXT(".json"));
		return ReadJsonDocument(GetShaderSourceRoot() + ProgramJsonPath, OutProgramDocument);
	}

	// ---------------------------------------------------------------------------
	// Axis
	// ---------------------------------------------------------------------------

	static void AddAxisDefine(TArray<EShaderVariantDefine>& InOutDefines, const CHAR* InName, const INT32 InValue)
	{
		EShaderVariantDefine Define;
		Define.Name = EName(InName);
		Define.Value = InValue;
		InOutDefines.Add(Define);
	}

	BOOL32 EShaderJobBuilder::ResolveVertexAxis(const EMesh* InMesh
		, EShaderVertexFactoryType& OutVertexFactory, EShaderVertexProfile& OutProfile)
	{
		// One factory for every engine mesh. The skin stream is an attribute of the mesh and
		// not a second factory identity, so this value never varies with the mesh and the
		// profile below is the entire answer to "what is different about this one".
		OutVertexFactory = EShaderVertexFactoryType::ENGINE_MESH;
		OutProfile = EShaderVertexProfile();
		if (!InMesh)
		{
			PE_LOG_ERROR(EString(PE_TEXT("[ShaderJobBuilder] Null mesh, no vertex axis to resolve.")));
			return FALSE;
		}

		// Position is the factory's only required stream: every other attribute is optional
		// and falls back to a constant in VFUnpackVertex, so one factory serves meshes whose
		// streams differ (the VF_HAS_* flags and the json input conditions cull the members
		// together). Position has no such fallback, which is why a mesh without it is a hard
		// error rather than a silently zeroed stream.
		if (!InMesh->CheckVertexLayoutPartExist(EVertexLayoutType::MESH_VERTEX, 0u))
		{
			PE_LOG_ERROR(EString(PE_TEXT("[ShaderJobBuilder] Mesh has no position stream, which the engine mesh factory requires: ")) + InMesh->GetMeshName());
			return FALSE;
		}

		UINT32 StreamProfile = SHADER_VERTEX_ATTRIBUTE_POSITION;
		if (InMesh->CheckVertexLayoutPartExist(EVertexLayoutType::MESH_TEXTURECOORD, 0u))
		{
			StreamProfile |= SHADER_VERTEX_ATTRIBUTE_UV0;
		}
		if (InMesh->CheckVertexLayoutPartExist(EVertexLayoutType::MESH_NORMAL, 0u))
		{
			StreamProfile |= SHADER_VERTEX_ATTRIBUTE_NORMAL;
		}
		if (InMesh->CheckVertexLayoutPartExist(EVertexLayoutType::MESH_TANGENT, 0u))
		{
			StreamProfile |= SHADER_VERTEX_ATTRIBUTE_TANGENT;
		}
		if (InMesh->CheckVertexLayoutPartExist(EVertexLayoutType::MESH_TEXTURECOORD, 1u))
		{
			StreamProfile |= SHADER_VERTEX_ATTRIBUTE_UV1;
		}
		if (InMesh->CheckVertexLayoutPartExist(EVertexLayoutType::MESH_COLOR, 0u))
		{
			StreamProfile |= SHADER_VERTEX_ATTRIBUTE_COLOR;
		}

		// Skin is a pair of streams and four influences wide, and both are properties of the
		// data rather than of an asset class: only a skinned mesh ever sets MESH_SKIN, and the
		// element each stream declares has to cover exactly one vertex's influences. A mesh
		// whose skin says something else cannot be served by this factory, which is a hard
		// error like a missing position rather than a profile whose element would read past
		// each vertex.
		if (InMesh->CheckVertexLayoutPartExist(EVertexLayoutType::MESH_SKIN, 0u))
		{
			const ESkinnedMesh* SkinnedMesh = static_cast<const ESkinnedMesh*>(InMesh);
			const UINT32 InfluenceNum = static_cast<UINT32>(SkinnedMesh->GetEffectBoneNum());
			if (InfluenceNum != SHADER_VERTEX_SKIN_INFLUENCE_NUM)
			{
				PE_LOG_ERROR(EString(PE_TEXT("[ShaderJobBuilder] Mesh skin stream carries ")) + ToString(InfluenceNum)
					+ EString(PE_TEXT(" influences, but the engine mesh factory declares ")) + ToString(SHADER_VERTEX_SKIN_INFLUENCE_NUM)
					+ EString(PE_TEXT(": ")) + InMesh->GetMeshName());
				return FALSE;
			}
			StreamProfile |= (SHADER_VERTEX_ATTRIBUTE_SKIN_INDEX | SHADER_VERTEX_ATTRIBUTE_SKIN_WEIGHT);
		}

		OutProfile.StreamProfile = StreamProfile;
		return TRUE;
	}

	EString EShaderJobBuilder::MakeVariantName(const EShaderAssemblyDesc& InDesc)
	{
		// V<VF>_P<Profile>_D<Domain>_S<SurfaceMode> - design Section14.6, with the profile kept in
		// decimal so it decodes against the attribute constants in the header.
		EString Name(PE_TEXT("V"));
		Name += ToString(static_cast<UINT32>(InDesc.VertexFactory));
		Name += EString(PE_TEXT("_P"));
		Name += ToString(InDesc.Profile.StreamProfile);
		Name += EString(PE_TEXT("_D"));
		Name += ToString(InDesc.DomainNumber);
		Name += EString(PE_TEXT("_S"));
		Name += ToString(InDesc.SurfaceMode);
		return Name;
	}

	void EShaderJobBuilder::MakeAxisDefines(const EShaderAssemblyDesc& InDesc, TArray<EShaderVariantDefine>& OutDefines)
	{
		OutDefines.Empty();
		AddAxisDefine(OutDefines, "VF_TYPE", static_cast<INT32>(InDesc.VertexFactory));
		AddAxisDefine(OutDefines, "VF_STREAM_PROFILE", static_cast<INT32>(InDesc.Profile.StreamProfile));
		// Always the complete VF_HAS_* set: a flag that is missing instead of 0 evaluates
		// to 0 in the json input conditions and silently culls that input.
		AddAxisDefine(OutDefines, "VF_HAS_POSITION", (InDesc.Profile.HasPosition() ? 1 : 0));
		AddAxisDefine(OutDefines, "VF_HAS_NORMAL", (InDesc.Profile.HasNormal() ? 1 : 0));
		AddAxisDefine(OutDefines, "VF_HAS_TANGENT", (InDesc.Profile.HasTangent() ? 1 : 0));
		AddAxisDefine(OutDefines, "VF_HAS_BITANGENT", 0);
		AddAxisDefine(OutDefines, "VF_HAS_UV0", (InDesc.Profile.HasUV0() ? 1 : 0));
		AddAxisDefine(OutDefines, "VF_HAS_UV1", (InDesc.Profile.HasUV1() ? 1 : 0));
		AddAxisDefine(OutDefines, "VF_HAS_COLOR", (InDesc.Profile.HasColor() ? 1 : 0));
		AddAxisDefine(OutDefines, "VF_HAS_SKIN_INDEX", (InDesc.Profile.HasSkinIndex() ? 1 : 0));
		AddAxisDefine(OutDefines, "VF_HAS_SKIN_WEIGHT", (InDesc.Profile.HasSkinWeight() ? 1 : 0));
		AddAxisDefine(OutDefines, "VF_HAS_INSTANCE_STREAM", 0);
		AddAxisDefine(OutDefines, "MATERIAL_SURFACE_MODE", InDesc.SurfaceMode);
		for (UINT32 Index = 0u; Index < InDesc.MaterialVariantDefines.Num<UINT32>(); Index++)
		{
			OutDefines.Add(InDesc.MaterialVariantDefines[Index]);
		}
	}

	// ---------------------------------------------------------------------------
	// Assembly resolution
	// ---------------------------------------------------------------------------

	BOOL32 EShaderJobBuilder::MakeAssembly(const EShaderAssemblyRequest& InRequest, EShaderAssemblyDesc& OutDesc)
	{
		if ((!InRequest.ShaderName.IsValid()) || (!InRequest.ProgramName.IsValid()))
		{
			PE_LOG_ERROR(EString(PE_TEXT("[ShaderJobBuilder] An assembly needs a shader name and a program name.")));
			return FALSE;
		}

		OutDesc = EShaderAssemblyDesc();
		OutDesc.ShaderName		= InRequest.ShaderName;
		OutDesc.GeneratedRoot	= GetShaderGeneratedRoot();
		OutDesc.OutputRoot		= GetShaderOutputRoot();
		OutDesc.ShaderModel		= GetShaderModelForActiveBackend();
		OutDesc.bDumpHlsl		= InRequest.bDumpHlsl;
		OutDesc.bForceRecompile = InRequest.bForceRecompile;
		OutDesc.VertexFactory	= InRequest.VertexFactory;
		OutDesc.Profile			= InRequest.Profile;

		EShaderJobMaterialInfo MaterialInfo;
		EString MaterialJsonPath;
		BOOL32 bHasMaterial = FALSE;
		if (InRequest.MaterialName.Length() > 0u)
		{
			if (!ResolveMaterialJson(InRequest.MaterialName, MaterialJsonPath))
			{
				return FALSE;
			}
			if (!ReadMaterialInfo(MaterialJsonPath, MaterialInfo))
			{
				return FALSE;
			}
			bHasMaterial = TRUE;
		}
		else
		{
			MaterialInfo.DomainName = InRequest.DomainName;
		}

		OutDesc.SurfaceMode			= ((InRequest.SurfaceModeOverride >= 0) ? InRequest.SurfaceModeOverride : MaterialInfo.SurfaceMode);
		OutDesc.bMaterialVertexOps	= MaterialInfo.bVertexOps;
		OutDesc.MaterialVariantDefines = MaterialInfo.VariantDefines;
		if (!TranslateDomainToNumber(MaterialInfo.DomainName, OutDesc.DomainNumber))
		{
			return FALSE;
		}
		OutDesc.VariantName = EName(MakeVariantName(OutDesc));

		rapidjson::Document ProgramDocument;
		if (!ReadProgramDocument(InRequest.ProgramName, ProgramDocument))
		{
			return FALSE;
		}
		if ((!ProgramDocument.HasMember("Stages")) || (!ProgramDocument["Stages"].IsArray()))
		{
			PE_LOG_ERROR(EString(PE_TEXT("[ShaderJobBuilder] The program has no Stages array: ")) + InRequest.ProgramName.ToString());
			return FALSE;
		}

		for (const auto& StageValue : ProgramDocument["Stages"].GetArray())
		{
			if (!StageValue.IsObject())
			{
				continue;
			}
			EShaderStageType Stage = EShaderStageType::SHADER_STAGE_VERTEX;
			if (!ParseShaderStageFromString(EString(ReadJsonString(StageValue, "Stage", "")), Stage))
			{
				PE_LOG_ERROR(EString(PE_TEXT("[ShaderJobBuilder] The program declares an unknown stage: ")) + InRequest.ProgramName.ToString());
				return FALSE;
			}
			UINT32 StageBit = 0u;
			if ((!GetShaderStageMaskBit(Stage, StageBit)) || ((InRequest.StageMask & StageBit) == 0u))
			{
				continue;
			}

			EShaderJobStageTokens Tokens;
			ReadStageTokens(StageValue, Tokens);

			EString StageName;
			if (!TranslateShaderStageToString(Stage, StageName))
			{
				return FALSE;
			}

			// The qualifier is present exactly when the generated content depends on it, so
			// the name is a cache key: same name <=> same content (see the header).
			EString StageSourceName = StageName + EString(PE_TEXT("_")) + InRequest.ProgramName.ToString();
			if ((Tokens.bMaterial) || ((Tokens.bMaterialVertex) && (MaterialInfo.bVertexOps)))
			{
				StageSourceName += EString(PE_TEXT("_")) + InRequest.MaterialName;
			}
			else if (Tokens.bDomain)
			{
				StageSourceName += EString(PE_TEXT("_")) + MaterialInfo.DomainName;
			}

			EShaderStageJobDesc StageDesc;
			StageDesc.Stage				= Stage;
			StageDesc.ProgramName		= InRequest.ProgramName;
			StageDesc.StageSourceName	= StageSourceName;
			StageDesc.MaterialJson		= (bHasMaterial ? MaterialJsonPath : EString());
			StageDesc.DomainName		= MaterialInfo.DomainName;
			StageDesc.bHasMaterial		= bHasMaterial;
			OutDesc.Stages.Add(StageDesc);
		}

		if (OutDesc.Stages.Num<UINT32>() == 0u)
		{
			PE_LOG_ERROR(EString(PE_TEXT("[ShaderJobBuilder] The requested stages are not in the program: ")) + InRequest.ProgramName.ToString());
			return FALSE;
		}
		return TRUE;
	}

	// ---------------------------------------------------------------------------
	// Entry json generation
	// ---------------------------------------------------------------------------

	static BOOL32 ExpandComponentJsonFile(const EString& InToken, const EShaderAssemblyDesc& InDesc
		, const EShaderStageJobDesc& InStage, const EString& InGeneratedRoot, EString& OutJsonFile)
	{
		if ((InToken.Length() == 0u) || (InToken[0u] != '$'))
		{
			// Already a RootDir relative path.
			OutJsonFile = InToken;
			return TRUE;
		}
		if (InToken == EString(SHADER_JOB_TOKEN_MATERIAL))
		{
			if ((!InStage.bHasMaterial) || (InStage.MaterialJson.Length() == 0u))
			{
				PE_LOG_ERROR(EString(PE_TEXT("[ShaderJobBuilder] A stage asks for $Material but no material was given: ")) + InStage.StageSourceName);
				return FALSE;
			}
			OutJsonFile = InStage.MaterialJson;
			return TRUE;
		}
		if (InToken == EString(SHADER_JOB_TOKEN_MATERIAL_VERTEX))
		{
			// VertexOps decides whose vertex functions the stage gets; a material without
			// them shares the neutral provider with every other such material, which is
			// what keeps VS_<Program> one compiled binary instead of one per material.
			if ((InStage.bHasMaterial) && (InDesc.bMaterialVertexOps))
			{
				OutJsonFile = InStage.MaterialJson;
			}
			else
			{
				OutJsonFile = EString(SHADER_JOB_NEUTRAL_MATERIAL_JSON);
			}
			return TRUE;
		}
		if (InToken == EString(SHADER_JOB_TOKEN_DOMAIN))
		{
			if (InStage.DomainName.Length() == 0u)
			{
				PE_LOG_ERROR(EString(PE_TEXT("[ShaderJobBuilder] A stage asks for $Domain but no domain was resolved: ")) + InStage.StageSourceName);
				return FALSE;
			}
			OutJsonFile = MakeDomainJsonPath(InStage.DomainName);
			return TRUE;
		}
		const EString GeneratedToken(SHADER_JOB_TOKEN_GENERATED_COMPONENT);
		if (InToken.Find(GeneratedToken) == 0)
		{
			// The whole file path is assembled before it is made relative: the path helper
			// rebuilds a path out of its parts, so a directory it is handed loses the trailing
			// separator and the file name would come out glued onto the directory name. This
			// is the same concatenation the generator writes the component with, which is what
			// keeps the written and the read path the same file.
			const EString LeafName = InToken.Right(InToken.Length() - GeneratedToken.Length());
			const EString ComponentJsonPath = InGeneratedRoot + EString(SHADER_JOB_GENERATED_COMPONENT_DIRECTORY) + LeafName;
			OutJsonFile = MakePathRelativeTo(GetShaderSourceRoot(), ComponentJsonPath);
			return TRUE;
		}
		PE_LOG_ERROR(EString(PE_TEXT("[ShaderJobBuilder] Unknown component token: ")) + InToken);
		return FALSE;
	}

	BOOL32 EShaderJobBuilder::BuildEntryJson(const EShaderAssemblyDesc& InDesc, const EShaderStageJobDesc& InStage, EString& OutEntryJsonPath)
	{
		const EString GeneratedRoot = ((InDesc.GeneratedRoot.Length() > 0u) ? InDesc.GeneratedRoot : GetShaderGeneratedRoot());
		OutEntryJsonPath = GeneratedRoot + EString(PE_TEXT("Jobs/")) + InStage.StageSourceName + EString(PE_TEXT(".json"));

		rapidjson::Document ProgramDocument;
		if (!ReadProgramDocument(InStage.ProgramName, ProgramDocument))
		{
			return FALSE;
		}
		const rapidjson::Value* StageValue = FindProgramStage(ProgramDocument, InStage.Stage);
		if (!StageValue)
		{
			PE_LOG_ERROR(EString(PE_TEXT("[ShaderJobBuilder] The program declares no such stage: ")) + InStage.StageSourceName);
			return FALSE;
		}
		const EString TemplatePath(ReadJsonString(*StageValue, "Template", ""));
		if (TemplatePath.Length() == 0u)
		{
			PE_LOG_ERROR(EString(PE_TEXT("[ShaderJobBuilder] The program stage has no Template: ")) + InStage.StageSourceName);
			return FALSE;
		}
		const EString EntryPoint(ReadJsonString(*StageValue, "Entry", ""));

		rapidjson::StringBuffer Buffer;
		rapidjson::PrettyWriter<rapidjson::StringBuffer> Writer(Buffer);
		Writer.StartObject();

		Writer.Key("Name");
		Writer.String(*InStage.StageSourceName);
		Writer.Key("Entry");
		Writer.String(*EntryPoint);
		Writer.Key("Template");
		Writer.String(*TemplatePath);

		// ShaderReplaces: the program's component list with every token expanded. The
		// Index order decides the register allocation (C5), so it is copied, not
		// recomputed.
		Writer.Key("ShaderReplaces");
		Writer.StartArray();
		if (((*StageValue).HasMember("Components")) && ((*StageValue)["Components"].IsArray()))
		{
			for (const auto& ComponentValue : (*StageValue)["Components"].GetArray())
			{
				if (!ComponentValue.IsObject())
				{
					continue;
				}
				EString ComponentJsonFile;
				if (!ExpandComponentJsonFile(EString(ReadJsonString(ComponentValue, "JsonFile", "")), InDesc, InStage, GeneratedRoot, ComponentJsonFile))
				{
					return FALSE;
				}
				Writer.StartObject();
				Writer.Key("Index");
				Writer.Int(ReadJsonInt(ComponentValue, "Index", 0));
				Writer.Key("Type");
				Writer.String(ReadJsonString(ComponentValue, "Type", "General"));
				Writer.Key("Name");
				Writer.String(ReadJsonString(ComponentValue, "Name", ""));
				Writer.Key("Replace");
				Writer.String(ReadJsonString(ComponentValue, "Replace", ""));
				Writer.Key("JsonFile");
				Writer.String(*ComponentJsonFile);
				Writer.EndObject();
			}
		}
		Writer.EndArray();

		// Defines: what the source itself declares, plus whether a material provider is
		// injected. The variant axis is deliberately absent - it travels in the variant,
		// so the two sets never overlap and no compilation depends on which one wins.
		Writer.Key("Defines");
		Writer.StartArray();
		if (((*StageValue).HasMember("Defines")) && ((*StageValue)["Defines"].IsArray()))
		{
			for (const auto& DefineValue : (*StageValue)["Defines"].GetArray())
			{
				if (!DefineValue.IsObject())
				{
					continue;
				}
				Writer.StartObject();
				Writer.Key("Name");
				Writer.String(ReadJsonString(DefineValue, "Name", ""));
				Writer.Key("Value");
				Writer.Int(ReadJsonInt(DefineValue, "Value", 1));
				Writer.EndObject();
			}
		}
		Writer.StartObject();
		Writer.Key("Name");
		Writer.String(SHADER_JOB_MATERIAL_FUNCTIONS_DEFINE);
		Writer.Key("Value");
		Writer.Int((InStage.bHasMaterial ? 1 : 0));
		Writer.EndObject();
		Writer.EndArray();

		// Stage level binding blocks belong to the entry json, where they pack after the
		// components (C5); they are copied verbatim.
		for (UINT32 BlockIndex = 0u; BlockIndex < PE_ARRAYSIZE(SHADER_JOB_BINDING_BLOCKS); BlockIndex++)
		{
			const CHAR* BlockName = SHADER_JOB_BINDING_BLOCKS[BlockIndex];
			if (((*StageValue).HasMember(BlockName)) && ((*StageValue)[BlockName].IsArray()))
			{
				Writer.Key(BlockName);
				(*StageValue)[BlockName].Accept(Writer);
			}
		}

		Writer.EndObject();

		if (!EFileHelper::SaveStringToFile(OutEntryJsonPath, EString(Buffer.GetString()), TRUE, FALSE))
		{
			PE_LOG_ERROR(EString(PE_TEXT("[ShaderJobBuilder] Cannot write the entry json: ")) + OutEntryJsonPath);
			return FALSE;
		}
		PE_LOG_LOG(EString(PE_TEXT("[ShaderJobBuilder] Entry json: ")) + OutEntryJsonPath);
		return TRUE;
	}

	BOOL32 EShaderJobBuilder::BuildGeneratedComponents(const EString& InGeneratedRoot)
	{
		// The sampler group is engine configuration rather than material data, which is why
		// one generated component serves every material: a quality setting then changes the
		// sampling behaviour of all of them at once. The hlsl next to it is hand written
		// source, so a missing one is an error and never a reason to write into the source
		// tree. The table below is the configuration; it moves into the engine settings
		// once that has a sampler group to read.
		if (!EFileHelper::IsFileExists(GetShaderSourceRoot() + EString(SHADER_JOB_MATERIAL_SAMPLERS_HLSL)))
		{
			PE_LOG_ERROR(EString(PE_TEXT("[ShaderJobBuilder] The material sampler template is missing: ")) + EString(SHADER_JOB_MATERIAL_SAMPLERS_HLSL));
			return FALSE;
		}

		rapidjson::StringBuffer Buffer;
		rapidjson::PrettyWriter<rapidjson::StringBuffer> Writer(Buffer);
		Writer.StartObject();
		Writer.Key("Name");
		Writer.String("MaterialSamplers");
		Writer.Key("Template");
		Writer.String(SHADER_JOB_MATERIAL_SAMPLERS_HLSL);
		Writer.Key("Samplers");
		Writer.StartArray();
		const CHAR* SamplerNames[] = { "SamplerMaterial2D", "SamplerMaterialCube", "SamplerMaterial3D" };
		for (UINT32 SamplerIndex = 0u; SamplerIndex < PE_ARRAYSIZE(SamplerNames); SamplerIndex++)
		{
			Writer.StartObject();
			Writer.Key("Name");
			Writer.String(SamplerNames[SamplerIndex]);
			Writer.Key("Filter");
			Writer.String("MinMagMipLinear");
			Writer.Key("AddressU");
			Writer.String("Wrap");
			Writer.Key("AddressV");
			Writer.String("Wrap");
			Writer.Key("AddressW");
			Writer.String("Wrap");
			Writer.Key("MaxLOD");
			Writer.Double(16.0);
			Writer.Key("Slot");
			Writer.Int(static_cast<INT32>(SamplerIndex));
			Writer.EndObject();
		}
		Writer.EndArray();
		Writer.EndObject();

		const EString ComponentJsonPath = InGeneratedRoot + EString(SHADER_JOB_GENERATED_COMPONENT_DIRECTORY) + EString(SHADER_JOB_MATERIAL_SAMPLERS_JSON);
		if (!EFileHelper::SaveStringToFile(ComponentJsonPath, EString(Buffer.GetString()), TRUE, FALSE))
		{
			PE_LOG_ERROR(EString(PE_TEXT("[ShaderJobBuilder] Cannot write the generated component: ")) + ComponentJsonPath);
			return FALSE;
		}
		return TRUE;
	}

	// ---------------------------------------------------------------------------
	// Assembly pipeline
	// ---------------------------------------------------------------------------

	static EShaderModelType GetShaderModelForActiveBackend()
	{
		// D3D11 cannot consume SM6.x DXIL, and the artifact directory is keyed by the
		// shader model, so the two back ends never share a compiled set by accident.
		return ((EEngineSettings::ENGINE_GRAPHICS_API == 1u) ? EShaderModelType::SHADER_MODEL_5_0 : EShaderModelType::SHADER_MODEL_6_6);
	}

	static void MakeAssemblyBaseRequest(const EShaderAssemblyDesc& InDesc, EShaderCompileRequest& OutRequest)
	{
		OutRequest = EShaderCompileRequest();
		OutRequest.RootDir			= GetShaderSourceRoot();
		OutRequest.OutputDir		= ((InDesc.OutputRoot.Length() > 0u) ? InDesc.OutputRoot : GetShaderOutputRoot());
		OutRequest.ShaderIncludeDir	= EString();	// empty falls back to RootDir
		OutRequest.ShaderModel		= InDesc.ShaderModel;
		OutRequest.bDebug			= InDesc.bDebug;
		OutRequest.bDumpHlsl		= InDesc.bDumpHlsl;
		OutRequest.bForceRecompile = InDesc.bForceRecompile;
		// The axis travels in the variant, never in the entry json, so the builder never
		// asks the compiler for the reserved "Base" compilation; MakeVariantDesc is still
		// the single place that decides what a variant name means.
		TArray<EShaderVariantDefine> AxisDefines;
		EShaderJobBuilder::MakeAxisDefines(InDesc, AxisDefines);
		const TArray<EShaderVariantReplace> NoReplaces;
		if (!EShaderJobBuilder::MakeVariantDesc(InDesc.VariantName, AxisDefines, NoReplaces, OutRequest.Variant))
		{
			// Unreachable by construction: a builder variant name always carries the axis
			// prefix, so it is never "Base" and the rule cannot reject it.
			PE_LOG_ERROR(EString(PE_TEXT("[ShaderJobBuilder] Cannot build the variant descriptor.")));
		}
	}

#if _EDITOR_ONLY

	// Compile side only: a shipping build reads the cooked artifacts and never asks which
	// stage asset name a compile would have produced.
	static BOOL32 MakeStageAssetName(const EShaderStageJobDesc& InStage, const EName& InVariantName, EString& OutStageAssetName)
	{
		// The compiler names a stage artifact <EntryName>_<Variant>_<StageSuffix> and the
		// importer keys the stage asset on that stem. A mismatch here only costs a
		// recompile, never a wrong asset, because the manifest stays the source of truth.
		EString StageSuffix;
		if (!GetShaderStageSuffix(InStage.Stage, StageSuffix))
		{
			return FALSE;
		}
		OutStageAssetName = InStage.StageSourceName + EString(PE_TEXT("_")) + InVariantName.ToString() + EString(PE_TEXT("_")) + StageSuffix;
		return TRUE;
	}

	static BOOL32 IsStageCompiled(const EString& InOutputDirectory, const EString& InStageAssetName)
	{
		return (EFileHelper::IsFileExists(InOutputDirectory + InStageAssetName + EString(EEngineSettings::ENGINE_ASSET_NAME_TYPE)) ? TRUE : FALSE);
	}

#endif	// _EDITOR_ONLY

	// The variant has to be the one the axis asked for. A same named variant with other
	// defines would be a cache collision, and noticing it here is cheaper than debugging a
	// shader compiled for a mesh it does not have.
	static BOOL32 DoesProgramMatchAxis(const EShaderProgram& InProgram, const EShaderAssemblyDesc& InDesc)
	{
		TArray<EShaderVariantDefine> AxisDefines;
		EShaderJobBuilder::MakeAxisDefines(InDesc, AxisDefines);
		for (UINT32 Index = 0u; Index < AxisDefines.Num<UINT32>(); Index++)
		{
			INT32 DefineValue = 0;
			if (!InProgram.FindDefineValue(AxisDefines[Index].Name, DefineValue))
			{
				return FALSE;
			}
			if (DefineValue != AxisDefines[Index].Value)
			{
				return FALSE;
			}
		}
		return TRUE;
	}

	static BOOL32 IsAssemblyComplete(const EShaderAsset& InAsset, const EShaderAssemblyDesc& InDesc)
	{
		if (InDesc.Stages.Num<UINT32>() == 0u)
		{
			return FALSE;
		}
		const EShaderProgram* Program = InAsset.FindProgram(InDesc.VariantName);
		if ((!Program) || (!DoesProgramMatchAxis(*Program, InDesc)))
		{
			return FALSE;
		}
		for (UINT32 Index = 0u; Index < InDesc.Stages.Num<UINT32>(); Index++)
		{
			if (!Program->HasStage(InDesc.Stages[Index].Stage))
			{
				return FALSE;
			}
		}
		return TRUE;
	}

	BOOL32 EShaderJobBuilder::EnsureAssembled(const EShaderAssemblyDesc& InDesc, const EShaderAsset*& OutAsset)
	{
		OutAsset = nullptr;
		if ((!InDesc.ShaderName.IsValid()) || (InDesc.Stages.Num<UINT32>() == 0u))
		{
			PE_LOG_ERROR(EString(PE_TEXT("[ShaderJobBuilder] An assembly needs a name and at least one stage.")));
			return FALSE;
		}
		const EString ShaderName = InDesc.ShaderName.ToString();
		EShaderAssetManager* AssetManager = EShaderAssetManager::GetManagerSingleton();

		// L1: assembled in this process already and complete - the "take the resource
		// pointer" path, no generation and no compiler.
		const EShaderAsset* Asset = AssetManager->FindShaderAsset(InDesc.ShaderName);
		if ((!InDesc.bForceRecompile) && (Asset) && (IsAssemblyComplete(*Asset, InDesc)))
		{
			OutAsset = Asset;
			return TRUE;
		}

		// The output directory is a function of the target rather than of a stage, so it is
		// resolved once and every stage artifact is looked for next to its siblings.
		EShaderCompileRequest TemplateRequest;
		MakeAssemblyBaseRequest(InDesc, TemplateRequest);
		EString OutputDirectory;
		if (!GetShaderOutputDirectory(TemplateRequest, OutputDirectory))
		{
			return FALSE;
		}

		// L2: the shader PAsset on disk. Loading it says which (variant, stage) pairs are
		// still missing, which is all the per stage loop needs to know.
		const EString ShaderPAssetPath = OutputDirectory + ShaderName + EString(EEngineSettings::ENGINE_ASSET_NAME_TYPE);
		if ((!InDesc.bForceRecompile) && (EFileHelper::IsFileExists(ShaderPAssetPath)))
		{
			if ((AssetManager->LoadShaderAsset(OutputDirectory, ShaderName, Asset)) && (Asset) && (IsAssemblyComplete(*Asset, InDesc)))
			{
				OutAsset = Asset;
				return TRUE;
			}
		}

#if _EDITOR_ONLY
		if (!BuildGeneratedComponents((InDesc.GeneratedRoot.Length() > 0u) ? InDesc.GeneratedRoot : GetShaderGeneratedRoot()))
		{
			return FALSE;
		}

		BOOL32 bImported = FALSE;
		for (UINT32 StageIndex = 0u; StageIndex < InDesc.Stages.Num<UINT32>(); StageIndex++)
		{
			const EShaderStageJobDesc& Stage = InDesc.Stages[StageIndex];
			const EShaderProgram* Program = (Asset ? Asset->FindProgram(InDesc.VariantName) : nullptr);
			if ((Program) && (Program->HasStage(Stage.Stage)) && (DoesProgramMatchAxis(*Program, InDesc)))
			{
				continue;
			}

			EString EntryJsonPath;
			if (!BuildEntryJson(InDesc, Stage, EntryJsonPath))
			{
				return FALSE;
			}

			EShaderCompileRequest Request = TemplateRequest;
			Request.Stage			= Stage.Stage;
			Request.EntryJsonPath	= MakePathRelativeTo(GetShaderSourceRoot(), EntryJsonPath);

			EString StageAssetName;
			if (!MakeStageAssetName(Stage, InDesc.VariantName, StageAssetName))
			{
				PE_LOG_ERROR(EString(PE_TEXT("[ShaderJobBuilder] Unsupported stage in the assembly: ")) + ShaderName);
				return FALSE;
			}

			EString ManifestPath;
			if (!GetShaderManifestPath(Request, ManifestPath))
			{
				return FALSE;
			}

			// L3: the compiled stage artifacts (binary, reflection, manifest) are on disk
			// already - another material compiled the same stage source - so only the fold
			// into this shader is left. L4 is the compiler.
			if ((InDesc.bForceRecompile) || (!IsStageCompiled(OutputDirectory, StageAssetName)) || (!EFileHelper::IsFileExists(ManifestPath)))
			{
				EString CompiledManifestPath;
				if (!EShaderCompileJob::CompileShader(Request, CompiledManifestPath))
				{
					return FALSE;
				}
				ManifestPath = CompiledManifestPath;
			}
			if (!AssetManager->ImportShaderAssetFromManifest(ManifestPath, OutputDirectory, ShaderName))
			{
				PE_LOG_ERROR(EString(PE_TEXT("[ShaderJobBuilder] Cannot fold the stage into the shader: ")) + ShaderName);
				return FALSE;
			}
			bImported = TRUE;
			Asset = nullptr;
		}

		if (!AssetManager->LoadShaderAsset(OutputDirectory, ShaderName, Asset, bImported))
		{
			PE_LOG_ERROR(EString(PE_TEXT("[ShaderJobBuilder] Cannot load the assembled shader: ")) + ShaderName);
			return FALSE;
		}
		if ((!Asset) || (!IsAssemblyComplete(*Asset, InDesc)))
		{
			// A half assembled shader is worse than a missing one: it would draw with a
			// program that has no vertex or no pixel stage at all.
			PE_LOG_ERROR(EString(PE_TEXT("[ShaderJobBuilder] The assembly is incomplete: ")) + ShaderName);
			PE_FAILED((ENGINE_ASSET_ERROR), (PE_TEXT("[ShaderJobBuilder] The assembly is incomplete.")));
			return FALSE;
		}
		OutAsset = Asset;
		return TRUE;
#else
		// A shipping build never generates or compiles: what the cook step produced is all
		// there is, so a missing asset is a hard stop with nothing left to try.
		PE_LOG_ERROR(EString(PE_TEXT("[ShaderJobBuilder] The cooked shader is missing: ")) + ShaderName);
		return FALSE;
#endif
	}

	BOOL32 EShaderJobBuilder::EnsureAssembly(const EShaderAssemblyRequest& InRequest, const EShaderAsset*& OutAsset, EName& OutVariantName)
	{
		EShaderAssemblyDesc Desc;
		if (!MakeAssembly(InRequest, Desc))
		{
			return FALSE;
		}
		OutVariantName = Desc.VariantName;
		return EnsureAssembled(Desc, OutAsset);
	}

	// Mirrors the compiler's reserved base variant name and ENGINE_SHADER_DEFAULT_VARIANT_NAME
	// in the render layer header; the asset layer does not include a render layer header, so
	// the name is spelled once here.
	PE_CONSTEXPR_STATIC const CHAR* SHADER_JOB_BASE_VARIANT_NAME = "Base";

	BOOL32 EShaderJobBuilder::MakeVariantDesc(const EName& InVariantName
		, const TArray<EShaderVariantDefine>& InDefines
		, const TArray<EShaderVariantReplace>& InReplaces
		, EShaderVariantDesc& OutVariantDesc)
	{
		// The one rule about variant names, applied in a single place: "Base" is the
		// compilation the compiler runs with nothing passed in, so it may not carry a
		// define or a replace; every other name is a real variant and is flagged as one,
		// with whatever defines the caller passed (an empty set is allowed - the compiler
		// warns that such a variant is a duplicate of Base).
		OutVariantDesc = EShaderVariantDesc();
		if (!InVariantName.IsValid())
		{
			PE_LOG_ERROR(EString(PE_TEXT("[ShaderJobBuilder] A variant needs a name.")));
			return FALSE;
		}
		OutVariantDesc.Name = InVariantName;
		if (InVariantName == EName(SHADER_JOB_BASE_VARIANT_NAME))
		{
			if ((InDefines.Num<UINT32>() > 0u) || (InReplaces.Num<UINT32>() > 0u))
			{
				PE_LOG_ERROR(EString(PE_TEXT("[ShaderJobBuilder] The reserved variant 'Base' cannot carry defines or replaces.")));
				return FALSE;
			}
			OutVariantDesc.bIsVariant = FALSE;
			return TRUE;
		}
		OutVariantDesc.bIsVariant = TRUE;
		OutVariantDesc.Defines = InDefines;
		OutVariantDesc.Replaces = InReplaces;
		return TRUE;
	}

} // namespace PigeonEngine
