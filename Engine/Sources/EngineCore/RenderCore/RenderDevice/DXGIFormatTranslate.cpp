#include "DXGIFormatTranslate.h"
#include <RHI/RFormatTraits.h>

namespace PigeonEngine
{

	// One entry per RFormatType member, in enum order, in the engine's own order of
	// declaration. The pairs are name-identical by construction, which is what makes the
	// list readable against the enum it mirrors.
	struct RFormatTranslateEntry
	{
		RFormatType		Format;
		DXGI_FORMAT		DXGIFormat;
	};

	static const RFormatTranslateEntry GFormatTranslateTable[] =
	{
#define PE_RFORMAT_TRANSLATE_ENTRY(_NAME) { RFormatType::FORMAT_##_NAME, DXGI_FORMAT_##_NAME },
		PE_RFORMAT_TYPE_LIST(PE_RFORMAT_TRANSLATE_ENTRY)
#undef PE_RFORMAT_TRANSLATE_ENTRY
	};

	static constexpr UINT32 GFormatTranslateCount = static_cast<UINT32>(PE_ARRAYSIZE(GFormatTranslateTable));

	BOOL32 RDXGIFormatTranslate::TranslateToDXGI(DXGI_FORMAT& OutFormat, RFormatType InFormat) noexcept
	{
		for (UINT32 i = 0u; i < GFormatTranslateCount; i++)
		{
			if (GFormatTranslateTable[i].Format == InFormat)
			{
				OutFormat = GFormatTranslateTable[i].DXGIFormat;
				return TRUE;
			}
		}

		// The format is in the enum but not in the shared table, which means the enum grew
		// without PE_RFORMAT_TYPE_LIST growing with it. The message names the format, so the
		// missing entry is found by reading it rather than by guessing.
		PE_CHECK((ENGINE_RENDER_CORE_ERROR),
			(RFormatTraits::GetFormatName(InFormat)),
			(FALSE));
		return FALSE;
	}

	DXGI_FORMAT RDXGIFormatTranslate::ToDXGIOrUnknown(RFormatType InFormat) noexcept
	{
		DXGI_FORMAT Result = DXGI_FORMAT_UNKNOWN;
		if (TranslateToDXGI(Result, InFormat) == FALSE)
		{
			Result = DXGI_FORMAT_UNKNOWN;
		}
		return Result;
	}

}	// PigeonEngine
