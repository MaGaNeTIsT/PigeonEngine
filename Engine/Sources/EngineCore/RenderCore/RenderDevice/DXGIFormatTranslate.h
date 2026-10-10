#pragma once

#include <CoreMinimal.h>
#include <dxgi.h>
#include <RenderCommon.h>

namespace PigeonEngine
{

	/*
	* RFormatType <-> DXGI_FORMAT, for every D3D back-end.
	*
	* The engine's format enum mirrors DXGI_FORMAT (same names, same values), so this
	* table is the identity - and it is still one table, not one per back-end. Two
	* hand-maintained copies of the same 140 entries is how a format ends up supported
	* on D3D11 and silently missing on D3D12: the lookup used to answer
	* DXGI_FORMAT_UNKNOWN and the caller created a resource the API then refused, far
	* from the place that asked for it.
	*
	* The table is generated from PE_RFORMAT_TYPE_LIST in RHI/RFormatTraits.h, the same
	* list the engine's format names come from, so a format is spelled in exactly one
	* place: adding it to RFormatType means adding it to that list, and both the name and
	* the translation follow.
	*/
	class RDXGIFormatTranslate
	{
	public:
		// Answers FALSE (and leaves OutFormat alone) when the format has no DXGI
		// counterpart, and reports the format by name. A caller that can still do
		// something useful with that answer should test it; the ones that cannot use
		// ToDXGIOrUnknown.
		static BOOL32		TranslateToDXGI(DXGI_FORMAT& OutFormat, RFormatType InFormat) noexcept;

		// For callers whose format came out of RFormatTraits, where an unsupported format
		// is a bug in the engine's tables rather than a condition to handle: asserts and
		// answers DXGI_FORMAT_UNKNOWN.
		static DXGI_FORMAT	ToDXGIOrUnknown(RFormatType InFormat) noexcept;
	};

}	// PigeonEngine
