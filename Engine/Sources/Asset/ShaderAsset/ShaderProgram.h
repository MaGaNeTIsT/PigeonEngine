#pragma once

#include <CoreMinimal.h>
#include <EngineCommon.h>
#include <ShaderAsset/ShaderReflection.h>
#include <Shader/ShaderBindingPlan.h>

namespace PigeonEngine
{

	class EShaderStageAsset;

	// Engine macro the compile layer injects for the bindless target. The binding
	// mode is therefore read from data (the variant defines reported by the
	// manifest), and the runtime never has to second guess the shader model. It must
	// match the macro the shader architecture defines for its bindless path.
	static constexpr const CHAR* SHADER_VARIANT_DEFINE_BINDLESS = "PE_SHADER_BINDLESS";

	/*
	* ShaderAsset - one compiled variant of one shader.
	*
	* A program is keyed by (ShaderName, VariantName). It owns one stage entry per
	* compiled stage, and every entry keeps its own reflection and binding plan:
	* stages are compiled independently, and the compiler may cull different
	* variables per stage, so merging them would only hide that difference.
	*
	* The stages are compiled one call at a time (the compiler takes a single stage
	* per invocation), so a program accumulates its entries across loads. Entries are
	* replaced in place by EnsureStageEntry, which keeps previously handed out entry
	* pointers valid.
	*/

	struct EShaderVariantDefine
	{
	public:
		EName	Name;
		INT32	Value	= 0;
	};

	struct EShaderVariantReplace
	{
	public:
		EName	Name;
		EString	Element;
	};

	// Author side description of one variant. It is both what the caller hands to
	// the compile layer and what the manifest reports back, so the loader can prove
	// that the compiled variants match what was requested.
	struct EShaderVariantDesc
	{
	public:
		EName						Name;
		TArray<EShaderVariantDefine>	Defines;
		TArray<EShaderVariantReplace>	Replaces;
		BOOL32						bIsVariant	= TRUE;
	};

	struct EShaderStageEntry
	{
	public:
		EShaderStageType			Stage		= EShaderStageType::SHADER_STAGE_VERTEX;
		const EShaderStageAsset*	StageAsset	= nullptr;
		// Name of the stage asset above, copied in while that asset is alive. The pointer is
		// borrowed, and a stage asset is shared across shader assets: an assembly that folds a new
		// stage into one shader replaces the stage under its own name and releases the object every
		// other shader still points at. The draw path detects exactly that state ("the vertex stage
		// has no render resource") and has to name the stage when it reports it - so the name lives
		// here, where reading it stays safe, rather than behind the pointer.
		EString						StageAssetName;
		EShaderReflection			Reflection;
		EShaderBindingPlan			BindingPlan;
		BOOL32						bValid		= FALSE;
	};

	class EShaderProgram
	{
	public:
		EShaderProgram(const EName& InShaderName, const EName& InVariantName, const UINT32 InVariantIndex);
		~EShaderProgram();
	public:
		PE_NODISCARD const EName&	GetShaderName() const { return ShaderName; }
		PE_NODISCARD const EName&	GetVariantName() const { return VariantName; }
		PE_NODISCARD UINT32			GetVariantIndex() const { return VariantIndex; }
		PE_NODISCARD const EString&	GetDefineHash() const { return DefineHash; }
		PE_NODISCARD const EString&	GetReplaceHash() const { return ReplaceHash; }
		PE_NODISCARD UINT32			GetDefineNum() const { return Defines.Num<UINT32>(); }
		PE_NODISCARD const EShaderVariantDefine*	GetDefineByIndex(const UINT32 InIndex) const;
		// Defines of the variant, exactly as the manifest reported them. They are the
		// keyword set the render layer uses to pick a variant, and they carry the
		// engine macros that were injected for this target (e.g. the bindless switch).
		PE_NODISCARD const TArray<EShaderVariantDefine>&	GetDefines() const { return Defines; }
		PE_NODISCARD BOOL32			HasDefine(const EName& InDefineName) const;
		PE_NODISCARD BOOL32			FindDefineValue(const EName& InDefineName, INT32& OutValue) const;
		// TRUE when the variant was compiled for the engine bindless path. Resolved
		// once while the variant defines are applied (loading thread), so the runtime
		// only ever reads a flag.
		PE_NODISCARD BOOL32			IsBindlessVariant() const { return bBindless; }
	public:
		void	SetVariantSource(const TArray<EShaderVariantDefine>& InDefines, const EString& InDefineHash, const EString& InReplaceHash);
		// Creates the entry when the stage is new, otherwise overwrites it in place.
		EShaderStageEntry*	EnsureStageEntry(const EShaderStageType InStage);
		EShaderStageEntry*	FindStageEntry(const EShaderStageType InStage);
		PE_NODISCARD const EShaderStageEntry*	FindStageEntry(const EShaderStageType InStage) const;
		PE_NODISCARD BOOL32	HasStage(const EShaderStageType InStage) const;
		PE_NODISCARD UINT32	GetStageEntryNum() const { return StageEntries.Num<UINT32>(); }
		PE_NODISCARD const EShaderStageEntry*	GetStageEntryByIndex(const UINT32 InIndex) const;
		// Binding plan of one stage, for the layout generator to rewrite. The plan's root and
		// table indices are derived data: they are recomputed from the stage's reflection every
		// time the program is laid out, so this cannot change what the program means. Null when
		// the stage has no valid entry.
		PE_NODISCARD EShaderBindingPlan*	GetMutableBindingPlan(const EShaderStageType InStage) const;
		PE_NODISCARD UINT32	GetStageMask() const;
		// A program is usable as soon as one of its stages is valid.
		PE_NODISCARD BOOL32	IsValid() const;
		PE_NODISCARD UINT32	GetCulledFieldNum() const;
		PE_NODISCARD UINT32	GetExposedFieldNum() const;
	private:
		// Resolved while the variant defines are applied, never from the runtime path.
		PE_NODISCARD static BOOL32	ResolveBindlessFlag(const TArray<EShaderVariantDefine>& InDefines);
	private:
		EName						ShaderName;
		EName						VariantName;
		UINT32						VariantIndex;
		TArray<EShaderVariantDefine>	Defines;
		// Hashes are identities reported by the compiler, not names: keep them as
		// strings for logging and cache keys.
		EString						DefineHash;
		EString						ReplaceHash;
		// Resolved from the defines once, the runtime only reads it.
		BOOL32						bBindless;
		TArray<EShaderStageEntry>	StageEntries;

	public:
		EShaderProgram() = delete;

		CLASS_REMOVE_COPY_BODY(EShaderProgram)

	};

} // namespace PigeonEngine
