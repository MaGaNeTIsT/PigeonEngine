#pragma once

#include <CoreMinimal.h>
#include <RHI/RHITypes.h>
#include <RenderCommon.h>

namespace PigeonEngine
{

	// -------------------------------------------------------------------------
	// RFormatTraits - everything the engine knows about RFormatType.
	//
	// Three rules live here, and they are the reason a texture's sRGB content can
	// survive from an asset to a shader without any call site growing a special case:
	//
	//   1. Storage and view are separate. A resource is never created with an sRGB
	//      format: sRGB is how a VIEW reads or writes the data. The linear-to-sRGB
	//      decode therefore happens in the sampler and the encode in the render target
	//      unit, not in a shader.
	//   2. A view format that differs from the content format needs a typeless
	//      resource. D3D11 only allows a differently formatted view over a resource
	//      created as TYPELESS, and D3D12 is merely wider; creating the resource
	//      typeless whenever views diverge is the intersection of both APIs, and it is
	//      what lets one texture be sampled as sRGB in one place and as data in
	//      another.
	//   3. The format family table below is the single source of that knowledge.
	//      linear<->sRGB<->typeless counterparts, view compatibility and the depth
	//      bit-count mapping are all read off it. Adding a format means adding it to a
	//      family row; no back-end and no call site has to learn about it.
	//
	// Both back-ends resolve their formats through ResolveTextureFormats, so a texture
	// descriptor means the same thing on D3D11 and D3D12 by construction rather than
	// by two implementations agreeing.
	// -------------------------------------------------------------------------

	// -------------------------------------------------------------------------
	// The engine's format name list. One entry per RFormatType member, in enum order,
	// spelled exactly like the member and like its DXGI counterpart.
	//
	// Two things are built from this list, so a format is spelled in one place:
	//   - the names RFormatTraits::GetFormatName answers with
	//   - the back-end translation table (RenderDevice/DXGIFormatTranslate.h)
	//
	// A member missing here is not silently mapped to nothing: the translation reports
	// it as unsupported, which is what makes an omission show up the first time the
	// format is used. The enum's own comment states the same rule.
	// -------------------------------------------------------------------------
#define PE_RFORMAT_TYPE_LIST(_ENTRY) \
		_ENTRY(UNKNOWN) \
		_ENTRY(R32G32B32A32_TYPELESS) \
		_ENTRY(R32G32B32A32_FLOAT) \
		_ENTRY(R32G32B32A32_UINT) \
		_ENTRY(R32G32B32A32_SINT) \
		_ENTRY(R32G32B32_TYPELESS) \
		_ENTRY(R32G32B32_FLOAT) \
		_ENTRY(R32G32B32_UINT) \
		_ENTRY(R32G32B32_SINT) \
		_ENTRY(R16G16B16A16_TYPELESS) \
		_ENTRY(R16G16B16A16_FLOAT) \
		_ENTRY(R16G16B16A16_UNORM) \
		_ENTRY(R16G16B16A16_UINT) \
		_ENTRY(R16G16B16A16_SNORM) \
		_ENTRY(R16G16B16A16_SINT) \
		_ENTRY(R32G32_TYPELESS) \
		_ENTRY(R32G32_FLOAT) \
		_ENTRY(R32G32_UINT) \
		_ENTRY(R32G32_SINT) \
		_ENTRY(R32G8X24_TYPELESS) \
		_ENTRY(D32_FLOAT_S8X24_UINT) \
		_ENTRY(R32_FLOAT_X8X24_TYPELESS) \
		_ENTRY(X32_TYPELESS_G8X24_UINT) \
		_ENTRY(R10G10B10A2_TYPELESS) \
		_ENTRY(R10G10B10A2_UNORM) \
		_ENTRY(R10G10B10A2_UINT) \
		_ENTRY(R11G11B10_FLOAT) \
		_ENTRY(R8G8B8A8_TYPELESS) \
		_ENTRY(R8G8B8A8_UNORM) \
		_ENTRY(R8G8B8A8_UNORM_SRGB) \
		_ENTRY(R8G8B8A8_UINT) \
		_ENTRY(R8G8B8A8_SNORM) \
		_ENTRY(R8G8B8A8_SINT) \
		_ENTRY(R16G16_TYPELESS) \
		_ENTRY(R16G16_FLOAT) \
		_ENTRY(R16G16_UNORM) \
		_ENTRY(R16G16_UINT) \
		_ENTRY(R16G16_SNORM) \
		_ENTRY(R16G16_SINT) \
		_ENTRY(R32_TYPELESS) \
		_ENTRY(D32_FLOAT) \
		_ENTRY(R32_FLOAT) \
		_ENTRY(R32_UINT) \
		_ENTRY(R32_SINT) \
		_ENTRY(R24G8_TYPELESS) \
		_ENTRY(D24_UNORM_S8_UINT) \
		_ENTRY(R24_UNORM_X8_TYPELESS) \
		_ENTRY(X24_TYPELESS_G8_UINT) \
		_ENTRY(R8G8_TYPELESS) \
		_ENTRY(R8G8_UNORM) \
		_ENTRY(R8G8_UINT) \
		_ENTRY(R8G8_SNORM) \
		_ENTRY(R8G8_SINT) \
		_ENTRY(R16_TYPELESS) \
		_ENTRY(R16_FLOAT) \
		_ENTRY(D16_UNORM) \
		_ENTRY(R16_UNORM) \
		_ENTRY(R16_UINT) \
		_ENTRY(R16_SNORM) \
		_ENTRY(R16_SINT) \
		_ENTRY(R8_TYPELESS) \
		_ENTRY(R8_UNORM) \
		_ENTRY(R8_UINT) \
		_ENTRY(R8_SNORM) \
		_ENTRY(R8_SINT) \
		_ENTRY(A8_UNORM) \
		_ENTRY(R1_UNORM) \
		_ENTRY(R9G9B9E5_SHAREDEXP) \
		_ENTRY(R8G8_B8G8_UNORM) \
		_ENTRY(G8R8_G8B8_UNORM) \
		_ENTRY(BC1_TYPELESS) \
		_ENTRY(BC1_UNORM) \
		_ENTRY(BC1_UNORM_SRGB) \
		_ENTRY(BC2_TYPELESS) \
		_ENTRY(BC2_UNORM) \
		_ENTRY(BC2_UNORM_SRGB) \
		_ENTRY(BC3_TYPELESS) \
		_ENTRY(BC3_UNORM) \
		_ENTRY(BC3_UNORM_SRGB) \
		_ENTRY(BC4_TYPELESS) \
		_ENTRY(BC4_UNORM) \
		_ENTRY(BC4_SNORM) \
		_ENTRY(BC5_TYPELESS) \
		_ENTRY(BC5_UNORM) \
		_ENTRY(BC5_SNORM) \
		_ENTRY(B5G6R5_UNORM) \
		_ENTRY(B5G5R5A1_UNORM) \
		_ENTRY(B8G8R8A8_UNORM) \
		_ENTRY(B8G8R8X8_UNORM) \
		_ENTRY(R10G10B10_XR_BIAS_A2_UNORM) \
		_ENTRY(B8G8R8A8_TYPELESS) \
		_ENTRY(B8G8R8A8_UNORM_SRGB) \
		_ENTRY(B8G8R8X8_TYPELESS) \
		_ENTRY(B8G8R8X8_UNORM_SRGB) \
		_ENTRY(BC6H_TYPELESS) \
		_ENTRY(BC6H_UF16) \
		_ENTRY(BC6H_SF16) \
		_ENTRY(BC7_TYPELESS) \
		_ENTRY(BC7_UNORM) \
		_ENTRY(BC7_UNORM_SRGB) \
		_ENTRY(AYUV) \
		_ENTRY(Y410) \
		_ENTRY(Y416) \
		_ENTRY(NV12) \
		_ENTRY(P010) \
		_ENTRY(P016) \
		_ENTRY(420_OPAQUE) \
		_ENTRY(YUY2) \
		_ENTRY(Y210) \
		_ENTRY(Y216) \
		_ENTRY(NV11) \
		_ENTRY(AI44) \
		_ENTRY(IA44) \
		_ENTRY(P8) \
		_ENTRY(A8P8) \
		_ENTRY(B4G4R4A4_UNORM) \
		_ENTRY(P208) \
		_ENTRY(V208) \
		_ENTRY(V408)

	// Which view of a texture is being asked about: see ERHIViewFormatType in RHITypes.h.

	// The formats one texture resource and its views are created with, as resolved from
	// an RTextureDesc. FORMAT_UNKNOWN means "this resource has no such view".
	struct RTextureFormatSet
	{
		RFormatType		Resource	= RFormatType::FORMAT_UNKNOWN;
		RFormatType		SRV			= RFormatType::FORMAT_UNKNOWN;
		RFormatType		RTV			= RFormatType::FORMAT_UNKNOWN;
		RFormatType		DSV			= RFormatType::FORMAT_UNKNOWN;
		RFormatType		UAV			= RFormatType::FORMAT_UNKNOWN;

		PE_NODISCARD RFormatType GetViewFormat(ERHIViewFormatType InView) const noexcept;
	};

	class RFormatTraits
	{
	public:
		// The format names the engine logs with, e.g. "FORMAT_R8G8B8A8_UNORM_SRGB".
		// A format missing from PE_RFORMAT_TYPE_LIST answers "FORMAT_UNSUPPORTED".
		PE_NODISCARD static const CHAR*	GetFormatName(RFormatType InFormat) noexcept;

		// A real format the engine and the back-ends know how to create. FORMAT_UNKNOWN is
		// not one, which is what the query is for.
		PE_NODISCARD static BOOL32		IsKnownFormat(RFormatType InFormat) noexcept;
		// The format decodes as sRGB when read through a view.
		PE_NODISCARD static BOOL32		IsSRGBFormat(RFormatType InFormat) noexcept;
		// Storage only: a view must never be created with one of these.
		PE_NODISCARD static BOOL32		IsTypelessFormat(RFormatType InFormat) noexcept;
		// A depth-stencil view format (D16_UNORM / D24_UNORM_S8_UINT / D32_FLOAT[_S8X24_UINT]).
		// The plain colour members of a depth format's family (R32_FLOAT, R16_UNORM) answer
		// FALSE: they are the shader-visible side of a depth resource, not the depth side.
		PE_NODISCARD static BOOL32		IsDepthStencilFormat(RFormatType InFormat) noexcept;
		// Whether the family has an sRGB member at all. 16 and 32 bit formats do not, which is
		// what makes an "import as sRGB" request on such a texture a no-op instead of an error.
		PE_NODISCARD static BOOL32		HasSRGBVariant(RFormatType InFormat) noexcept;

		// Counterparts within a format's family. The linear counterpart of a linear format is
		// the format itself; the other two answer FORMAT_UNKNOWN when the family has no such
		// member (float families have no sRGB member, R11G11B10 has no typeless one).
		PE_NODISCARD static RFormatType	GetLinearCounterpart(RFormatType InFormat) noexcept;
		PE_NODISCARD static RFormatType	GetSRGBCounterpart(RFormatType InFormat) noexcept;
		PE_NODISCARD static RFormatType	GetTypelessCounterpart(RFormatType InFormat) noexcept;

		// The format to read a resource of InFormat as, for the requested colour space.
		// Asking for sRGB on a family without an sRGB member answers FORMAT_UNKNOWN so the
		// caller can report it rather than silently reading the data as something else.
		PE_NODISCARD static RFormatType	GetFormatForColorSpace(RFormatType InFormat, BOOL32 InIsSRGB) noexcept;

		// Whether a view created with InViewFormat may read a resource created with
		// InResourceFormat: same format, or another member of the same family. A typeless
		// resource is viewable by every typed member of its family, which is the whole point
		// of a typeless resource.
		PE_NODISCARD static BOOL32		AreFormatsViewCompatible(RFormatType InResourceFormat, RFormatType InViewFormat) noexcept;

		// Depth textures carry their bit count in RTextureDesc::Depth and leave BufferFormat
		// unknown. This recognises that convention, which a creator needs in order to tell a
		// bit count apart from a 3D texture's volume depth.
		PE_NODISCARD static BOOL32		IsDepthBitCountDesc(const RTextureDesc& InDesc) noexcept;

		// The same convention seen from the format side: the bit count maps onto the typeless
		// resource format and the typed views that go with it. Answers FALSE for a bit count
		// that has no depth format.
		static BOOL32					ResolveDepthStencilFormats(UINT32 InDepthBits, BOOL32 InHasStencil
											, RFormatType& OutResourceFormat, RFormatType& OutDSVFormat, RFormatType& OutSRVFormat) noexcept;

		// Turns a texture descriptor into the formats the resource and its views are created
		// with: the resolved storage format, plus one format per bound view. Answers FALSE when
		// the desc asks for a combination no API can express (an sRGB view over a format with no
		// sRGB member, a typeless view format, a depth target whose format has no depth format).
		//
		// Both back-ends call this, so a desc means the same thing on either one.
		static BOOL32					ResolveTextureFormats(const RTextureDesc& InDesc, RTextureFormatSet& OutSet) noexcept;
	};

}	// PigeonEngine
