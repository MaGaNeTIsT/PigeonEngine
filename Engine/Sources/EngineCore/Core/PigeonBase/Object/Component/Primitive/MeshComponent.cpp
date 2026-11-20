#include "MeshComponent.h"
#include <RenderProxy/MeshSceneProxy.h>
#include <Renderer/ScenePass.h>
#include <ShaderAsset/ShaderInstance.h>
#include <ShaderAsset/ShaderJobBuilder.h>
#include <Shader/EngineShaderInstanceCache.h>
#include <Reflection/Property.h>

namespace PigeonEngine
{
	static void SerializeMaterialSlotReferences(EArchive& InArchive, const CHAR* InName, TArray<FMaterialSlotReference>& InOutReferences)
	{
		UINT32 Count = InOutReferences.Num<UINT32>();
		InArchive.BeginArray(InName, Count);
		if (!InArchive.IsSaving())
		{
			InOutReferences.Empty();
		}
		for (UINT32 Index = 0u; Index < Count; Index++)
		{
			const EString IndexKey = EString::FromInt(Index);
			InArchive.BeginObject(*IndexKey);
			FMaterialSlotReference Reference = InArchive.IsSaving() ? InOutReferences[Index] : FMaterialSlotReference();
			InArchive.SerializeString(PE_TEXT("MaterialName"), Reference.MaterialName);
			InArchive.SerializeString(PE_TEXT("ShaderName"), Reference.ShaderName);
			InArchive.SerializeString(PE_TEXT("ProgramName"), Reference.ProgramName);
			if (!InArchive.IsSaving())
			{
				InOutReferences.Add(Reference);
			}
			InArchive.EndObject();
		}
		InArchive.EndArray();
	}

	static void RegisterMaterialSlotStruct()
	{
		const size_t StructHash = GetClassHashCode<FMaterialSlotReference>();
		const EString StructName(PE_TEXT("FMaterialSlotReference"));
		const UINT32 EditFlags = static_cast<UINT32>(EPropertyFlagsType::EDIT_ANYWHERE);
		// Picking a different material invalidates the derived shader/program names. Clear
		// them through the field setter so SetMaterialSlotReferences recomputes the triple
		// from the new material instead of keeping the previous material's shader.
		AddStructAssetProperty<FMaterialSlotReference>(StructHash, StructName, PE_TEXT("MaterialName"),
			[](const FMaterialSlotReference* Reference) -> EString { return Reference->MaterialName; },
			[](FMaterialSlotReference* Reference, const EString& Value) -> void
			{
				Reference->MaterialName = Value;
				Reference->ShaderName = EString();
				Reference->ProgramName = EString();
			},
			EPropertyAssetType::MATERIAL, EditFlags);
		// The shader and program are derived from the material and the mesh, so they are
		// shown but never edited directly.
		AddStructReadOnlyProperty(StructHash, StructName, PE_TEXT("ShaderName"), &FMaterialSlotReference::ShaderName);
		AddStructReadOnlyProperty(StructHash, StructName, PE_TEXT("ProgramName"), &FMaterialSlotReference::ProgramName);
	}

	static void RegisterClassProperties()
	{
		const size_t ClassHash = GetClassHashCode<PMeshComponent>();
		RegisterMaterialSlotStruct();
		AddClassPropertyStructCustom<PMeshComponent, TArray<FMaterialSlotReference>>(ClassHash, PE_TEXT("MaterialSlots"),
			[](const PMeshComponent* Component) -> TArray<FMaterialSlotReference> { return Component->GetMaterialSlotReferences(); },
			[](PMeshComponent* Component, const TArray<FMaterialSlotReference>& Value) -> void { Component->SetMaterialSlotReferences(Value); },
			SerializeMaterialSlotReferences,
			static_cast<UINT32>(EPropertyFlagsType::EDIT_ANYWHERE));
	}

	static void RegisterClassTypes()
	{
		RegisterClassType<PMeshComponent, PPrimitiveComponent>();
		RegisterClassProperties();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	PMeshComponent::PMeshComponent()
	{
		POBJ_DEBUGNAME_SET(this, "MeshComponent");
	}
	PMeshComponent::~PMeshComponent()
	{
	}

	const TArray<FMaterialSlotReference>& PMeshComponent::GetMaterialSlotReferences() const
	{
		return MaterialSlotReferences;
	}

	const FMaterialSlotReference& PMeshComponent::GetMaterialSlotReference(UINT32 InSlotIdx) const
	{
		static const FMaterialSlotReference EmptyReference;
		if (InSlotIdx < MaterialSlotReferences.Num<UINT32>())
		{
			return MaterialSlotReferences[InSlotIdx];
		}
		return EmptyReference;
	}

	void PMeshComponent::EnsureMaterialSlotReferenceNum(UINT32 InNum)
	{
		const UINT32 OldNum = MaterialSlotReferences.Num<UINT32>();
		if (MaterialSlotReferences.Num<UINT32>() < InNum)
		{
			MaterialSlotReferences.SetNum(InNum);
		}
		if (MaterialSlotRequestGenerations.Num<UINT32>() < InNum)
		{
			MaterialSlotRequestGenerations.SetNum(InNum);
		}
		for (UINT32 Index = OldNum; Index < InNum; Index++)
		{
			MaterialSlotRequestGenerations[Index] = 0u;
		}
	}

	FMaterialSlotReference PMeshComponent::MakeMaterialSlotReference(const EString& InMaterialName) const
	{
		FMaterialSlotReference Reference;
		Reference.MaterialName = InMaterialName;
		if (InMaterialName.Length() == 0u)
		{
			return Reference;
		}
		const INT32 Separator = InMaterialName.RightFind(EString(PE_TEXT("/")));
		Reference.ShaderName = (Separator >= 0)
			? InMaterialName.Right(InMaterialName.Length() - static_cast<UINT32>(Separator) - 1u)
			: InMaterialName;
		Reference.ProgramName = GetMaterialProgramName();
		return Reference;
	}

	void PMeshComponent::SetMaterialSlotReferences(const TArray<FMaterialSlotReference>& InReferences)
	{
		const UINT32 OldNum = MaterialSlotReferences.Num<UINT32>();
		for (UINT32 Index = 0u; Index < OldNum; Index++)
		{
			if (MaterialSlotReferences[Index].MaterialName.Length() > 0u)
			{
				SetMaterialInstance(Index, nullptr);
			}
		}
		MaterialSlotReferences.SetNum(InReferences.Num<UINT32>());
		MaterialSlotRequestGenerations.SetNum(InReferences.Num<UINT32>());
		for (UINT32 Index = 0u, Num = InReferences.Num<UINT32>(); Index < Num; Index++)
		{
			FMaterialSlotReference Reference = InReferences[Index];
			if (Reference.MaterialName.Length() > 0u)
			{
				if (Reference.ShaderName.Length() == 0u)
				{
					Reference.ShaderName = MakeMaterialSlotReference(Reference.MaterialName).ShaderName;
				}
				if (Reference.ProgramName.Length() == 0u)
				{
					Reference.ProgramName = GetMaterialProgramName();
				}
			}
			MaterialSlotReferences[Index] = Reference;
			MaterialSlotRequestGenerations[Index] = 0u;
			if (Reference.MaterialName.Length() > 0u)
			{
				RequestMaterialSlot(Index);
			}
		}
	}

	void PMeshComponent::SetMaterialSlotReference(UINT32 InSlotIdx, const FMaterialSlotReference& InReference)
	{
		EnsureMaterialSlotReferenceNum(InSlotIdx + 1u);
		FMaterialSlotReference Reference = InReference;
		if (Reference.MaterialName.Length() > 0u)
		{
			if (Reference.ShaderName.Length() == 0u)
			{
				const FMaterialSlotReference Derived = MakeMaterialSlotReference(Reference.MaterialName);
				Reference.ShaderName = Derived.ShaderName;
			}
			if (Reference.ProgramName.Length() == 0u)
			{
				Reference.ProgramName = GetMaterialProgramName();
			}
		}
		MaterialSlotReferences[InSlotIdx] = Reference;
		RequestMaterialSlot(InSlotIdx);
	}

	void PMeshComponent::ClearMaterialSlotReference(UINT32 InSlotIdx)
	{
		if (InSlotIdx >= MaterialSlotReferences.Num<UINT32>())
		{
			return;
		}
		MaterialSlotReferences[InSlotIdx] = FMaterialSlotReference();
		if (InSlotIdx < MaterialSlotRequestGenerations.Num<UINT32>())
		{
			MaterialSlotRequestGenerations[InSlotIdx]++;
		}
		SetMaterialInstance(InSlotIdx, nullptr);
	}

	void PMeshComponent::RefreshMaterialSlotReferences()
	{
		for (UINT32 Index = 0u, Num = MaterialSlotReferences.Num<UINT32>(); Index < Num; Index++)
		{
			if (MaterialSlotReferences[Index].MaterialName.Length() > 0u)
			{
				RequestMaterialSlot(Index);
			}
		}
	}

	void PMeshComponent::RequestMaterialSlot(UINT32 InSlotIdx)
	{
		if (InSlotIdx >= MaterialSlotReferences.Num<UINT32>())
		{
			return;
		}
		const UINT32 RequestGeneration = ++MaterialSlotRequestGenerations[InSlotIdx];
		SetMaterialInstance(InSlotIdx, nullptr);
		if (MaterialSlotReferences[InSlotIdx].MaterialName.Length() == 0u)
		{
			return;
		}
		TFunction<BOOL32()> Step = [this, InSlotIdx, RequestGeneration]() -> BOOL32
		{
			if ((InSlotIdx >= MaterialSlotRequestGenerations.Num<UINT32>()) ||
				(MaterialSlotRequestGenerations[InSlotIdx] != RequestGeneration))
			{
				return TRUE;
			}
			return TryResolveMaterialSlot(InSlotIdx);
		};
		if (Step() == FALSE)
		{
			AddDeferredApply(static_cast<TFunction<BOOL32()>&&>(Step));
		}
	}

	BOOL32 PMeshComponent::TryResolveMaterialSlot(UINT32 InSlotIdx)
	{
		if (InSlotIdx >= MaterialSlotReferences.Num<UINT32>())
		{
			return TRUE;
		}
		const FMaterialSlotReference& Reference = MaterialSlotReferences[InSlotIdx];
		if (Reference.MaterialName.Length() == 0u)
		{
			return TRUE;
		}
		// The mesh load will call RefreshMaterialSlotReferences after its vertex axis is
		// available; until then this is not an error and must not keep a deferred step alive.
		if (!GetMeshAssetBase())
		{
			return TRUE;
		}
		EShaderVertexFactoryType VertexFactory = EShaderVertexFactoryType::ENGINE_MESH;
		EShaderVertexProfile Profile;
		if (ResolveMaterialVertexAxis(VertexFactory, Profile) == FALSE)
		{
			return FALSE;
		}
		EShaderAssemblyRequest Request;
		Request.ShaderName = EName(Reference.ShaderName);
		Request.MaterialName = Reference.MaterialName;
		Request.ProgramName = EName(Reference.ProgramName);
		Request.VertexFactory = VertexFactory;
		Request.Profile = Profile;
		const EShaderInstance* Instance = REngineShaderInstanceCache::GetManagerSingleton()->RequireMaterialInstance(Request);
		if (!Instance)
		{
			return FALSE;
		}
		SetMaterialInstance(InSlotIdx, Instance);
		return TRUE;
	}

	EString PMeshComponent::GetMaterialProgramName() const
	{
		const FRenderPassDesc& PrimaryPassDescription = FRenderPassRegistry::GetDesc(ERenderPassType::RENDER_PASS_TYPE_BASE);
		return (PrimaryPassDescription.PassName != nullptr)
			? EString(PrimaryPassDescription.PassName)
			: EString(PE_TEXT("BasePass"));
	}

	BOOL32 PMeshComponent::ResolveMaterialVertexAxis(EShaderVertexFactoryType& OutVertexFactory, EShaderVertexProfile& OutProfile) const
	{
		(void)OutVertexFactory;
		(void)OutProfile;
		return FALSE;
	}

	const EShaderInstance* PMeshComponent::GetMaterialInstance(UINT32 InSlotIdx) const
	{
		if (InSlotIdx < MaterialSlots.Num<UINT32>())
		{
			return (MaterialSlots[InSlotIdx]);
		}
		return nullptr;
	}
	UINT32 PMeshComponent::GetMaterialSlotCount() const
	{
		return (MaterialSlots.Num<UINT32>());
	}
	void PMeshComponent::SetMaterialInstance(const EShaderInstance* InMaterialInstance)
	{
		if (MaterialSlots.Num<INT32>() <= 0)
		{
			MaterialSlots.Add(nullptr);
		}
		MaterialSlots[0] = InMaterialInstance;
		MarkMaterialAsDirty();
	}
	void PMeshComponent::SetMaterialInstance(UINT32 InSlotIdx, const EShaderInstance* InMaterialInstance)
	{
		if (InSlotIdx >= MaterialSlots.Num<UINT32>())
		{
			MaterialSlots.SetNum(InSlotIdx + 1);
		}
		MaterialSlots[InSlotIdx] = InMaterialInstance;
		MarkMaterialAsDirty();
	}
	void PMeshComponent::SetMaterialSlotsNum(UINT32 InNum)
	{
		const UINT32 OldNum = MaterialSlots.Num<UINT32>();
		MaterialSlots.SetNum(InNum);
		for (UINT32 i = OldNum; i < InNum; i++)
		{
			MaterialSlots[i] = nullptr;
		}
	}
	const EAssetBase* PMeshComponent::GetMeshAssetBase() const
	{
		return MeshAssetBase;
	}
	void PMeshComponent::SetMeshAssetBase(const EAssetBase* InMeshAsset)
	{
		MeshAssetBase = InMeshAsset;
	}

	void PMeshComponent::CollectRenderAssets(FPrimitiveRenderAssetReport& InOutReport) const
	{
		InOutReport.SetMeshAsset(MeshAssetBase);

		// A slot is reported under the primary pass, which is the pass a material's own shader
		// asset is assembled for. The render side resolves the other passes of the material
		// through the fallback path until materials carry a full pass table.
		const FRenderPassDesc& PrimaryPassDesc = FRenderPassRegistry::GetDesc(ERenderPassType::RENDER_PASS_TYPE_BASE);
		if (PrimaryPassDesc.PassName == nullptr)
		{
			return;
		}
		const EName PrimaryPassName(PrimaryPassDesc.PassName);
		for (UINT32 SlotIndex = 0u, SlotNum = MaterialSlots.Num<UINT32>(); SlotIndex < SlotNum; SlotIndex++)
		{
			if (MaterialSlots[SlotIndex])
			{
				InOutReport.SetMaterialInstance(SlotIndex, PrimaryPassName, MaterialSlots[SlotIndex]);
			}
		}
	}

	// Render proxy functions START
	void PMeshComponent::CreateRenderState()
	{
		PPrimitiveComponent::CreateRenderState();
		if (ShouldRender())
		{
			//TODO
		}
	}
	void PMeshComponent::DestroyRenderState()
	{
		//TODO
		PPrimitiveComponent::DestroyRenderState();
	}
	void PMeshComponent::SendUpdateRenderState()
	{
		if (ShouldRender() && IsRenderStateDirty())
		{
			//TODO
		}
		PPrimitiveComponent::SendUpdateRenderState();
	}
	void PMeshComponent::MarkRenderTransformAsDirty()
	{
		//TODO
		PPrimitiveComponent::MarkRenderTransformAsDirty();
	}
	void PMeshComponent::MarkRenderStateAsDirty()
	{
		PPrimitiveComponent::MarkRenderStateAsDirty();
	}
	void PMeshComponent::CleanMarkRenderStateDirty()
	{
		//TODO
		PPrimitiveComponent::CleanMarkRenderStateDirty();
	}
	// Render proxy functions END

}
