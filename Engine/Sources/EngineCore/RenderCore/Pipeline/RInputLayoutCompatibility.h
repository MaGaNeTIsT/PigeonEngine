#pragma once

#include <CoreMinimal.h>
#include <RenderCommon.h>

namespace PigeonEngine
{

	/*
	* RenderCore/Pipeline - input layout compatibility.
	*
	* A pipeline's input layout describes the DATA: what the vertex buffer actually holds
	* (see RInputLayoutDesc). The shader declares what it wants to read, for example a float4
	* COLOR0 or an int4 BLENDINDICES0, and the two do not have to be equal - the input assembler
	* converts the stored format into the shader's component type, so UNORM bytes feed a float4
	* just as well as four floats do. What the back-ends do with a bad pairing differs (D3D12 fails
	* the pipeline creation, D3D11 warns and reinterprets the register), and neither names the
	* element that caused it, which is why the engine checks the pairing itself and reports which
	* element is at fault.
	*
	* A pairing is compatible when
	*   - the semantic and its index are equal, and
	*   - the data element's format is one the API can express at all, and
	*   - both sides are in the same scalar class: FLOAT / UNORM data with a float shader field,
	*     INT with an int field, UINT with a uint field.
	* The class rule is the engine's, not the API's: D3D12 lets the pipeline creation fail on a
	* mismatched class, while D3D11 only warns and reinterprets the register, which is a silent
	* wrong result. Refusing it here is deliberately stricter than D3D11 alone would be.
	*
	* The expressive rule is the intersection of the two back-ends' format tables, so it never
	* accepts something a back-end would then refuse. (D3D11's table still maps an 8 bit FLOAT to a
	* normalized format, a legacy entry this rule makes unreachable.)
	*
	* Component counts may differ. Data with fewer components than the shader reads leaves the
	* missing ones at the API's defaults (0 for xyz, 1 for w); data with more carries bytes the
	* shader never reads (which is how the engine already feeds float3 POSITION from a four float
	* stream). Both directions are reported distinctly so a deliberate widening is not confused
	* with an accidental truncation, and only the first of them is worth a warning.
	*/

	enum ERHIInputElementClassType : UINT8
	{
		// FLOAT and UNORM both feed a float shader input: the input assembler converts.
		RHI_INPUT_ELEMENT_CLASS_FLOAT	= 0,
		RHI_INPUT_ELEMENT_CLASS_INT		= 1,
		RHI_INPUT_ELEMENT_CLASS_UINT	= 2,
		RHI_INPUT_ELEMENT_CLASS_UNKNOWN	= 3,
		RHI_INPUT_ELEMENT_CLASS_COUNT
	};

	enum ERHIInputElementMatchType : UINT8
	{
		// Same semantic and index, same class, data supplies at least the components the shader reads.
		RHI_INPUT_ELEMENT_MATCH					= 0,
		// Same class, data supplies fewer components: the shader reads the API's defaults for the
		// rest. Legal, and usually an authoring mistake.
		RHI_INPUT_ELEMENT_MATCH_WITH_DEFAULTS	= 1,
		// Different scalar class: the pipeline creation fails (D3D12) or the register is
		// reinterpreted (D3D11), so the engine refuses it either way.
		RHI_INPUT_ELEMENT_CLASS_MISMATCH		= 2,
		// The shader declares an input the data layout has no element for.
		RHI_INPUT_ELEMENT_MISSING_IN_DATA		= 3,
		// The data element's (scalar width, component count, format) combination has no API format
		// at all, so no pipeline can read it as described.
		RHI_INPUT_ELEMENT_DATA_NOT_EXPRESSIBLE	= 4,
		RHI_INPUT_ELEMENT_MATCH_TYPE_COUNT
	};

	// One shader input and the data element that feeds it. Match is MATCH with no data element when
	// the caller declared nothing for this input, in which case the shader's own element is what the
	// pipeline gets (see BuildPipelineInputLayout).
	struct RInputElementPair
	{
	public:
		ERHIInputElementMatchType	Match				= ERHIInputElementMatchType::RHI_INPUT_ELEMENT_MISSING_IN_DATA;
		// Index into the shader input array, and into the data layout array (-1 when nothing matched).
		INT32						ShaderElementIndex	= -1;
		INT32						DataElementIndex	= -1;
	};

	// Scalar class of one element's format. UNKNOWN for an element whose format was never set
	// (RInputLayoutFormatType::COUNT), which no pairing can match.
	PE_NODISCARD extern ERHIInputElementClassType	GetInputElementClass(const RInputLayoutFormatType InFormat) noexcept;

	// Compares one data element against the shader input it is meant to feed.
	PE_NODISCARD extern ERHIInputElementMatchType	MatchInputElement(const RInputLayoutDesc& InDataElement, const RInputLayoutDesc& InShaderElement) noexcept;

	/*
	* TRUE when the element's (scalar width, component count, format class) combination has an API
	* format, i.e. a pipeline can be created that reads it as described. Element widths / counts the
	* API has no format for are 8 bit FLOAT, 3 component 8 or 16 bit (there is no R8G8B8 /
	* R16G16B16) and 32 bit UNORM, and a width outside 1 / 2 / 4 names nothing at all.
	*
	* Kept in step with the two back-ends' format tables: this answers "is the descriptor usable",
	* while those tables answer "which format is it". A descriptor this refuses would reach them as
	* an unknown format, which is why the check is here instead of in a pipeline creation failure.
	*/
	PE_NODISCARD extern BOOL32	IsInputElementExpressible(const RInputLayoutDesc& InElement) noexcept;

	// A match that makes the pairing unusable: a class mismatch, a shader input the data layout has
	// no element for, or a data element no API format exists for. A short element is usable - the
	// API defaults the components the shader reads beyond it - so it is a warning, not a refusal.
	PE_NODISCARD extern BOOL32	IsInputElementMatchFatal(const ERHIInputElementMatchType InMatch) noexcept;

	/*
	* Builds the input layout a pipeline should be created with: the shader's own elements, with
	* every element the caller declared replacing the one that feeds the same semantic. A caller
	* therefore only declares the elements whose real data differs from what the shader assumes
	* (a packed color stream, a half float position) and leaves the rest alone - declaring only
	* part of the data is the normal case, not an omission.
	*
	* Only the data shape is taken from a declaration: its scalar width, component count, format and
	* byte offset. Where an element sits (its stream slot and its per vertex / per instance
	* classification) comes from the shader, because that is what the draw binds its vertex buffers
	* by; a declaration cannot move an attribute to another slot.
	*
	* The order is the shader's, so the API's appending offsets stay aligned with the way the engine
	* binds its streams, and the result is what the pipeline state has to declare.
	*
	* OutPairs gets one entry per shader input: the pairing it ended up with, with the declared
	* element's index when one replaced it (a pair with no declared element keeps the shader's own
	* layout). OutUnmatchedDeclaredIndices lists the declarations that named no shader input - not
	* an error, since a stream can carry attributes for another pass, but worth reporting because it
	* is also what a mistyped semantic looks like.
	*
	* Returns TRUE when every declared element is usable (see IsInputElementMatchFatal); an element
	* that is not usable keeps the shader's own, so a FALSE return means the caller's declaration
	* could not be honored.
	*/
	PE_NODISCARD extern BOOL32	BuildPipelineInputLayout(const RInputLayoutDesc* InShaderInputs, const UINT32 InShaderInputCount
		, const RInputLayoutDesc* InDeclaredLayouts, const UINT32 InDeclaredLayoutCount
		, TArray<RInputLayoutDesc>& OutLayouts, TArray<RInputElementPair>& OutPairs
		, TArray<INT32>& OutUnmatchedDeclaredIndices);

	// Diagnostics for logs. Both return text that never fails to build, so a caller can log a
	// mismatch unconditionally.
	PE_NODISCARD extern const CHAR*	GetInputElementMatchTypeString(const ERHIInputElementMatchType InMatch) noexcept;
	PE_NODISCARD extern EString		MakeInputElementText(const RInputLayoutDesc& InElement);

}	// PigeonEngine
