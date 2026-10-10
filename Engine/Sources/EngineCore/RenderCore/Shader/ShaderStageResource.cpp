#include <Shader/ShaderStageResource.h>

#include <RHI/IRRHIDevice.h>
#include <Main/MainManager.h>
#include <Editor/EditorLogManager.h>

namespace PigeonEngine
{

	// ---------------------------------------------------------------------------
	// Reflection -> render types
	// ---------------------------------------------------------------------------

	BOOL32 TranslateShaderInputSemantic(const EName& InSemanticName, RShaderSemanticType& OutSemantic)
	{
		if (InSemanticName == EName("POSITION"))
		{
			OutSemantic = RShaderSemanticType::SHADER_SEMANTIC_POSITION;
			return TRUE;
		}
		if (InSemanticName == EName("TEXCOORD"))
		{
			OutSemantic = RShaderSemanticType::SHADER_SEMANTIC_TEXCOORD;
			return TRUE;
		}
		if (InSemanticName == EName("NORMAL"))
		{
			OutSemantic = RShaderSemanticType::SHADER_SEMANTIC_NORMAL;
			return TRUE;
		}
		if (InSemanticName == EName("TANGENT"))
		{
			OutSemantic = RShaderSemanticType::SHADER_SEMANTIC_TANGENT;
			return TRUE;
		}
		if (InSemanticName == EName("BINORMAL"))
		{
			OutSemantic = RShaderSemanticType::SHADER_SEMANTIC_BINORMAL;
			return TRUE;
		}
		if (InSemanticName == EName("COLOR"))
		{
			OutSemantic = RShaderSemanticType::SHADER_SEMANTIC_COLOR;
			return TRUE;
		}
		if (InSemanticName == EName("BLENDWEIGHT"))
		{
			OutSemantic = RShaderSemanticType::SHADER_SEMANTIC_BLENDWEIGHT;
			return TRUE;
		}
		if (InSemanticName == EName("BLENDINDICES"))
		{
			OutSemantic = RShaderSemanticType::SHADER_SEMANTIC_BLENDINDICES;
			return TRUE;
		}
		return FALSE;
	}

	BOOL32 TranslateShaderStageToRHI(const EShaderStageType InStage, ERHIShaderStage& OutStage)
	{
		switch (InStage)
		{
		case EShaderStageType::SHADER_STAGE_VERTEX:
		{
			OutStage = ERHIShaderStage::RHI_SHADER_STAGE_VERTEX;
			return TRUE;
		}
		case EShaderStageType::SHADER_STAGE_PIXEL:
		{
			OutStage = ERHIShaderStage::RHI_SHADER_STAGE_PIXEL;
			return TRUE;
		}
		case EShaderStageType::SHADER_STAGE_COMPUTE:
		{
			OutStage = ERHIShaderStage::RHI_SHADER_STAGE_COMPUTE;
			return TRUE;
		}
		default:
		{
			OutStage = ERHIShaderStage::RHI_SHADER_STAGE_COUNT;
			return FALSE;
		}
		}
	}

	BOOL32 BuildShaderInputLayouts(const TArray<EShaderReflectionInputLayout>& InReflectionLayouts, TArray<RInputLayoutDesc>& OutLayouts)
	{
		if (OutLayouts.Num<UINT32>() > 0u)
		{
			OutLayouts.Empty();
		}
		if (InReflectionLayouts.Num<UINT32>() == 0u)
		{
			return TRUE;
		}

		for (UINT32 LayoutIdx = 0u, LayoutNum = InReflectionLayouts.Num<UINT32>(); LayoutIdx < LayoutNum; LayoutIdx++)
		{
			const EShaderReflectionInputLayout& ReflectionLayout = InReflectionLayouts[LayoutIdx];
			if (ReflectionLayout.Format.IsMatrix())
			{
				PE_LOG_ERROR(EString("[ShaderStageResource] Input '") + ReflectionLayout.Name.ToString()
					+ EString("' is a matrix attribute, which the render input layout cannot express."));
				return FALSE;
			}

			RShaderSemanticType Semantic = RShaderSemanticType::SHADER_SEMANTIC_NONE;
			if (!TranslateShaderInputSemantic(ReflectionLayout.Semantic, Semantic))
			{
				PE_LOG_ERROR(EString("[ShaderStageResource] Input '") + ReflectionLayout.Name.ToString()
					+ EString("' uses the unknown semantic '") + ReflectionLayout.Semantic.ToString() + EString("'."));
				return FALSE;
			}

			RInputLayoutFormatType MemberFormat = RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_FLOAT;
			switch (ReflectionLayout.Format.Prefix)
			{
			case EShaderFieldFormatPrefixType::SHADER_FIELD_FORMAT_PREFIX_FLOAT:
			{
				MemberFormat = RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_FLOAT;
				break;
			}
			case EShaderFieldFormatPrefixType::SHADER_FIELD_FORMAT_PREFIX_INT:
			{
				MemberFormat = RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_INT;
				break;
			}
			case EShaderFieldFormatPrefixType::SHADER_FIELD_FORMAT_PREFIX_UINT:
			{
				MemberFormat = RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_UINT;
				break;
			}
			default:
			{
				break;
			}
			}

			// The index rides inside the semantic value, so the ctor has to be used
			// rather than assigning SemanticName directly: RInputLayoutDesc folds
			// InSemanticIndex into the low 4 bits, which is what the back-ends read.
			RInputLayoutDesc LayoutDesc(Semantic
				, ReflectionLayout.Format.GetScalarByteSize(), ReflectionLayout.Format.GetColumnNum(), MemberFormat
				, ReflectionLayout.SemanticIndex, ReflectionLayout.Slot);
			OutLayouts.Add(LayoutDesc);
		}
		return TRUE;
	}

	// ---------------------------------------------------------------------------
	// RShaderStageResource
	// ---------------------------------------------------------------------------

	RShaderStageResource::RShaderStageResource()
		: RHIShader(nullptr)
		, RHIShaderStage(ERHIShaderStage::RHI_SHADER_STAGE_COUNT)
		, InputLayouts{}
	{
	}

	RShaderStageResource::~RShaderStageResource()
	{
		ReleaseRenderResource();
	}

	BOOL8 RShaderStageResource::IsRenderResourceValid() const noexcept
	{
		return ((RHIShader != nullptr) ? TRUE : FALSE);
	}

	BOOL8 RShaderStageResource::InitRenderResource() noexcept
	{
		// The shader object is created by CreateRenderResource, which needs the
		// bytecode and the input layout; there is nothing left to do here.
		return IsRenderResourceValid();
	}

	void RShaderStageResource::ReleaseRenderResource() noexcept
	{
		if (RHIShader)
		{
			// IRRHIShader derives from IRRHIResource, which declares a virtual
			// destructor, so deleting through the interface is safe on both back-ends.
			delete RHIShader;
			RHIShader = nullptr;
		}
		RHIShaderStage = ERHIShaderStage::RHI_SHADER_STAGE_COUNT;
		if (InputLayouts.Num<UINT32>() > 0u)
		{
			InputLayouts.Empty();
		}
	}

	BOOL32 RShaderStageResource::CreateRenderResource(const void* InBytecode, const ULONG InByteSize
		, const ERHIShaderStage InRHIShaderStage, const TArray<EShaderReflectionInputLayout>& InReflectionLayouts)
	{
		if ((!InBytecode) || (InByteSize == 0u))
		{
			PE_LOG_ERROR(EString("[ShaderStageResource] Cannot create a shader from empty bytecode."));
			return FALSE;
		}
		if (RHIShader)
		{
			PE_LOG_ERROR(EString("[ShaderStageResource] The render resource is already created."));
			return FALSE;
		}

		IRRHIDevice* RenderDevice = EMainManager::GetManagerSingleton()->GetRenderDevice();
		if (!RenderDevice)
		{
			// No device yet: the caller retries once the render device is alive.
			return FALSE;
		}

		if (!BuildShaderInputLayouts(InReflectionLayouts, InputLayouts))
		{
			return FALSE;
		}

		// The RHI takes shared ownership of the bytecode. A shared_ptr of UINT8 over
		// an array needs the array deleter, otherwise the bytes would be freed with
		// scalar delete.
		TSharedPtr<UINT8> ByteCodeShared(new UINT8[InByteSize], std::default_delete<UINT8[]>());
		::memcpy_s(ByteCodeShared.get(), static_cast<rsize_t>(InByteSize), InBytecode, static_cast<rsize_t>(InByteSize));

		RRHIShaderDesc Desc(ByteCodeShared, static_cast<SIZE_T>(InByteSize), InRHIShaderStage);
		Desc.InputLayouts = InputLayouts;

		IRRHIShader* OutShader = nullptr;
		if ((!RenderDevice->CreateShader(Desc, &OutShader)) || (!OutShader))
		{
			PE_LOG_ERROR(EString("[ShaderStageResource] The RHI failed to create the shader object."));
			return FALSE;
		}

		RHIShader = OutShader;
		RHIShaderStage = InRHIShaderStage;
		return TRUE;
	}

} // namespace PigeonEngine
