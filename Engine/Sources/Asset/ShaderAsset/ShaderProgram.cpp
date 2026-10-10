#include "ShaderProgram.h"

#include <Editor/EditorLogManager.h>

namespace PigeonEngine
{

	EShaderProgram::EShaderProgram(const EName& InShaderName, const EName& InVariantName, const UINT32 InVariantIndex)
		: ShaderName(InShaderName)
		, VariantName(InVariantName)
		, VariantIndex(InVariantIndex)
		, Defines{}
		, DefineHash{}
		, ReplaceHash{}
		, bBindless(FALSE)
		, StageEntries{}
	{
	}

	EShaderProgram::~EShaderProgram()
	{
		if (StageEntries.Num<UINT32>() > 0u)
		{
			StageEntries.Empty();
		}
	}

	const EShaderVariantDefine* EShaderProgram::GetDefineByIndex(const UINT32 InIndex) const
	{
		if (InIndex >= Defines.Num<UINT32>())
		{
			return nullptr;
		}
		return (&(Defines[InIndex]));
	}

	BOOL32 EShaderProgram::HasDefine(const EName& InDefineName) const
	{
		for (UINT32 DefIdx = 0u, DefNum = Defines.Num<UINT32>(); DefIdx < DefNum; DefIdx++)
		{
			if (Defines[DefIdx].Name == InDefineName)
			{
				return TRUE;
			}
		}
		return FALSE;
	}

	BOOL32 EShaderProgram::FindDefineValue(const EName& InDefineName, INT32& OutValue) const
	{
		for (UINT32 DefIdx = 0u, DefNum = Defines.Num<UINT32>(); DefIdx < DefNum; DefIdx++)
		{
			if (Defines[DefIdx].Name == InDefineName)
			{
				OutValue = Defines[DefIdx].Value;
				return TRUE;
			}
		}
		return FALSE;
	}

	BOOL32 EShaderProgram::ResolveBindlessFlag(const TArray<EShaderVariantDefine>& InDefines)
	{
		// Called once per program while the loading thread applies the variant source.
		// Resolving the flag here keeps EName construction out of the runtime path.
		const EName BindlessDefineName(SHADER_VARIANT_DEFINE_BINDLESS);
		if (BindlessDefineName.IsNone())
		{
			return FALSE;
		}
		for (UINT32 DefIdx = 0u, DefNum = InDefines.Num<UINT32>(); DefIdx < DefNum; DefIdx++)
		{
			if (InDefines[DefIdx].Name != BindlessDefineName)
			{
				continue;
			}
			return ((InDefines[DefIdx].Value != 0) ? TRUE : FALSE);
		}
		// The engine macro was not injected, so this variant was compiled for the
		// register bound path.
		return FALSE;
	}

	void EShaderProgram::SetVariantSource(const TArray<EShaderVariantDefine>& InDefines, const EString& InDefineHash, const EString& InReplaceHash)
	{
		Defines = InDefines;
		DefineHash = InDefineHash;
		ReplaceHash = InReplaceHash;
		bBindless = ResolveBindlessFlag(InDefines);
	}

	EShaderStageEntry* EShaderProgram::EnsureStageEntry(const EShaderStageType InStage)
	{
		EShaderStageEntry* ExistingEntry = FindStageEntry(InStage);
		if (ExistingEntry)
		{
			return ExistingEntry;
		}
		EShaderStageEntry NewEntry;
		NewEntry.Stage = InStage;
		StageEntries.Add(NewEntry);
		return (&(StageEntries[StageEntries.Num<UINT32>() - 1u]));
	}

	EShaderStageEntry* EShaderProgram::FindStageEntry(const EShaderStageType InStage)
	{
		for (UINT32 EntryIdx = 0u, EntryNum = StageEntries.Num<UINT32>(); EntryIdx < EntryNum; EntryIdx++)
		{
			if (StageEntries[EntryIdx].Stage == InStage)
			{
				return (&(StageEntries[EntryIdx]));
			}
		}
		return nullptr;
	}

	const EShaderStageEntry* EShaderProgram::FindStageEntry(const EShaderStageType InStage) const
	{
		for (UINT32 EntryIdx = 0u, EntryNum = StageEntries.Num<UINT32>(); EntryIdx < EntryNum; EntryIdx++)
		{
			if (StageEntries[EntryIdx].Stage == InStage)
			{
				return (&(StageEntries[EntryIdx]));
			}
		}
		return nullptr;
	}

	BOOL32 EShaderProgram::HasStage(const EShaderStageType InStage) const
	{
		const EShaderStageEntry* Entry = FindStageEntry(InStage);
		if (!Entry)
		{
			return FALSE;
		}
		return Entry->bValid;
	}

	EShaderBindingPlan* EShaderProgram::GetMutableBindingPlan(const EShaderStageType InStage) const
	{
		// The plan's root and table indices are derived data: the layout generator recomputes
		// them from the stage's own reflection, so handing them out as mutable does not let a
		// caller change what the program means. The cast is only sound while the program object
		// itself is held non-const, which is how the engine owns programs (a program is built and
		// then kept alive by its owner, never as a const object).
		EShaderProgram* MutableThis = const_cast<EShaderProgram*>(this);
		EShaderStageEntry* Entry = ((MutableThis != nullptr) ? MutableThis->FindStageEntry(InStage) : nullptr);
		if ((!Entry) || (!Entry->bValid))
		{
			return nullptr;
		}
		return (&(Entry->BindingPlan));
	}

	const EShaderStageEntry* EShaderProgram::GetStageEntryByIndex(const UINT32 InIndex) const
	{
		if (InIndex >= StageEntries.Num<UINT32>())
		{
			return nullptr;
		}
		return (&(StageEntries[InIndex]));
	}

	UINT32 EShaderProgram::GetStageMask() const
	{
		UINT32 StageMaskValue = SHADER_STAGE_MASK_NONE;
		for (UINT32 EntryIdx = 0u, EntryNum = StageEntries.Num<UINT32>(); EntryIdx < EntryNum; EntryIdx++)
		{
			if (!StageEntries[EntryIdx].bValid)
			{
				continue;
			}
			UINT32 StageMaskBit = SHADER_STAGE_MASK_NONE;
			if (GetShaderStageMaskBit(StageEntries[EntryIdx].Stage, StageMaskBit))
			{
				StageMaskValue |= StageMaskBit;
			}
		}
		return StageMaskValue;
	}

	BOOL32 EShaderProgram::IsValid() const
	{
		for (UINT32 EntryIdx = 0u, EntryNum = StageEntries.Num<UINT32>(); EntryIdx < EntryNum; EntryIdx++)
		{
			if (StageEntries[EntryIdx].bValid)
			{
				return TRUE;
			}
		}
		return FALSE;
	}

	UINT32 EShaderProgram::GetCulledFieldNum() const
	{
		UINT32 CulledNum = 0u;
		for (UINT32 EntryIdx = 0u, EntryNum = StageEntries.Num<UINT32>(); EntryIdx < EntryNum; EntryIdx++)
		{
			if (!StageEntries[EntryIdx].bValid)
			{
				continue;
			}
			CulledNum += StageEntries[EntryIdx].Reflection.GetCulledFieldNum();
		}
		return CulledNum;
	}

	UINT32 EShaderProgram::GetExposedFieldNum() const
	{
		UINT32 ExposedNum = 0u;
		for (UINT32 EntryIdx = 0u, EntryNum = StageEntries.Num<UINT32>(); EntryIdx < EntryNum; EntryIdx++)
		{
			if (!StageEntries[EntryIdx].bValid)
			{
				continue;
			}
			ExposedNum += StageEntries[EntryIdx].Reflection.GetExposedFieldNum();
		}
		return ExposedNum;
	}

} // namespace PigeonEngine
