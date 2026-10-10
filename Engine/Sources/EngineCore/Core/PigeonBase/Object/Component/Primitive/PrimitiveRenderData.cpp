#include "PrimitiveRenderData.h"

#include <ShaderAsset/ShaderInstance.h>

namespace PigeonEngine
{

	// ---------------------------------------------------------------------------
	// FShaderValueReport
	// ---------------------------------------------------------------------------

	FShaderValueReportEntry* FShaderValueReport::FindOrAdd(const EName& InBlockName, const EName& InFieldName)
	{
		for (UINT32 Index = 0u, Num = Values.Num<UINT32>(); Index < Num; Index++)
		{
			if ((Values[Index].BlockName == InBlockName) && (Values[Index].FieldName == InFieldName))
			{
				return &(Values[Index]);
			}
		}
		FShaderValueReportEntry Entry;
		Entry.BlockName = InBlockName;
		Entry.FieldName = InFieldName;
		Values.Add(Entry);
		return &(Values[Values.Num<UINT32>() - 1u]);
	}

	void FShaderValueReport::SetScalar(const EName& InBlockName, const EName& InFieldName, const FLOAT InValue)
	{
		FShaderValueReportEntry* Entry = FindOrAdd(InBlockName, InFieldName);
		if (!Entry)
		{
			return;
		}
		Entry->Kind = EPrimitiveValueKindType::PRIMITIVE_VALUE_SCALAR;
		Entry->Words.Float32[0u] = InValue;
	}

	void FShaderValueReport::SetVector4(const EName& InBlockName, const EName& InFieldName, const Vector4& InValue)
	{
		FShaderValueReportEntry* Entry = FindOrAdd(InBlockName, InFieldName);
		if (!Entry)
		{
			return;
		}
		Entry->Kind = EPrimitiveValueKindType::PRIMITIVE_VALUE_VECTOR;
		Entry->Words.Float32[0u] = InValue.x;
		Entry->Words.Float32[1u] = InValue.y;
		Entry->Words.Float32[2u] = InValue.z;
		Entry->Words.Float32[3u] = InValue.w;
	}

	void FShaderValueReport::SetMatrix(const EName& InBlockName, const EName& InFieldName, const Matrix4x4& InValue)
	{
		FShaderValueReportEntry* Entry = FindOrAdd(InBlockName, InFieldName);
		if (!Entry)
		{
			return;
		}
		Entry->Kind = EPrimitiveValueKindType::PRIMITIVE_VALUE_MATRIX;
		// Row major, and that is the whole conversion: the shader side matrix is the transpose
		// of this one, which is what mul(Matrix, Vector) expects for a row vector cpu matrix.
		// The storage is a flat row major 4x4, so the sixteen words are a straight copy.
		const FLOAT* Source = &(InValue.m[0u][0u]);
		for (UINT32 Index = 0u; Index < PRIMITIVE_VALUE_WORD_MAX; Index++)
		{
			Entry->Words.Float32[Index] = Source[Index];
		}
	}

	void FShaderValueReport::SetUInt(const EName& InBlockName, const EName& InFieldName, const UINT32 InValue)
	{
		FShaderValueReportEntry* Entry = FindOrAdd(InBlockName, InFieldName);
		if (!Entry)
		{
			return;
		}
		Entry->Kind = EPrimitiveValueKindType::PRIMITIVE_VALUE_UINT;
		Entry->Words.UInt32[0u] = InValue;
	}

	void FShaderValueReport::SetUInt4(const EName& InBlockName, const EName& InFieldName, const UINT32* InValues)
	{
		if (!InValues)
		{
			return;
		}
		FShaderValueReportEntry* Entry = FindOrAdd(InBlockName, InFieldName);
		if (!Entry)
		{
			return;
		}
		Entry->Kind = EPrimitiveValueKindType::PRIMITIVE_VALUE_UINT4;
		for (UINT32 Index = 0u; Index < 4u; Index++)
		{
			Entry->Words.UInt32[Index] = InValues[Index];
		}
	}

	void FShaderValueReport::Clear()
	{
		Values.Empty();
	}

	const FShaderValueReportEntry* FShaderValueReport::GetValueByIndex(const UINT32 InIndex) const
	{
		if (InIndex >= Values.Num<UINT32>())
		{
			return nullptr;
		}
		return &(Values[InIndex]);
	}

	// ---------------------------------------------------------------------------
	// FPrimitiveRenderAssetReport
	// ---------------------------------------------------------------------------

	const EShaderInstance* FPrimitiveMaterialSlotReport::Find(const EName& InPassName) const
	{
		for (UINT32 Index = 0u, Num = Passes.Num<UINT32>(); Index < Num; Index++)
		{
			if (Passes[Index].PassName == InPassName)
			{
				return Passes[Index].Instance;
			}
		}
		return nullptr;
	}

	void FPrimitiveRenderAssetReport::SetMaterialInstance(const UINT32 InSlotIndex, const EName& InPassName, const EShaderInstance* InInstance)
	{
		if (!InPassName.IsValid())
		{
			return;
		}
		while (MaterialSlots.Num<UINT32>() <= InSlotIndex)
		{
			FPrimitiveMaterialSlotReport Empty;
			MaterialSlots.Add(Empty);
		}
		FPrimitiveMaterialSlotReport& Slot = MaterialSlots[InSlotIndex];
		for (UINT32 Index = 0u, Num = Slot.Passes.Num<UINT32>(); Index < Num; Index++)
		{
			if (Slot.Passes[Index].PassName == InPassName)
			{
				Slot.Passes[Index].Instance = InInstance;
				return;
			}
		}
		FPrimitiveMaterialPassReport Pass;
		Pass.PassName = InPassName;
		Pass.Instance = InInstance;
		Slot.Passes.Add(Pass);
	}

	void FPrimitiveRenderAssetReport::SetNamedInstance(const EName& InRoleName, const EShaderInstance* InInstance)
	{
		if (!InRoleName.IsValid())
		{
			return;
		}
		for (UINT32 Index = 0u, Num = NamedInstances.Num<UINT32>(); Index < Num; Index++)
		{
			if (NamedInstances[Index].RoleName == InRoleName)
			{
				NamedInstances[Index].Instance = InInstance;
				return;
			}
		}
		FPrimitiveNamedInstanceReport Entry;
		Entry.RoleName = InRoleName;
		Entry.Instance = InInstance;
		NamedInstances.Add(Entry);
	}

	void FPrimitiveRenderAssetReport::SetNamedAsset(const EName& InRoleName, const EAssetBase* InAsset)
	{
		if (!InRoleName.IsValid())
		{
			return;
		}
		for (UINT32 Index = 0u, Num = NamedAssets.Num<UINT32>(); Index < Num; Index++)
		{
			if (NamedAssets[Index].RoleName == InRoleName)
			{
				NamedAssets[Index].Asset = InAsset;
				return;
			}
		}
		FPrimitiveNamedAssetReport Entry;
		Entry.RoleName = InRoleName;
		Entry.Asset = InAsset;
		NamedAssets.Add(Entry);
	}

	const FPrimitiveMaterialSlotReport* FPrimitiveRenderAssetReport::GetMaterialSlot(const UINT32 InSlotIndex) const
	{
		if (InSlotIndex >= MaterialSlots.Num<UINT32>())
		{
			return nullptr;
		}
		return &(MaterialSlots[InSlotIndex]);
	}

	const EShaderInstance* FPrimitiveRenderAssetReport::FindMaterialInstance(const UINT32 InSlotIndex, const EName& InPassName) const
	{
		const FPrimitiveMaterialSlotReport* Slot = GetMaterialSlot(InSlotIndex);
		if (!Slot)
		{
			return nullptr;
		}
		return Slot->Find(InPassName);
	}

	const EShaderInstance* FPrimitiveRenderAssetReport::FindNamedInstance(const EName& InRoleName) const
	{
		for (UINT32 Index = 0u, Num = NamedInstances.Num<UINT32>(); Index < Num; Index++)
		{
			if (NamedInstances[Index].RoleName == InRoleName)
			{
				return NamedInstances[Index].Instance;
			}
		}
		return nullptr;
	}

	const EAssetBase* FPrimitiveRenderAssetReport::FindNamedAsset(const EName& InRoleName) const
	{
		for (UINT32 Index = 0u, Num = NamedAssets.Num<UINT32>(); Index < Num; Index++)
		{
			if (NamedAssets[Index].RoleName == InRoleName)
			{
				return NamedAssets[Index].Asset;
			}
		}
		return nullptr;
	}

} // namespace PigeonEngine
