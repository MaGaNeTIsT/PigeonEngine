#pragma once

#include <CoreMinimal.h>
#include <EngineCommon.h>
#include <ShaderAsset/ShaderCompileJob.h>
#include <ShaderAsset/ShaderProgram.h>

namespace PigeonEngine
{

	class EShaderAsset;
	class EMesh;
	class ESkinnedMesh;

	/*
	* ShaderJobBuilder - the generated half of the shader pipeline (design phase P3).
	*
	* A program descriptor (Assets/ShaderSources/Programs/P_*.json) is a hand written
	* "program source": stages, templates and a component list. The compiler never reads
	* it; the compiler reads an *entry json*, which is a generated artifact and therefore
	* belongs under Build/ (design Section3.2: the source tree never holds generated files).
	* This builder is the only place that turns one into the other, so the editor UI that
	* will author materials later calls exactly the functions the engine calls today.
	*
	* Naming contract, and why it is a contract:
	*
	*   stage source name    <Stage>_<Program>[_<Material>|_<Domain>]
	*                        It is the entry json file name, the entry json "Name", and
	*                        the stem the compiler names every artifact after, so two
	*                        compilations that share it share one stage binary - that is
	*                        what lets one VS serve every material of one program.
	*                        The qualifier is present exactly when the generated content
	*                        depends on it: $Material always qualifies, $MaterialVertex
	*                        only when the material declares VertexOps (otherwise the
	*                        neutral vertex material is injected for everyone), $Domain
	*                        qualifies by the domain name. Same name <=> same content.
	*
	*   variant name         V<VF>_P<Profile>_D<Domain>_S<SurfaceMode>
	*                        It carries every variant axis define. The axis never travels
	*                        in the entry json, so no define name is declared in both
	*                        places and no compilation depends on which side would win.
	*
	*   "Base"               Reserved. It is the compilation the compiler runs with no
	*                        passed in define at all, so any compilation that has an axis
	*                        uses a name of its own and always passes bIsVariant = TRUE.
	*
	* Cache order (design C1/C3): an assembly is only worked on when the loaded asset does
	* not already answer for every (variant, stage) pair the caller asked for. In order:
	*   1. the asset cache in EShaderAssetManager (this process),
	*   2. the shader PAsset on disk,
	*   3. the stage assets and the manifest of a stage that is missing,
	*   4. the compiler.
	* Steps 3 and 4 are editor only: a shipping build reads what the cook step produced.
	*/

	// VF_TYPE, mirrored from Public/VertexFactory/VertexFactoryTypes.hlsl. Append only:
	// the value ends up in the variant key, so renumbering would silently repoint it.
	//
	// The value names a factory, not a kind of mesh: one engine mesh factory serves both
	// static and skinned meshes, and what distinguishes them is the mesh's own vertex
	// layout rather than a second factory (see ResolveVertexAxis). Value 1 is retired --
	// it was reserved for a separate skeletal factory before that decision, and no
	// artifact was ever compiled with it, so it is simply never handed out again.
	enum class EShaderVertexFactoryType : UINT8
	{
		ENGINE_MESH				= 0,
		INSTANCED_STATIC_MESH	= 2,
		PROCEDURAL_GRASS		= 3,
		FULLSCREEN				= 4,
		IMGUI					= 5,
	};

	// Attribute bits of the stream profile, in the VF_ATTRIBUTE_* order of
	// Public/VertexFactory/VertexFactoryTypes.hlsl.
	PE_CONSTEXPR_STATIC UINT32 SHADER_VERTEX_ATTRIBUTE_POSITION			= (1u << 0u);
	PE_CONSTEXPR_STATIC UINT32 SHADER_VERTEX_ATTRIBUTE_NORMAL			= (1u << 1u);
	PE_CONSTEXPR_STATIC UINT32 SHADER_VERTEX_ATTRIBUTE_TANGENT			= (1u << 2u);
	PE_CONSTEXPR_STATIC UINT32 SHADER_VERTEX_ATTRIBUTE_UV0				= (1u << 3u);
	PE_CONSTEXPR_STATIC UINT32 SHADER_VERTEX_ATTRIBUTE_UV1				= (1u << 4u);
	PE_CONSTEXPR_STATIC UINT32 SHADER_VERTEX_ATTRIBUTE_COLOR			= (1u << 5u);
	PE_CONSTEXPR_STATIC UINT32 SHADER_VERTEX_ATTRIBUTE_SKIN_INDEX		= (1u << 6u);
	PE_CONSTEXPR_STATIC UINT32 SHADER_VERTEX_ATTRIBUTE_SKIN_WEIGHT		= (1u << 7u);
	PE_CONSTEXPR_STATIC UINT32 SHADER_VERTEX_ATTRIBUTE_INSTANCE_STREAM	= (1u << 8u);

	/*
	* Attribute presence of one compilation. StreamProfile *is* the mask: bit N set means
	* attribute N is present. The design's own example (VF_TYPE 0 with Position, Normal,
	* UV0 and UV1 -> 27) is exactly that layout.
	*
	* The builder emits the complete VF_HAS_* set from the mask and never only the set
	* bits: a flag that is simply missing evaluates to 0 in the json input conditions,
	* which silently culls the matching input and desyncs FinalInputStream from the
	* VFAttributes declaration.
	*/
	struct EShaderVertexProfile
	{
	public:
		UINT32	StreamProfile	= 0u;
	public:
		PE_NODISCARD BOOL32	HasPosition() const { return ((StreamProfile & SHADER_VERTEX_ATTRIBUTE_POSITION) != 0u); }
		PE_NODISCARD BOOL32	HasNormal() const { return ((StreamProfile & SHADER_VERTEX_ATTRIBUTE_NORMAL) != 0u); }
		PE_NODISCARD BOOL32	HasTangent() const { return ((StreamProfile & SHADER_VERTEX_ATTRIBUTE_TANGENT) != 0u); }
		PE_NODISCARD BOOL32	HasUV0() const { return ((StreamProfile & SHADER_VERTEX_ATTRIBUTE_UV0) != 0u); }
		PE_NODISCARD BOOL32	HasUV1() const { return ((StreamProfile & SHADER_VERTEX_ATTRIBUTE_UV1) != 0u); }
		PE_NODISCARD BOOL32	HasColor() const { return ((StreamProfile & SHADER_VERTEX_ATTRIBUTE_COLOR) != 0u); }
		// The skin pair is never half present: ResolveVertexAxis sets both or neither, and
		// the factory's skinning step, its attribute declarations and the two structured
		// buffer declarations all hang off these two flags.
		PE_NODISCARD BOOL32	HasSkinIndex() const { return ((StreamProfile & SHADER_VERTEX_ATTRIBUTE_SKIN_INDEX) != 0u); }
		PE_NODISCARD BOOL32	HasSkinWeight() const { return ((StreamProfile & SHADER_VERTEX_ATTRIBUTE_SKIN_WEIGHT) != 0u); }
	};

	// The influence count Public/VertexFactory/EngineMeshVF.hlsl declares: one uint4 of
	// bone indices and one float4 of weights per vertex, which is one element of each
	// stream because the mesh stores one 16 bit bone index and one float per influence.
	//
	// The count is not a variant axis, so a mesh that reports a different one is refused
	// rather than compiled into a factory that would read the wrong bytes. A data element
	// narrower than the shader's declaration is the case to keep out: the API supplies the
	// components the stream does not carry, and those defaults are 0 for xyz but 1 for w,
	// which would blend in a bone the vertex never named.
	PE_CONSTEXPR_STATIC UINT32 SHADER_VERTEX_SKIN_INFLUENCE_NUM = 4u;

	// One stage of one assembly, resolved: no token is left, the source name is fixed.
	struct EShaderStageJobDesc
	{
	public:
		EShaderStageType	Stage	= EShaderStageType::SHADER_STAGE_VERTEX;
		EName				ProgramName;
		// <Stage>_<Program>[_<Material>|_<Domain>], see the naming contract above.
		EString				StageSourceName;
		// RootDir relative provider of MATERIAL_FUNCTIONS, empty when the program has none.
		EString				MaterialJson;
		// Domain the stage's $Domain token expands to, empty when the program has none.
		EString				DomainName;
		BOOL32				bHasMaterial	= FALSE;
	};

	// What a caller knows before the resolution: which shader it wants, out of which
	// material, program and vertex axis.
	struct EShaderAssemblyRequest
	{
	public:
		EName						ShaderName;					// the name the render layer asks for
		EString						MaterialName;				// empty = the program declares no material
		EString						DomainName;					// used when MaterialName is empty
		EName						ProgramName;
		EShaderVertexFactoryType	VertexFactory		= EShaderVertexFactoryType::ENGINE_MESH;
		EShaderVertexProfile		Profile;
		UINT32						StageMask			= (SHADER_STAGE_MASK_VERTEX | SHADER_STAGE_MASK_PIXEL);
		// Compile even when the stage artifacts are already on disk: the dependency graph is
		// a follow up, so this is how a caller says the source it edited has to be rebuilt.
		BOOL32						bForceRecompile		= FALSE;
		// -1 keeps what the material declares (or Opaque when there is no material).
		INT32						SurfaceModeOverride	= -1;
		BOOL32						bDumpHlsl			= FALSE;
	};

	// The resolved job: the stage list plus the axis every stage of it is compiled with.
	// One variant per assembly on purpose - the vertex axis shapes the varying struct, so
	// the vertex and the pixel stage of one draw must be compiled for the same axis.
	struct EShaderAssemblyDesc
	{
	public:
		EName						ShaderName;
		EString						GeneratedRoot;			// empty = ENGINE_SHADER_GENERATED_ROOT_DIR
		EString						OutputRoot;				// empty = ENGINE_SHADER_OUTPUT_ROOT_DIR
		// Compile even when the stage artifacts are already on disk (see the request field).
		BOOL32						bForceRecompile	= FALSE;
		EShaderModelType			ShaderModel		= EShaderModelType::SHADER_MODEL_6_6;
		BOOL32						bDebug			= TRUE;
		BOOL32						bDumpHlsl		= FALSE;
	public:
		EName						VariantName;
		EShaderVertexFactoryType	VertexFactory	= EShaderVertexFactoryType::ENGINE_MESH;
		EShaderVertexProfile		Profile;
		INT32						DomainNumber	= 0;
		INT32						SurfaceMode		= 0;
		// A material that declares VertexOps supplies its own vertex functions; every
		// other material gets the neutral provider, which is what makes the shared
		// VS_<Program> source shareable across materials.
		BOOL32						bMaterialVertexOps	= FALSE;
		TArray<EShaderVariantDefine>	MaterialVariantDefines;
		TArray<EShaderStageJobDesc>	Stages;
	};

	class EShaderJobBuilder
	{
	public:
		// ---- axis ---------------------------------------------------------------
		// The single place a mesh becomes a vertex axis (design 7.10 step 5): which factory
		// draws it, and which attributes it actually carries. Derived from the mesh's own
		// vertex layout, never from the asset class, so a static mesh and a skinned mesh go
		// through the same call and differ only in what their layout says.
		//
		// FALSE is a hard error and never a silent downgrade to a profile the mesh does not
		// have: it means the mesh is missing a stream the selected factory has to read
		// (Position, or a skin pair whose influence count the factory declares).
		PE_NODISCARD static BOOL32	ResolveVertexAxis(const EMesh* InMesh
			, EShaderVertexFactoryType& OutVertexFactory, EShaderVertexProfile& OutProfile);
		PE_NODISCARD static EString	MakeVariantName(const EShaderAssemblyDesc& InDesc);
		static void					MakeAxisDefines(const EShaderAssemblyDesc& InDesc, TArray<EShaderVariantDefine>& OutDefines);

		// ---- generation ---------------------------------------------------------
		// Reads the program descriptor and the material, resolves every `$` token, names
		// the stage sources and fills the desc. Writes nothing, compiles nothing.
		PE_NODISCARD static BOOL32	MakeAssembly(const EShaderAssemblyRequest& InRequest, EShaderAssemblyDesc& OutDesc);
		// Writes the entry json of one stage under <GeneratedRoot>Jobs/. Idempotent:
		// identical input produces the identical file.
		PE_NODISCARD static BOOL32	BuildEntryJson(const EShaderAssemblyDesc& InDesc, const EShaderStageJobDesc& InStage, EString& OutEntryJsonPath);
		// Writes the generated component json every material bearing program references.
		PE_NODISCARD static BOOL32	BuildGeneratedComponents(const EString& InGeneratedRoot);

		// ---- the pipeline -------------------------------------------------------
		// Generates (as needed), compiles (as needed), folds the stage manifests into
		// <ShaderName>.PAsset and loads it. Every requested (variant, stage) pair is
		// verified before the asset is handed out.
		PE_NODISCARD static BOOL32	EnsureAssembled(const EShaderAssemblyDesc& InDesc, const EShaderAsset*& OutAsset);
		// MakeAssembly + EnsureAssembled. OutVariantName is the variant the asset holds,
		// so the caller can build its instance without recomputing the axis.
		PE_NODISCARD static BOOL32	EnsureAssembly(const EShaderAssemblyRequest& InRequest, const EShaderAsset*& OutAsset, EName& OutVariantName);

		// The only rule the compile layer has about variant names, applied in one place:
		// "Base" is the compilation the compiler runs with nothing passed in, so it may not
		// carry a define or a replace; every other name is a real variant and is flagged as
		// one, with whatever defines the caller passed.
		PE_NODISCARD static BOOL32	MakeVariantDesc(const EName& InVariantName
			, const TArray<EShaderVariantDefine>& InDefines
			, const TArray<EShaderVariantReplace>& InReplaces
			, EShaderVariantDesc& OutVariantDesc);
	};

} // namespace PigeonEngine
