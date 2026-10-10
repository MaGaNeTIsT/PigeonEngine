#include <RHI/RFormatTraits.h>

namespace PigeonEngine
{

	// -------------------------------------------------------------------------
	// Format families.
	//
	// One row per family - the formats that share a bit layout and can therefore view
	// each other's resources. This mirrors the D3D format compatibility (format
	// casting) table; the engine lists the members it can name, and a family it does
	// not list is reported as an unknown format rather than guessed at.
	//
	// The named roles are what the queries read:
	//   Typeless   storage format without an interpretation - what a resource is
	//              created with when its views need more than one interpretation
	//   Linear     the family's plain (non-sRGB) member; the counterpart of SRGB
	//   SRGB       the member that decodes on read and encodes on write
	//   DepthDSV   the depth-stencil view format of a depth family
	//   DepthSRV   the shader-visible format that pairs with DepthDSV
	//   Members    every member, FORMAT_UNKNOWN terminated
	//
	// Adding a format is adding it here: to the role it plays and to the member list of
	// the family it shares a layout with. Nothing else in the engine names formats.
	// -------------------------------------------------------------------------
	struct RFormatFamilyDesc
	{
		RFormatType		Typeless;
		RFormatType		Linear;
		RFormatType		SRGB;
		RFormatType		DepthDSV;
		RFormatType		DepthSRV;
		RFormatType		Members[8];
	};

	static const RFormatFamilyDesc GFormatFamilies[] =
	{
		// 32 bit per channel
		{ FORMAT_R32G32B32A32_TYPELESS, FORMAT_R32G32B32A32_FLOAT, FORMAT_UNKNOWN, FORMAT_UNKNOWN, FORMAT_UNKNOWN,
			{ FORMAT_R32G32B32A32_TYPELESS, FORMAT_R32G32B32A32_FLOAT, FORMAT_R32G32B32A32_UINT, FORMAT_R32G32B32A32_SINT, FORMAT_UNKNOWN } },
		{ FORMAT_R32G32B32_TYPELESS, FORMAT_R32G32B32_FLOAT, FORMAT_UNKNOWN, FORMAT_UNKNOWN, FORMAT_UNKNOWN,
			{ FORMAT_R32G32B32_TYPELESS, FORMAT_R32G32B32_FLOAT, FORMAT_R32G32B32_UINT, FORMAT_R32G32B32_SINT, FORMAT_UNKNOWN } },
		// 16 bit per channel, 4 channels
		{ FORMAT_R16G16B16A16_TYPELESS, FORMAT_R16G16B16A16_FLOAT, FORMAT_UNKNOWN, FORMAT_UNKNOWN, FORMAT_UNKNOWN,
			{ FORMAT_R16G16B16A16_TYPELESS, FORMAT_R16G16B16A16_FLOAT, FORMAT_R16G16B16A16_UNORM, FORMAT_R16G16B16A16_UINT,
			  FORMAT_R16G16B16A16_SNORM, FORMAT_R16G16B16A16_SINT, FORMAT_UNKNOWN } },
		{ FORMAT_R32G32_TYPELESS, FORMAT_R32G32_FLOAT, FORMAT_UNKNOWN, FORMAT_UNKNOWN, FORMAT_UNKNOWN,
			{ FORMAT_R32G32_TYPELESS, FORMAT_R32G32_FLOAT, FORMAT_R32G32_UINT, FORMAT_R32G32_SINT, FORMAT_UNKNOWN } },
		// Depth with a 32 bit stencil field: D32_FLOAT_S8X24_UINT and friends
		{ FORMAT_R32G8X24_TYPELESS, FORMAT_UNKNOWN, FORMAT_UNKNOWN, FORMAT_D32_FLOAT_S8X24_UINT, FORMAT_R32_FLOAT_X8X24_TYPELESS,
			{ FORMAT_R32G8X24_TYPELESS, FORMAT_D32_FLOAT_S8X24_UINT, FORMAT_R32_FLOAT_X8X24_TYPELESS, FORMAT_X32_TYPELESS_G8X24_UINT, FORMAT_UNKNOWN } },
		{ FORMAT_R10G10B10A2_TYPELESS, FORMAT_R10G10B10A2_UNORM, FORMAT_UNKNOWN, FORMAT_UNKNOWN, FORMAT_UNKNOWN,
			{ FORMAT_R10G10B10A2_TYPELESS, FORMAT_R10G10B10A2_UNORM, FORMAT_R10G10B10A2_UINT, FORMAT_UNKNOWN } },
		// No typeless member: nothing else to view these as
		{ FORMAT_UNKNOWN, FORMAT_R11G11B10_FLOAT, FORMAT_UNKNOWN, FORMAT_UNKNOWN, FORMAT_UNKNOWN,
			{ FORMAT_R11G11B10_FLOAT, FORMAT_UNKNOWN } },
		// The 8 bit family: the one the engine's textures and LDR targets live in
		{ FORMAT_R8G8B8A8_TYPELESS, FORMAT_R8G8B8A8_UNORM, FORMAT_R8G8B8A8_UNORM_SRGB, FORMAT_UNKNOWN, FORMAT_UNKNOWN,
			{ FORMAT_R8G8B8A8_TYPELESS, FORMAT_R8G8B8A8_UNORM, FORMAT_R8G8B8A8_UNORM_SRGB, FORMAT_R8G8B8A8_UINT,
			  FORMAT_R8G8B8A8_SNORM, FORMAT_R8G8B8A8_SINT, FORMAT_UNKNOWN } },
		{ FORMAT_R16G16_TYPELESS, FORMAT_R16G16_FLOAT, FORMAT_UNKNOWN, FORMAT_UNKNOWN, FORMAT_UNKNOWN,
			{ FORMAT_R16G16_TYPELESS, FORMAT_R16G16_FLOAT, FORMAT_R16G16_UNORM, FORMAT_R16G16_UINT,
			  FORMAT_R16G16_SNORM, FORMAT_R16G16_SINT, FORMAT_UNKNOWN } },
		// Single 32 bit channel; D32_FLOAT shares the layout, so it is the depth member here
		{ FORMAT_R32_TYPELESS, FORMAT_R32_FLOAT, FORMAT_UNKNOWN, FORMAT_D32_FLOAT, FORMAT_R32_FLOAT,
			{ FORMAT_R32_TYPELESS, FORMAT_D32_FLOAT, FORMAT_R32_FLOAT, FORMAT_R32_UINT, FORMAT_R32_SINT, FORMAT_UNKNOWN } },
		// Depth plus 8 bit stencil
		{ FORMAT_R24G8_TYPELESS, FORMAT_UNKNOWN, FORMAT_UNKNOWN, FORMAT_D24_UNORM_S8_UINT, FORMAT_R24_UNORM_X8_TYPELESS,
			{ FORMAT_R24G8_TYPELESS, FORMAT_D24_UNORM_S8_UINT, FORMAT_R24_UNORM_X8_TYPELESS, FORMAT_X24_TYPELESS_G8_UINT, FORMAT_UNKNOWN } },
		{ FORMAT_R8G8_TYPELESS, FORMAT_R8G8_UNORM, FORMAT_UNKNOWN, FORMAT_UNKNOWN, FORMAT_UNKNOWN,
			{ FORMAT_R8G8_TYPELESS, FORMAT_R8G8_UNORM, FORMAT_R8G8_UINT, FORMAT_R8G8_SNORM, FORMAT_R8G8_SINT, FORMAT_UNKNOWN } },
		// Single 16 bit channel; D16_UNORM shares the layout
		{ FORMAT_R16_TYPELESS, FORMAT_R16_UNORM, FORMAT_UNKNOWN, FORMAT_D16_UNORM, FORMAT_R16_UNORM,
			{ FORMAT_R16_TYPELESS, FORMAT_R16_FLOAT, FORMAT_D16_UNORM, FORMAT_R16_UNORM, FORMAT_R16_UINT,
			  FORMAT_R16_SNORM, FORMAT_R16_SINT, FORMAT_UNKNOWN } },
		{ FORMAT_R8_TYPELESS, FORMAT_R8_UNORM, FORMAT_UNKNOWN, FORMAT_UNKNOWN, FORMAT_UNKNOWN,
			{ FORMAT_R8_TYPELESS, FORMAT_R8_UNORM, FORMAT_R8_UINT, FORMAT_R8_SNORM, FORMAT_R8_SINT, FORMAT_UNKNOWN } },
		// Formats with a layout of their own
		{ FORMAT_UNKNOWN, FORMAT_A8_UNORM, FORMAT_UNKNOWN, FORMAT_UNKNOWN, FORMAT_UNKNOWN, { FORMAT_A8_UNORM, FORMAT_UNKNOWN } },
		{ FORMAT_UNKNOWN, FORMAT_R1_UNORM, FORMAT_UNKNOWN, FORMAT_UNKNOWN, FORMAT_UNKNOWN, { FORMAT_R1_UNORM, FORMAT_UNKNOWN } },
		{ FORMAT_UNKNOWN, FORMAT_R9G9B9E5_SHAREDEXP, FORMAT_UNKNOWN, FORMAT_UNKNOWN, FORMAT_UNKNOWN, { FORMAT_R9G9B9E5_SHAREDEXP, FORMAT_UNKNOWN } },
		{ FORMAT_UNKNOWN, FORMAT_R8G8_B8G8_UNORM, FORMAT_UNKNOWN, FORMAT_UNKNOWN, FORMAT_UNKNOWN, { FORMAT_R8G8_B8G8_UNORM, FORMAT_UNKNOWN } },
		{ FORMAT_UNKNOWN, FORMAT_G8R8_G8B8_UNORM, FORMAT_UNKNOWN, FORMAT_UNKNOWN, FORMAT_UNKNOWN, { FORMAT_G8R8_G8B8_UNORM, FORMAT_UNKNOWN } },
		// Block compressed. BC4 / BC5 / BC6H are single purpose and have no sRGB member
		{ FORMAT_BC1_TYPELESS, FORMAT_BC1_UNORM, FORMAT_BC1_UNORM_SRGB, FORMAT_UNKNOWN, FORMAT_UNKNOWN,
			{ FORMAT_BC1_TYPELESS, FORMAT_BC1_UNORM, FORMAT_BC1_UNORM_SRGB, FORMAT_UNKNOWN } },
		{ FORMAT_BC2_TYPELESS, FORMAT_BC2_UNORM, FORMAT_BC2_UNORM_SRGB, FORMAT_UNKNOWN, FORMAT_UNKNOWN,
			{ FORMAT_BC2_TYPELESS, FORMAT_BC2_UNORM, FORMAT_BC2_UNORM_SRGB, FORMAT_UNKNOWN } },
		{ FORMAT_BC3_TYPELESS, FORMAT_BC3_UNORM, FORMAT_BC3_UNORM_SRGB, FORMAT_UNKNOWN, FORMAT_UNKNOWN,
			{ FORMAT_BC3_TYPELESS, FORMAT_BC3_UNORM, FORMAT_BC3_UNORM_SRGB, FORMAT_UNKNOWN } },
		{ FORMAT_BC4_TYPELESS, FORMAT_BC4_UNORM, FORMAT_UNKNOWN, FORMAT_UNKNOWN, FORMAT_UNKNOWN,
			{ FORMAT_BC4_TYPELESS, FORMAT_BC4_UNORM, FORMAT_BC4_SNORM, FORMAT_UNKNOWN } },
		{ FORMAT_BC5_TYPELESS, FORMAT_BC5_UNORM, FORMAT_UNKNOWN, FORMAT_UNKNOWN, FORMAT_UNKNOWN,
			{ FORMAT_BC5_TYPELESS, FORMAT_BC5_UNORM, FORMAT_BC5_SNORM, FORMAT_UNKNOWN } },
		{ FORMAT_UNKNOWN, FORMAT_B5G6R5_UNORM, FORMAT_UNKNOWN, FORMAT_UNKNOWN, FORMAT_UNKNOWN, { FORMAT_B5G6R5_UNORM, FORMAT_UNKNOWN } },
		{ FORMAT_UNKNOWN, FORMAT_B5G5R5A1_UNORM, FORMAT_UNKNOWN, FORMAT_UNKNOWN, FORMAT_UNKNOWN, { FORMAT_B5G5R5A1_UNORM, FORMAT_UNKNOWN } },
		// 8 bit BGRA: what a swap chain buffer usually is
		{ FORMAT_B8G8R8A8_TYPELESS, FORMAT_B8G8R8A8_UNORM, FORMAT_B8G8R8A8_UNORM_SRGB, FORMAT_UNKNOWN, FORMAT_UNKNOWN,
			{ FORMAT_B8G8R8A8_TYPELESS, FORMAT_B8G8R8A8_UNORM, FORMAT_B8G8R8A8_UNORM_SRGB, FORMAT_UNKNOWN } },
		{ FORMAT_B8G8R8X8_TYPELESS, FORMAT_B8G8R8X8_UNORM, FORMAT_B8G8R8X8_UNORM_SRGB, FORMAT_UNKNOWN, FORMAT_UNKNOWN,
			{ FORMAT_B8G8R8X8_TYPELESS, FORMAT_B8G8R8X8_UNORM, FORMAT_B8G8R8X8_UNORM_SRGB, FORMAT_UNKNOWN } },
		{ FORMAT_UNKNOWN, FORMAT_R10G10B10_XR_BIAS_A2_UNORM, FORMAT_UNKNOWN, FORMAT_UNKNOWN, FORMAT_UNKNOWN, { FORMAT_R10G10B10_XR_BIAS_A2_UNORM, FORMAT_UNKNOWN } },
		// HDR block compression: linear by definition
		{ FORMAT_BC6H_TYPELESS, FORMAT_BC6H_UF16, FORMAT_UNKNOWN, FORMAT_UNKNOWN, FORMAT_UNKNOWN,
			{ FORMAT_BC6H_TYPELESS, FORMAT_BC6H_UF16, FORMAT_BC6H_SF16, FORMAT_UNKNOWN } },
		{ FORMAT_BC7_TYPELESS, FORMAT_BC7_UNORM, FORMAT_BC7_UNORM_SRGB, FORMAT_UNKNOWN, FORMAT_UNKNOWN,
			{ FORMAT_BC7_TYPELESS, FORMAT_BC7_UNORM, FORMAT_BC7_UNORM_SRGB, FORMAT_UNKNOWN } },
		// Video and palettised formats: named so a lookup can answer, not usable as a
		// render or shader resource by this engine
		{ FORMAT_UNKNOWN, FORMAT_AYUV, FORMAT_UNKNOWN, FORMAT_UNKNOWN, FORMAT_UNKNOWN, { FORMAT_AYUV, FORMAT_UNKNOWN } },
		{ FORMAT_UNKNOWN, FORMAT_Y410, FORMAT_UNKNOWN, FORMAT_UNKNOWN, FORMAT_UNKNOWN, { FORMAT_Y410, FORMAT_UNKNOWN } },
		{ FORMAT_UNKNOWN, FORMAT_Y416, FORMAT_UNKNOWN, FORMAT_UNKNOWN, FORMAT_UNKNOWN, { FORMAT_Y416, FORMAT_UNKNOWN } },
		{ FORMAT_UNKNOWN, FORMAT_NV12, FORMAT_UNKNOWN, FORMAT_UNKNOWN, FORMAT_UNKNOWN, { FORMAT_NV12, FORMAT_UNKNOWN } },
		{ FORMAT_UNKNOWN, FORMAT_P010, FORMAT_UNKNOWN, FORMAT_UNKNOWN, FORMAT_UNKNOWN, { FORMAT_P010, FORMAT_UNKNOWN } },
		{ FORMAT_UNKNOWN, FORMAT_P016, FORMAT_UNKNOWN, FORMAT_UNKNOWN, FORMAT_UNKNOWN, { FORMAT_P016, FORMAT_UNKNOWN } },
		{ FORMAT_UNKNOWN, FORMAT_420_OPAQUE, FORMAT_UNKNOWN, FORMAT_UNKNOWN, FORMAT_UNKNOWN, { FORMAT_420_OPAQUE, FORMAT_UNKNOWN } },
		{ FORMAT_UNKNOWN, FORMAT_YUY2, FORMAT_UNKNOWN, FORMAT_UNKNOWN, FORMAT_UNKNOWN, { FORMAT_YUY2, FORMAT_UNKNOWN } },
		{ FORMAT_UNKNOWN, FORMAT_Y210, FORMAT_UNKNOWN, FORMAT_UNKNOWN, FORMAT_UNKNOWN, { FORMAT_Y210, FORMAT_UNKNOWN } },
		{ FORMAT_UNKNOWN, FORMAT_Y216, FORMAT_UNKNOWN, FORMAT_UNKNOWN, FORMAT_UNKNOWN, { FORMAT_Y216, FORMAT_UNKNOWN } },
		{ FORMAT_UNKNOWN, FORMAT_NV11, FORMAT_UNKNOWN, FORMAT_UNKNOWN, FORMAT_UNKNOWN, { FORMAT_NV11, FORMAT_UNKNOWN } },
		{ FORMAT_UNKNOWN, FORMAT_AI44, FORMAT_UNKNOWN, FORMAT_UNKNOWN, FORMAT_UNKNOWN, { FORMAT_AI44, FORMAT_UNKNOWN } },
		{ FORMAT_UNKNOWN, FORMAT_IA44, FORMAT_UNKNOWN, FORMAT_UNKNOWN, FORMAT_UNKNOWN, { FORMAT_IA44, FORMAT_UNKNOWN } },
		{ FORMAT_UNKNOWN, FORMAT_P8, FORMAT_UNKNOWN, FORMAT_UNKNOWN, FORMAT_UNKNOWN, { FORMAT_P8, FORMAT_UNKNOWN } },
		{ FORMAT_UNKNOWN, FORMAT_A8P8, FORMAT_UNKNOWN, FORMAT_UNKNOWN, FORMAT_UNKNOWN, { FORMAT_A8P8, FORMAT_UNKNOWN } },
		{ FORMAT_UNKNOWN, FORMAT_B4G4R4A4_UNORM, FORMAT_UNKNOWN, FORMAT_UNKNOWN, FORMAT_UNKNOWN, { FORMAT_B4G4R4A4_UNORM, FORMAT_UNKNOWN } },
		{ FORMAT_UNKNOWN, FORMAT_P208, FORMAT_UNKNOWN, FORMAT_UNKNOWN, FORMAT_UNKNOWN, { FORMAT_P208, FORMAT_UNKNOWN } },
		{ FORMAT_UNKNOWN, FORMAT_V208, FORMAT_UNKNOWN, FORMAT_UNKNOWN, FORMAT_UNKNOWN, { FORMAT_V208, FORMAT_UNKNOWN } },
		{ FORMAT_UNKNOWN, FORMAT_V408, FORMAT_UNKNOWN, FORMAT_UNKNOWN, FORMAT_UNKNOWN, { FORMAT_V408, FORMAT_UNKNOWN } },
	};

	static constexpr UINT32 GFormatFamilyCount = static_cast<UINT32>(PE_ARRAYSIZE(GFormatFamilies));

	// The family a format belongs to, or null when the engine does not know the format.
	// Two formats are view compatible exactly when they answer with the same family.
	static const RFormatFamilyDesc* FindFormatFamily(RFormatType InFormat) noexcept
	{
		if (InFormat == RFormatType::FORMAT_UNKNOWN)
		{
			return nullptr;
		}
		for (UINT32 i = 0u; i < GFormatFamilyCount; i++)
		{
			for (UINT32 j = 0u; j < static_cast<UINT32>(PE_ARRAYSIZE(GFormatFamilies[i].Members)); j++)
			{
				const RFormatType Member = GFormatFamilies[i].Members[j];
				if (Member == RFormatType::FORMAT_UNKNOWN)
				{
					break;
				}
				if (Member == InFormat)
				{
					return (&(GFormatFamilies[i]));
				}
			}
		}
		return nullptr;
	}

	// RTextureDesc::Depth is overloaded: the volume depth of a 3D texture, or the bit
	// count of a depth texture. This recognises the depth reading, which is the one that
	// needs typed views derived from a bit count instead of from a format.
	BOOL32 RFormatTraits::IsDepthBitCountDesc(const RTextureDesc& InDesc) noexcept
	{
		if ((InDesc.BindFlags & static_cast<UINT8>(RBindFlagType::BIND_DEPTH_STENCIL)) == 0u)
		{
			return FALSE;
		}
		if (InDesc.BufferFormat != RFormatType::FORMAT_UNKNOWN)
		{
			return FALSE;
		}
		return ((InDesc.Depth == 16u) || (InDesc.Depth == 24u) || (InDesc.Depth == 32u)) ? TRUE : FALSE;
	}

	RFormatType RTextureFormatSet::GetViewFormat(ERHIViewFormatType InView) const noexcept
	{
		switch (InView)
		{
		case ERHIViewFormatType::RHI_VIEW_FORMAT_SRV:
			return SRV;
		case ERHIViewFormatType::RHI_VIEW_FORMAT_RTV:
			return RTV;
		case ERHIViewFormatType::RHI_VIEW_FORMAT_DSV:
			return DSV;
		case ERHIViewFormatType::RHI_VIEW_FORMAT_UAV:
			return UAV;
		default:
			break;
		}
		return RFormatType::FORMAT_UNKNOWN;
	}

	const CHAR* RFormatTraits::GetFormatName(RFormatType InFormat) noexcept
	{
		struct RFormatNameEntry
		{
			RFormatType		Format;
			const CHAR*		Name;
		};
		static const RFormatNameEntry FormatNames[] =
		{
#define PE_RFORMAT_NAME_ENTRY(_NAME) { RFormatType::FORMAT_##_NAME, PE_TEXT("FORMAT_" #_NAME) },
			PE_RFORMAT_TYPE_LIST(PE_RFORMAT_NAME_ENTRY)
#undef PE_RFORMAT_NAME_ENTRY
		};
		for (UINT32 i = 0u; i < static_cast<UINT32>(PE_ARRAYSIZE(FormatNames)); i++)
		{
			if (FormatNames[i].Format == InFormat)
			{
				return FormatNames[i].Name;
			}
		}
		return PE_TEXT("FORMAT_UNSUPPORTED");
	}

	BOOL32 RFormatTraits::IsKnownFormat(RFormatType InFormat) noexcept
	{
		return (FindFormatFamily(InFormat) != nullptr) ? TRUE : FALSE;
	}

	BOOL32 RFormatTraits::IsSRGBFormat(RFormatType InFormat) noexcept
	{
		const RFormatFamilyDesc* Family = FindFormatFamily(InFormat);
		return ((Family != nullptr) && (Family->SRGB == InFormat)) ? TRUE : FALSE;
	}

	BOOL32 RFormatTraits::IsTypelessFormat(RFormatType InFormat) noexcept
	{
		const RFormatFamilyDesc* Family = FindFormatFamily(InFormat);
		return ((Family != nullptr) && (Family->Typeless == InFormat)) ? TRUE : FALSE;
	}

	BOOL32 RFormatTraits::IsDepthStencilFormat(RFormatType InFormat) noexcept
	{
		const RFormatFamilyDesc* Family = FindFormatFamily(InFormat);
		return ((Family != nullptr) && (Family->DepthDSV == InFormat)) ? TRUE : FALSE;
	}

	BOOL32 RFormatTraits::HasSRGBVariant(RFormatType InFormat) noexcept
	{
		const RFormatFamilyDesc* Family = FindFormatFamily(InFormat);
		return ((Family != nullptr) && (Family->SRGB != RFormatType::FORMAT_UNKNOWN)) ? TRUE : FALSE;
	}

	RFormatType RFormatTraits::GetLinearCounterpart(RFormatType InFormat) noexcept
	{
		const RFormatFamilyDesc* Family = FindFormatFamily(InFormat);
		if (Family == nullptr)
		{
			return InFormat;
		}
		// An sRGB member reads as its family's plain member; anything else is already the
		// linear reading of its own bits.
		return (Family->SRGB == InFormat) ? Family->Linear : InFormat;
	}

	RFormatType RFormatTraits::GetSRGBCounterpart(RFormatType InFormat) noexcept
	{
		const RFormatType LinearFormat = GetLinearCounterpart(InFormat);
		const RFormatFamilyDesc* Family = FindFormatFamily(LinearFormat);
		if ((Family == nullptr) || (Family->SRGB == RFormatType::FORMAT_UNKNOWN))
		{
			return RFormatType::FORMAT_UNKNOWN;
		}
		return Family->SRGB;
	}

	RFormatType RFormatTraits::GetTypelessCounterpart(RFormatType InFormat) noexcept
	{
		const RFormatType LinearFormat = GetLinearCounterpart(InFormat);
		const RFormatFamilyDesc* Family = FindFormatFamily(LinearFormat);
		if (Family == nullptr)
		{
			return RFormatType::FORMAT_UNKNOWN;
		}
		return Family->Typeless;
	}

	RFormatType RFormatTraits::GetFormatForColorSpace(RFormatType InFormat, BOOL32 InIsSRGB) noexcept
	{
		if (InIsSRGB == FALSE)
		{
			return GetLinearCounterpart(InFormat);
		}
		return GetSRGBCounterpart(InFormat);
	}

	BOOL32 RFormatTraits::AreFormatsViewCompatible(RFormatType InResourceFormat, RFormatType InViewFormat) noexcept
	{
		const RFormatFamilyDesc* ResourceFamily = FindFormatFamily(InResourceFormat);
		if (ResourceFamily == nullptr)
		{
			return FALSE;
		}
		if (InResourceFormat == InViewFormat)
		{
			return TRUE;
		}
		const RFormatFamilyDesc* ViewFamily = FindFormatFamily(InViewFormat);
		return ((ViewFamily != nullptr) && (ViewFamily == ResourceFamily)) ? TRUE : FALSE;
	}

	BOOL32 RFormatTraits::ResolveDepthStencilFormats(UINT32 InDepthBits, BOOL32 InHasStencil
		, RFormatType& OutResourceFormat, RFormatType& OutDSVFormat, RFormatType& OutSRVFormat) noexcept
	{
		OutResourceFormat	= RFormatType::FORMAT_UNKNOWN;
		OutDSVFormat		= RFormatType::FORMAT_UNKNOWN;
		OutSRVFormat		= RFormatType::FORMAT_UNKNOWN;

		// The bit count names the depth format; the stencil field only exists in the 24 and
		// 32 bit ones, and 32 bit has a family of its own for each reading.
		if ((InDepthBits == 16u) && (InHasStencil != FALSE))
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR),
				(PE_TEXT("RFormatTraits: a 16 bit depth texture cannot carry a stencil.")));
			return FALSE;
		}
		RFormatType DepthFormat = RFormatType::FORMAT_UNKNOWN;
		switch (InDepthBits)
		{
		case 16u:
			DepthFormat = RFormatType::FORMAT_D16_UNORM;
			break;
		case 24u:
			DepthFormat = RFormatType::FORMAT_D24_UNORM_S8_UINT;
			break;
		case 32u:
			DepthFormat = (InHasStencil != FALSE) ? RFormatType::FORMAT_D32_FLOAT_S8X24_UINT : RFormatType::FORMAT_D32_FLOAT;
			break;
		default:
			break;
		}
		if (DepthFormat == RFormatType::FORMAT_UNKNOWN)
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR),
				(PE_TEXT("RFormatTraits: depth textures take 16, 24 or 32 bits; any other value has no depth format.")));
			return FALSE;
		}

		const RFormatFamilyDesc* Family = FindFormatFamily(DepthFormat);
		if (Family == nullptr)
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR),
				(PE_TEXT("RFormatTraits: the depth format family is missing from the format table.")));
			return FALSE;
		}

		OutResourceFormat	= Family->Typeless;
		OutDSVFormat		= Family->DepthDSV;
		OutSRVFormat		= Family->DepthSRV;
		return TRUE;
	}

	BOOL32 RFormatTraits::ResolveTextureFormats(const RTextureDesc& InDesc, RTextureFormatSet& OutSet) noexcept
	{
		const UINT8 BindFlags = InDesc.BindFlags;
		const BOOL8 bBindSRV = ((BindFlags & static_cast<UINT8>(RBindFlagType::BIND_SHADER_RESOURCE)) != 0u) ? TRUE : FALSE;
		const BOOL8 bBindRTV = ((BindFlags & static_cast<UINT8>(RBindFlagType::BIND_RENDER_TARGET)) != 0u) ? TRUE : FALSE;
		const BOOL8 bBindDSV = ((BindFlags & static_cast<UINT8>(RBindFlagType::BIND_DEPTH_STENCIL)) != 0u) ? TRUE : FALSE;
		const BOOL8 bBindUAV = ((BindFlags & static_cast<UINT8>(RBindFlagType::BIND_UNORDERED_ACCESS)) != 0u) ? TRUE : FALSE;

		OutSet = RTextureFormatSet();

		// Depth textures state their bit count in Depth and leave the format unknown. The
		// same table that describes every other format turns that into the typeless storage
		// format plus the typed views, so no back-end carries its own bit count switch.
		if (IsDepthBitCountDesc(InDesc))
		{
			if (ResolveDepthStencilFormats(InDesc.Depth, InDesc.HasStencil,
				OutSet.Resource, OutSet.DSV, OutSet.SRV) == FALSE)
			{
				return FALSE;
			}
			// A view exists only when its bind flag asked for one; the depth SRV format the
			// table names is what a requested SRV gets.
			if (bBindSRV == FALSE)
			{
				OutSet.SRV = RFormatType::FORMAT_UNKNOWN;
			}
			return TRUE;
		}

		OutSet.SRV = bBindSRV ? InDesc.SRVFormat : RFormatType::FORMAT_UNKNOWN;
		OutSet.RTV = bBindRTV ? InDesc.RTVFormat : RFormatType::FORMAT_UNKNOWN;
		OutSet.UAV = bBindUAV ? InDesc.UAVFormat : RFormatType::FORMAT_UNKNOWN;

		if (IsKnownFormat(InDesc.BufferFormat) == FALSE)
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR),
				(PE_TEXT("RFormatTraits: the texture's content format is not one the engine knows.")));
			return FALSE;
		}

		// An sRGB unordered access view does not exist on any D3D version, so an sRGB UAV
		// request is answered with the linear member of the same family instead of refused:
		// writing through that view is well defined - it stores exactly the values written -
		// while the read side of an sRGB texture is the sRGB SRV, which does decode.
		if (IsSRGBFormat(OutSet.UAV) != FALSE)
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR),
				(PE_TEXT("RFormatTraits: an sRGB format cannot be an unordered access view; the UAV is created linear.")));
			OutSet.UAV = GetLinearCounterpart(OutSet.UAV);
		}

		const RFormatType ContentLinear = GetLinearCounterpart(InDesc.BufferFormat);

		// A texture bound as a depth target needs a depth view format, and the readable side
		// of a depth resource is the family's depth SRV format, never the depth format itself
		// (D24_UNORM_S8_UINT has no SRV; R24_UNORM_X8_TYPELESS is its readable form). Only a
		// default is replaced here: an explicitly requested SRV format is checked below like
		// any other.
		if (bBindDSV != FALSE)
		{
			const RFormatFamilyDesc* DepthFamily = FindFormatFamily(ContentLinear);
			if ((DepthFamily == nullptr) || (DepthFamily->DepthDSV == RFormatType::FORMAT_UNKNOWN))
			{
				PE_FAILED((ENGINE_RENDER_CORE_ERROR),
					(PE_TEXT("RFormatTraits: this content format has no depth format, so it cannot be bound as a depth target.")));
				return FALSE;
			}
			OutSet.DSV = DepthFamily->DepthDSV;
			if (IsTypelessFormat(OutSet.SRV) != FALSE)
			{
				OutSet.SRV = DepthFamily->DepthSRV;
			}
		}

		// Every requested view has to be a real, typed format of the content's family. A
		// typeless format is storage only - the API refuses a view created with one - and a
		// format from another family would read the bits as something they are not.
		for (UINT32 i = 0u; i < static_cast<UINT32>(ERHIViewFormatType::RHI_VIEW_FORMAT_COUNT); i++)
		{
			const RFormatType ViewFormat = OutSet.GetViewFormat(static_cast<ERHIViewFormatType>(i));
			if (ViewFormat == RFormatType::FORMAT_UNKNOWN)
			{
				continue;
			}
			if ((IsKnownFormat(ViewFormat) == FALSE) || (IsTypelessFormat(ViewFormat) != FALSE)
				|| (AreFormatsViewCompatible(ContentLinear, ViewFormat) == FALSE))
			{
				PE_FAILED((ENGINE_RENDER_CORE_ERROR),
					(PE_TEXT("RFormatTraits: a view format is not usable with this texture's content format. A view needs a typed format from the content format's family; typeless and unknown formats cannot be viewed.")));
				return FALSE;
			}
		}

		// Storage: never the sRGB member, and typeless as soon as one view reads the bits
		// differently from another. Both come from the content format's family.
		OutSet.Resource = ContentLinear;
		for (UINT32 i = 0u; i < static_cast<UINT32>(ERHIViewFormatType::RHI_VIEW_FORMAT_COUNT); i++)
		{
			const RFormatType ViewFormat = OutSet.GetViewFormat(static_cast<ERHIViewFormatType>(i));
			if ((ViewFormat == RFormatType::FORMAT_UNKNOWN) || (ViewFormat == OutSet.Resource))
			{
				continue;
			}
			const RFormatType TypelessResource = GetTypelessCounterpart(OutSet.Resource);
			if (TypelessResource == RFormatType::FORMAT_UNKNOWN)
			{
				PE_FAILED((ENGINE_RENDER_CORE_ERROR),
					(PE_TEXT("RFormatTraits: this format family has no typeless member, so a resource of it cannot carry views with different formats.")));
				return FALSE;
			}
			OutSet.Resource = TypelessResource;
			break;
		}

		return TRUE;
	}

}	// PigeonEngine
