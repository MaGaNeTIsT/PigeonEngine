#include <Shader/ShaderBindingPlan.h>

#include <Editor/EditorLogManager.h>

namespace PigeonEngine
{

	EShaderBindingPlan::EShaderBindingPlan()
		: Stage(EShaderStageType::SHADER_STAGE_VERTEX)
		, bBindless(FALSE)
		, bValid(FALSE)
		, ConstantBufferBindings{}
		, ResourceBindings{}
	{
	}

	EShaderBindingPlan::~EShaderBindingPlan()
	{
		Reset();
	}

	void EShaderBindingPlan::Reset()
	{
		Stage = EShaderStageType::SHADER_STAGE_VERTEX;
		bBindless = FALSE;
		bValid = FALSE;
		if (ConstantBufferBindings.Num<UINT32>() > 0u)
		{
			ConstantBufferBindings.Empty();
		}
		if (ResourceBindings.Num<UINT32>() > 0u)
		{
			ResourceBindings.Empty();
		}
	}

	BOOL32 EShaderBindingPlan::BuildFromReflection(const EShaderReflection& InReflection, const BOOL32 InIsBindless)
	{
		Reset();

		Stage = InReflection.Stage;
		bBindless = InIsBindless;

		// Constant buffers. A culled buffer is not present in the compiled shader, so
		// binding it would be wrong; it stays visible in the reflection only.
		for (UINT32 CbIdx = 0u, CbNum = InReflection.ConstantBuffers.Num<UINT32>(); CbIdx < CbNum; CbIdx++)
		{
			const EShaderReflectionConstantBuffer& ConstantBuffer = InReflection.ConstantBuffers[CbIdx];
			if (ConstantBuffer.bCompilerCulled)
			{
				continue;
			}

			EShaderConstantBufferBinding Binding;
			Binding.BlockName = ConstantBuffer.Name;
			Binding.RegisterSlot = ConstantBuffer.Slot;
			// Register bound default: the legacy shared root signature maps one root
			// parameter per constant buffer register, so the root index equals the register
			// slot. A program that gets a signature of its own has this overwritten by the
			// layout generator with the root index that signature actually uses.
			Binding.RootIndex = ConstantBuffer.Slot;
			Binding.ByteSize = ConstantBuffer.ByteSize;
			// Root constant blocks have no view to bind; the flag travels with the binding
			// so the render layer does not have to walk the reflection to find them, and the
			// kind is the mode the binding side branches on.
			Binding.RootConstant = ConstantBuffer.RootConstant;
			Binding.Kind = (ConstantBuffer.RootConstant
				? EShaderConstantBufferBindingKindType::SHADER_CONSTANT_BUFFER_BINDING_KIND_ROOT_CONSTANTS
				: EShaderConstantBufferBindingKindType::SHADER_CONSTANT_BUFFER_BINDING_KIND_CBV);
			Binding.Num32BitValues = ((ConstantBuffer.ByteSize + 3u) / 4u);
			ConstantBufferBindings.Add(Binding);
		}

		// Shader resource views and unordered access views. A culled entry is not part of
		// the compiled shader: its slot does not exist in the root signature, so it is
		// left out of the plan and stays visible only in the reflection.
		for (UINT32 TexIdx = 0u, TexNum = InReflection.Textures.Num<UINT32>(); TexIdx < TexNum; TexIdx++)
		{
			const EShaderReflectionTexture& Texture = InReflection.Textures[TexIdx];
			if (Texture.bCompilerCulled)
			{
				continue;
			}
			EShaderResourceBinding Binding;
			Binding.Name = Texture.Name;
			Binding.Kind = EShaderBindingKindType::SHADER_BINDING_KIND_SRV;
			Binding.RegisterSlot = Texture.Slot;
			Binding.TextureType = Texture.Type;
			ResourceBindings.Add(Binding);
		}
		for (UINT32 TexIdx = 0u, TexNum = InReflection.RWTextures.Num<UINT32>(); TexIdx < TexNum; TexIdx++)
		{
			const EShaderReflectionTexture& Texture = InReflection.RWTextures[TexIdx];
			if (Texture.bCompilerCulled)
			{
				continue;
			}
			EShaderResourceBinding Binding;
			Binding.Name = Texture.Name;
			Binding.Kind = EShaderBindingKindType::SHADER_BINDING_KIND_UAV;
			Binding.RegisterSlot = Texture.Slot;
			Binding.TextureType = Texture.Type;
			ResourceBindings.Add(Binding);
		}
		for (UINT32 ResIdx = 0u, ResNum = InReflection.Resources.Num<UINT32>(); ResIdx < ResNum; ResIdx++)
		{
			const EShaderReflectionResource& Resource = InReflection.Resources[ResIdx];
			if (Resource.bCompilerCulled)
			{
				continue;
			}
			EShaderResourceBinding Binding;
			Binding.Name = Resource.Name;
			Binding.Kind = EShaderBindingKindType::SHADER_BINDING_KIND_SRV;
			Binding.RegisterSlot = Resource.Slot;
			Binding.ResourceKind = Resource.Kind;
			ResourceBindings.Add(Binding);
		}
		for (UINT32 ResIdx = 0u, ResNum = InReflection.RWResources.Num<UINT32>(); ResIdx < ResNum; ResIdx++)
		{
			const EShaderReflectionResource& Resource = InReflection.RWResources[ResIdx];
			if (Resource.bCompilerCulled)
			{
				continue;
			}
			EShaderResourceBinding Binding;
			Binding.Name = Resource.Name;
			Binding.Kind = EShaderBindingKindType::SHADER_BINDING_KIND_UAV;
			Binding.RegisterSlot = Resource.Slot;
			Binding.ResourceKind = Resource.Kind;
			ResourceBindings.Add(Binding);
		}

		// Samplers. Their slot is independent from the texture slot, so they get their
		// own entries rather than being folded into the texture binding.
		for (UINT32 SamplerIdx = 0u, SamplerNum = InReflection.Samplers.Num<UINT32>(); SamplerIdx < SamplerNum; SamplerIdx++)
		{
			const EShaderReflectionSampler& Sampler = InReflection.Samplers[SamplerIdx];
			if (Sampler.bCompilerCulled)
			{
				continue;
			}
			EShaderResourceBinding Binding;
			Binding.Name = Sampler.Name;
			Binding.Kind = EShaderBindingKindType::SHADER_BINDING_KIND_SAMPLER;
			Binding.RegisterSlot = Sampler.Slot;
			Binding.SamplerDesc = Sampler.Desc;
			ResourceBindings.Add(Binding);
		}

		// Table offsets start out equal to the register slot: the legacy shared root signature
		// gives every table range a base register of 0, so a resource sits at its register
		// inside its table. The layout generator rewrites this together with the table root
		// once the program has a signature of its own.
		for (UINT32 BindIdx = 0u, BindNum = ResourceBindings.Num<UINT32>(); BindIdx < BindNum; BindIdx++)
		{
			ResourceBindings[BindIdx].OffsetInTable = ResourceBindings[BindIdx].RegisterSlot;
		}

		bValid = TRUE;
		return TRUE;
	}

	const EShaderConstantBufferBinding* EShaderBindingPlan::GetConstantBufferBindingByIndex(const UINT32 InIndex) const
	{
		if (InIndex >= ConstantBufferBindings.Num<UINT32>())
		{
			return nullptr;
		}
		return (&(ConstantBufferBindings[InIndex]));
	}

	const EShaderResourceBinding* EShaderBindingPlan::GetResourceBindingByIndex(const UINT32 InIndex) const
	{
		if (InIndex >= ResourceBindings.Num<UINT32>())
		{
			return nullptr;
		}
		return (&(ResourceBindings[InIndex]));
	}

	EShaderConstantBufferBinding* EShaderBindingPlan::GetMutableConstantBufferBindingByIndex(const UINT32 InIndex)
	{
		if (InIndex >= ConstantBufferBindings.Num<UINT32>())
		{
			return nullptr;
		}
		return (&(ConstantBufferBindings[InIndex]));
	}

	EShaderResourceBinding* EShaderBindingPlan::GetMutableResourceBindingByIndex(const UINT32 InIndex)
	{
		if (InIndex >= ResourceBindings.Num<UINT32>())
		{
			return nullptr;
		}
		return (&(ResourceBindings[InIndex]));
	}

	const EShaderConstantBufferBinding* EShaderBindingPlan::FindConstantBufferBinding(const EName& InBlockName) const
	{
		for (UINT32 BindIdx = 0u, BindNum = ConstantBufferBindings.Num<UINT32>(); BindIdx < BindNum; BindIdx++)
		{
			if (ConstantBufferBindings[BindIdx].BlockName == InBlockName)
			{
				return (&(ConstantBufferBindings[BindIdx]));
			}
		}
		return nullptr;
	}

	const EShaderResourceBinding* EShaderBindingPlan::FindResourceBinding(const EName& InName) const
	{
		for (UINT32 BindIdx = 0u, BindNum = ResourceBindings.Num<UINT32>(); BindIdx < BindNum; BindIdx++)
		{
			if (ResourceBindings[BindIdx].Name == InName)
			{
				return (&(ResourceBindings[BindIdx]));
			}
		}
		return nullptr;
	}

	BOOL32 EShaderBindingPlan::FindConstantBufferRootIndex(const EName& InBlockName, UINT32& OutRootIndex) const
	{
		const EShaderConstantBufferBinding* Binding = FindConstantBufferBinding(InBlockName);
		if (!Binding)
		{
			return FALSE;
		}
		OutRootIndex = Binding->RootIndex;
		return TRUE;
	}

} // namespace PigeonEngine
