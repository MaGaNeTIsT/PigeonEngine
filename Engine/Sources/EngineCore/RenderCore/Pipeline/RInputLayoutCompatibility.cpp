#include <Pipeline/RInputLayoutCompatibility.h>

namespace PigeonEngine
{

	ERHIInputElementClassType GetInputElementClass(const RInputLayoutFormatType InFormat) noexcept
	{
		switch (InFormat)
		{
		case RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_FLOAT:
		case RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_UNORM:
			// UNORM is in the float class on purpose: the input assembler normalizes it into the
			// shader's float components, which is what makes packed color data bindable to a float4.
			return ERHIInputElementClassType::RHI_INPUT_ELEMENT_CLASS_FLOAT;
		case RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_INT:
			return ERHIInputElementClassType::RHI_INPUT_ELEMENT_CLASS_INT;
		case RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_UINT:
			return ERHIInputElementClassType::RHI_INPUT_ELEMENT_CLASS_UINT;
		default:
			return ERHIInputElementClassType::RHI_INPUT_ELEMENT_CLASS_UNKNOWN;
		}
	}

	BOOL32 IsInputElementExpressible(const RInputLayoutDesc& InElement) noexcept
	{
		// Widths the API has formats for. 0 is deliberately not among them: a descriptor that does
		// not say how wide its scalars are cannot be turned into a format, and the engine's own
		// layouts always spell the width out.
		if ((InElement.MemberStride != 1u) && (InElement.MemberStride != 2u) && (InElement.MemberStride != 4u))
		{
			return FALSE;
		}
		if ((InElement.MemberNum < 1u) || (InElement.MemberNum > 4u))
		{
			return FALSE;
		}
		if (GetInputElementClass(InElement.MemberFormat) == ERHIInputElementClassType::RHI_INPUT_ELEMENT_CLASS_UNKNOWN)
		{
			return FALSE;
		}
		if ((InElement.MemberFormat == RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_FLOAT) && (InElement.MemberStride == 1u))
		{
			// There is no 8 bit float format: an 8 bit element can only be integer or normalized.
			return FALSE;
		}
		if ((InElement.MemberNum == 3u) && (InElement.MemberStride != 4u))
		{
			// Packed three component formats do not exist (no R8G8B8 / R16G16B16).
			return FALSE;
		}
		if ((InElement.MemberFormat == RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_UNORM) && (InElement.MemberStride == 4u))
		{
			// There is no 32 bit normalized format; 10:10:10:2 data cannot be told apart from this
			// descriptor's three values, so it is not expressible here either.
			return FALSE;
		}
		return TRUE;
	}

	ERHIInputElementMatchType MatchInputElement(const RInputLayoutDesc& InDataElement, const RInputLayoutDesc& InShaderElement) noexcept
	{
		// The semantic carries its own index in the low bits, so equality covers both.
		if (InDataElement.SemanticName != InShaderElement.SemanticName)
		{
			return ERHIInputElementMatchType::RHI_INPUT_ELEMENT_MISSING_IN_DATA;
		}
		if (IsInputElementExpressible(InDataElement) == FALSE)
		{
			return ERHIInputElementMatchType::RHI_INPUT_ELEMENT_DATA_NOT_EXPRESSIBLE;
		}
		const ERHIInputElementClassType DataClass = GetInputElementClass(InDataElement.MemberFormat);
		const ERHIInputElementClassType ShaderClass = GetInputElementClass(InShaderElement.MemberFormat);
		if ((DataClass == ERHIInputElementClassType::RHI_INPUT_ELEMENT_CLASS_UNKNOWN)
			|| (ShaderClass == ERHIInputElementClassType::RHI_INPUT_ELEMENT_CLASS_UNKNOWN)
			|| (DataClass != ShaderClass))
		{
			return ERHIInputElementMatchType::RHI_INPUT_ELEMENT_CLASS_MISMATCH;
		}
		if (InDataElement.MemberNum < InShaderElement.MemberNum)
		{
			return ERHIInputElementMatchType::RHI_INPUT_ELEMENT_MATCH_WITH_DEFAULTS;
		}
		return ERHIInputElementMatchType::RHI_INPUT_ELEMENT_MATCH;
	}

	BOOL32 IsInputElementMatchFatal(const ERHIInputElementMatchType InMatch) noexcept
	{
		// A short element stays usable: the input assembler defaults the components the shader reads
		// beyond it (0 for xyz, 1 for w), so the pipeline can be created and the caller only has to
		// be told about it.
		return (((InMatch == ERHIInputElementMatchType::RHI_INPUT_ELEMENT_CLASS_MISMATCH)
			|| (InMatch == ERHIInputElementMatchType::RHI_INPUT_ELEMENT_MISSING_IN_DATA)
			|| (InMatch == ERHIInputElementMatchType::RHI_INPUT_ELEMENT_DATA_NOT_EXPRESSIBLE)) ? TRUE : FALSE);
	}

	BOOL32 BuildPipelineInputLayout(const RInputLayoutDesc* InShaderInputs, const UINT32 InShaderInputCount
		, const RInputLayoutDesc* InDeclaredLayouts, const UINT32 InDeclaredLayoutCount
		, TArray<RInputLayoutDesc>& OutLayouts, TArray<RInputElementPair>& OutPairs
		, TArray<INT32>& OutUnmatchedDeclaredIndices)
	{
		OutLayouts.Empty();
		OutPairs.Empty();
		OutUnmatchedDeclaredIndices.Empty();
		if ((!InShaderInputs) || (InShaderInputCount == 0u))
		{
			// Nothing the shader reads, so every declaration names no input. Reported like any other
			// unmatched declaration instead of being dropped in silence.
			for (UINT32 DataIndex = 0u; DataIndex < InDeclaredLayoutCount; DataIndex++)
			{
				OutUnmatchedDeclaredIndices.Add(static_cast<INT32>(DataIndex));
			}
			return TRUE;
		}

		// Start from what the shader declares: an element the caller did not mention keeps it, which
		// is the layout the engine has always created and therefore the one the meshes are packed for.
		OutLayouts.SetNum(InShaderInputCount);
		for (UINT32 ShaderIndex = 0u; ShaderIndex < InShaderInputCount; ShaderIndex++)
		{
			OutLayouts[ShaderIndex] = InShaderInputs[ShaderIndex];
		}

		OutPairs.SetNum(InShaderInputCount);
		for (UINT32 ShaderIndex = 0u; ShaderIndex < InShaderInputCount; ShaderIndex++)
		{
			RInputElementPair& Pair = OutPairs[ShaderIndex];
			Pair.ShaderElementIndex = static_cast<INT32>(ShaderIndex);
			// No declaration for this input: the shader's own element is what the pipeline gets.
			Pair.Match = ERHIInputElementMatchType::RHI_INPUT_ELEMENT_MATCH;
		}

		BOOL32 bAllDeclaredUsable = TRUE;
		for (UINT32 DataIndex = 0u; DataIndex < InDeclaredLayoutCount; DataIndex++)
		{
			if (!InDeclaredLayouts)
			{
				break;
			}
			const RInputLayoutDesc& DeclaredElement = InDeclaredLayouts[DataIndex];

			// Find the shader input this declaration feeds. A declaration that names no such input is
			// not an error: a stream can carry attributes for another pass.
			INT32 MatchedShaderIndex = -1;
			for (UINT32 ShaderIndex = 0u; ShaderIndex < InShaderInputCount; ShaderIndex++)
			{
				if ((InShaderInputs) && (InShaderInputs[ShaderIndex].SemanticName == DeclaredElement.SemanticName))
				{
					MatchedShaderIndex = static_cast<INT32>(ShaderIndex);
					break;
				}
			}
			if (MatchedShaderIndex < 0)
			{
				// Named no shader input. Reported rather than refused: a stream may legitimately carry
				// attributes another pass reads.
				OutUnmatchedDeclaredIndices.Add(static_cast<INT32>(DataIndex));
				continue;
			}

			const UINT32 ShaderIndex = static_cast<UINT32>(MatchedShaderIndex);
			const ERHIInputElementMatchType Match = MatchInputElement(DeclaredElement, InShaderInputs[ShaderIndex]);
			RInputElementPair& Pair = OutPairs[ShaderIndex];
			// A first declaration that already failed is what the caller has to be told about, so a
			// later declaration of the same semantic must not overwrite that verdict with a passing
			// one - the pipeline is refused either way and a silent refusal helps nobody.
			const BOOL32 bAlreadyFatal = IsInputElementMatchFatal(Pair.Match);
			Pair.DataElementIndex = static_cast<INT32>(DataIndex);
			if (bAlreadyFatal == FALSE)
			{
				Pair.Match = Match;
			}
			if (IsInputElementMatchFatal(Match) != FALSE)
			{
				// Keep the shader's element: a pipeline built against a declaration the API cannot
				// express would only fail later, and the caller is told about the element instead.
				bAllDeclaredUsable = FALSE;
				continue;
			}

			// Only the data shape comes from the declaration. Where the element sits is the shader's
			// answer: the stream slot is what the draw binds its vertex buffers by (see the proxies'
			// SetVertexBuffers), so a declaration that moved an attribute to another slot would send
			// the input assembler to the wrong buffer - silently, because the pairing itself is fine.
			// Instance classification is part of that position rather than of the format, so it stays
			// with the shader too.
			RInputLayoutDesc Merged = DeclaredElement;
			Merged.SemanticName			= InShaderInputs[ShaderIndex].SemanticName;
			Merged.InputSlot			= InShaderInputs[ShaderIndex].InputSlot;
			Merged.InputSlotClass		= InShaderInputs[ShaderIndex].InputSlotClass;
			Merged.InstanceDataStepRate	= InShaderInputs[ShaderIndex].InstanceDataStepRate;
			OutLayouts[ShaderIndex] = Merged;
		}
		return bAllDeclaredUsable;
	}

	const CHAR* GetInputElementMatchTypeString(const ERHIInputElementMatchType InMatch) noexcept
	{
		switch (InMatch)
		{
		case ERHIInputElementMatchType::RHI_INPUT_ELEMENT_MATCH:
			return "match";
		case ERHIInputElementMatchType::RHI_INPUT_ELEMENT_MATCH_WITH_DEFAULTS:
			return "data supplies fewer components than the shader reads (missing ones read as 0 / 1)";
		case ERHIInputElementMatchType::RHI_INPUT_ELEMENT_CLASS_MISMATCH:
			return "scalar class mismatch (float / int / uint)";
		case ERHIInputElementMatchType::RHI_INPUT_ELEMENT_MISSING_IN_DATA:
			return "the data layout has no element with this semantic";
		case ERHIInputElementMatchType::RHI_INPUT_ELEMENT_DATA_NOT_EXPRESSIBLE:
			return "no API format exists for this scalar width / component count / format";
		default:
			return "unknown";
		}
	}

	EString MakeInputElementText(const RInputLayoutDesc& InElement)
	{
		// Numbers rather than format names: they are what a caller has to compare against its own
		// layout, and they never disagree with a second name table. The semantic index is the low
		// nibble, which is what the back-ends read back. "scalarBytes" is the width of one scalar,
		// not the vertex stride, which comes from the buffer binding.
		EString Text;
		Text += GetShaderSemanticNameString(InElement.SemanticName);
		Text += EString(PE_TEXT("[")) + EString::FromInt(static_cast<INT32>(static_cast<UINT32>(InElement.SemanticName) & 0xfu));
		Text += EString(PE_TEXT("] slot=")) + EString::FromInt(static_cast<INT32>(InElement.InputSlot));
		Text += EString(PE_TEXT(" scalarBytes=")) + EString::FromInt(static_cast<INT32>(InElement.MemberStride));
		Text += EString(PE_TEXT(" num=")) + EString::FromInt(static_cast<INT32>(InElement.MemberNum));
		Text += EString(PE_TEXT(" format=")) + EString::FromInt(static_cast<INT32>(InElement.MemberFormat));
		if (InElement.UsesAppendOffset() != FALSE)
		{
			Text += EString(PE_TEXT(" offset=append"));
		}
		else
		{
			Text += EString(PE_TEXT(" offset=")) + EString::FromInt(static_cast<INT32>(InElement.AlignedByteOffset));
		}
		return Text;
	}

}	// PigeonEngine
