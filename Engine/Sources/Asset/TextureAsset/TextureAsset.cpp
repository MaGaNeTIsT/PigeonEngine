#include "TextureAsset.h"
#include <Asset/AssetLoadManager.h>
#include <RenderDevice/DeviceD3D11.h>
#include <RHI/IRRHIDevice.h>
#include <Main/MainManager.h>
#if _EDITOR_ONLY
#include <Editor/EditorLogManager.h>
#endif

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<ETexture, EObjectBase, EResourceInterface>();

		RegisterClassType<ETexture2D, ETexture>();
		RegisterClassType<ETextureCube, ETexture>();

		RegisterBaseAssetClassTypes<ETexture2D>();
		RegisterClassType<ETexture2DAsset, TBaseAsset<ETexture2D>>();
		RegisterBaseAssetClassTypes<ETextureCube>();
		RegisterClassType<ETextureCubeAsset, TBaseAsset<ETextureCube>>();

		RegisterClassType<ETextureAssetManager, EManagerBase>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

#if _EDITOR_ONLY
	static UINT32 ReadUInt32BigEndian(const BYTE* InBytes) noexcept
	{
		return ((static_cast<UINT32>(InBytes[0]) << 24u) | (static_cast<UINT32>(InBytes[1]) << 16u)
			| (static_cast<UINT32>(InBytes[2]) << 8u) | static_cast<UINT32>(InBytes[3]));
	}

	// Whether a piece of text contains a name, in whichever encoding the text uses.
	//
	// An ICC description is written in one of three shapes depending on who wrote the profile:
	// ASCII (a v2 'desc' tag), UTF-16 big endian (a v4 'mluc' tag, which is what GIMP and the
	// ICC preference profiles write) or UTF-16 little endian (produced by a few writers). A
	// check that only knows ASCII therefore fails on exactly the profiles that are most likely
	// to be sRGB - the v4 ones - which is what this function exists to avoid.
	static BOOL32 DescriptionContainsName(const BYTE* InData, UINT32 InDataSize, const CHAR* InName, UINT32 InNameLength) noexcept
	{
		if ((!InData) || (!InName) || (InNameLength == 0u) || (InDataSize < InNameLength))
		{
			return FALSE;
		}
		for (UINT32 Offset = 0u; ((Offset + InNameLength) <= InDataSize); Offset++)
		{
			BOOL32 bAscii = TRUE;
			// The two byte encodings need a byte per character and one zero byte next to it.
			BOOL32 bUtf16BigEndian = (((Offset + (InNameLength * 2u)) <= InDataSize) ? TRUE : FALSE);
			BOOL32 bUtf16LittleEndian = bUtf16BigEndian;
			for (UINT32 i = 0u; i < InNameLength; i++)
			{
				const BYTE Expected = static_cast<BYTE>(InName[i]);
				if (InData[Offset + i] != Expected)
				{
					bAscii = FALSE;
				}
				if (bUtf16BigEndian != FALSE)
				{
					if ((InData[Offset + (i * 2u)] != 0u) || (InData[Offset + (i * 2u) + 1u] != Expected))
					{
						bUtf16BigEndian = FALSE;
					}
					if ((InData[Offset + (i * 2u)] != Expected) || (InData[Offset + (i * 2u) + 1u] != 0u))
					{
						bUtf16LittleEndian = FALSE;
					}
				}
				if ((bAscii == FALSE) && (bUtf16BigEndian == FALSE) && (bUtf16LittleEndian == FALSE))
				{
					break;
				}
			}
			if ((bAscii != FALSE) || (bUtf16BigEndian != FALSE) || (bUtf16LittleEndian != FALSE))
			{
				return TRUE;
			}
		}
		return FALSE;
	}

	// Whether an ICC profile names itself as sRGB.
	//
	// The engine cannot convert between colour spaces, so the only useful answer a profile gives
	// is whether it is the sRGB one. The profile's tag table is walked to its 'desc' (or 'mluc')
	// entry, whose payload carries the profile name, and the name is looked for in every
	// encoding a description can use. Two names identify sRGB: its own ("sRGB", as in "sRGB
	// IEC61966-2.1", "GIMP built-in sRGB", "sRGB v4 ICC preference profile") and the IEC
	// standard it is defined by ("61966-2.1"), which covers profiles named without the colour
	// space's name. A profile that matches neither is reported by the caller as "declared, but
	// not sRGB".
	static BOOL32 ICCProfileDescribesSRGB(const BYTE* InProfile, UINT32 InProfileSize) noexcept
	{
		// ICC header is 128 bytes, then the tag count, then 12 bytes per entry:
		// signature, data offset, data size - all four byte big endian.
		if ((!InProfile) || (InProfileSize < 132u))
		{
			return FALSE;
		}
		static const CHAR* const SRGBNames[] = { "sRGB", "61966-2.1" };
		static const UINT32 SRGBNameLengths[] = { 4u, 9u };
		constexpr UINT32 SRGBNameCount = static_cast<UINT32>(PE_ARRAYSIZE(SRGBNames));

		const UINT32 TagCount = ReadUInt32BigEndian(InProfile + 128u);
		for (UINT32 i = 0u; i < TagCount; i++)
		{
			const UINT32 EntryOffset = 132u + (i * 12u);
			if ((EntryOffset + 12u) > InProfileSize)
			{
				break;
			}
			const BYTE* Entry = InProfile + EntryOffset;
			const UINT32 Signature = ReadUInt32BigEndian(Entry);
			if ((Signature != 0x64657363u) && (Signature != 0x6d6c7563u))	// 'desc' / 'mluc'
			{
				continue;
			}
			const UINT32 DataOffset = ReadUInt32BigEndian(Entry + 4u);
			const UINT32 DataSize = ReadUInt32BigEndian(Entry + 8u);
			if ((DataOffset >= InProfileSize) || ((DataOffset + DataSize) > InProfileSize))
			{
				continue;
			}
			const BYTE* Data = InProfile + DataOffset;
			for (UINT32 NameIndex = 0u; NameIndex < SRGBNameCount; NameIndex++)
			{
				if (DescriptionContainsName(Data, DataSize, SRGBNames[NameIndex], SRGBNameLengths[NameIndex]) != FALSE)
				{
					return TRUE;
				}
			}
		}
		return FALSE;
	}

	// Whether a PNG file declares the sRGB colour space in its own chunk.
	//
	// WIC hands an embedded ICC profile to the caller as a colour context, but the PNG 'sRGB'
	// chunk - which is what many exporters write instead of a profile - does not reach that API,
	// so it is read from the file here. The answer says whether the file declared anything at all:
	// a PNG with neither an sRGB chunk nor a profile declares nothing and leaves the decision to
	// the importer's fallback. gAMA is deliberately not consulted - "gamma 2.2" is an approximation
	// of sRGB rather than a statement that the data is sRGB, and treating it as one would let a
	// guess outrank the fallback.
	static BOOL32 DetectPNGColorSpaceDeclaration(const EString& InPathName, BOOL32& OutDeclaredSRGB) noexcept
	{
		OutDeclaredSRGB = FALSE;
		void* FileMem = nullptr;
		ULONG FileSize = 0u;
		if (EFileHelper::ReadFileAsBinary(InPathName, FileMem, FileSize) == FALSE)
		{
			return FALSE;
		}
		const BYTE* Bytes = static_cast<const BYTE*>(FileMem);

		BOOL32 bDeclared = FALSE;
		// PNG signature: 137 'P' 'N' 'G' 13 10 26 10.
		BOOL32 bIsPNG = ((FileSize >= 8u) ? TRUE : FALSE);
		if ((bIsPNG != FALSE)
			&& ((Bytes[0] != 0x89u) || (Bytes[1] != 'P') || (Bytes[2] != 'N') || (Bytes[3] != 'G')
				|| (Bytes[4] != 0x0du) || (Bytes[5] != 0x0au) || (Bytes[6] != 0x1au) || (Bytes[7] != 0x0au)))
		{
			bIsPNG = FALSE;
		}
		if (bIsPNG != FALSE)
		{
			// A chunk is a four byte big endian length, a four byte type, that many bytes of
			// payload and a four byte CRC. Only the types matter here.
			UINT32 Offset = 8u;
			while ((Offset + 12u) <= FileSize)
			{
				const UINT32 ChunkLength = ReadUInt32BigEndian(Bytes + Offset);
				const BYTE* ChunkType = Bytes + Offset + 4u;
				if ((ChunkType[0] == 's') && (ChunkType[1] == 'R') && (ChunkType[2] == 'G') && (ChunkType[3] == 'B'))
				{
					bDeclared = TRUE;
					OutDeclaredSRGB = TRUE;
					break;
				}
				if ((ChunkType[0] == 'I') && (ChunkType[1] == 'E') && (ChunkType[2] == 'N') && (ChunkType[3] == 'D'))
				{
					break;
				}
				// A length that does not fit the file means the walk cannot continue; a file
				// that is not a PNG at all would have failed the signature check above.
				if ((ChunkLength > FileSize) || ((Offset + 12u + ChunkLength) > FileSize))
				{
					break;
				}
				Offset += 12u + ChunkLength;
			}
		}

		delete[] static_cast<BYTE*>(FileMem);
		return bDeclared;
	}

	// What the image itself says about its colour space.
	//
	// The file is the authority when it declares one: an sRGB ICC profile or an Exif colour space
	// of sRGB makes the texture sRGB, and a profile that is not sRGB is reported (the engine cannot
	// convert between colour spaces) and leaves the texture linear.
	//
	// A file that declares nothing is the case the import's fallback parameter exists for: TGA and
	// other formats carry no colour information at all, and no decoder can tell an 8 bit normal map
	// from an 8 bit albedo. OutDeclared tells the caller which of the two happened, so it can fall
	// back to the importer's choice only when the file really said nothing.
	static BOOL32 DetectImageColorSpace(IWICBitmapFrameDecode* InFrameDecode, BOOL32& OutDeclaredSRGB) noexcept
	{
		OutDeclaredSRGB = FALSE;
		if (!InFrameDecode)
		{
			return FALSE;
		}

		UINT32 ColorContextCount = 0u;
		if ((FAILED(InFrameDecode->GetColorContexts(0u, nullptr, &ColorContextCount))) || (ColorContextCount == 0u))
		{
			return FALSE;
		}

		TArray<IWICColorContext*, UINT32> ColorContexts;
		ColorContexts.SetNum(ColorContextCount);
		for (UINT32 i = 0u; i < ColorContextCount; i++)
		{
			ColorContexts[i] = nullptr;
		}

		// A file can carry more than one context (an Exif colour space and a profile, say); the
		// answer is sRGB when any of them says so. Having any context at all counts as a
		// declaration, even when the answer is "not sRGB".
		BOOL32 bDeclared = FALSE;
		UINT32 ActualCount = 0u;
		if (SUCCEEDED(InFrameDecode->GetColorContexts(ColorContextCount, ColorContexts.GetData(), &ActualCount)))
		{
			for (UINT32 i = 0u; i < ActualCount; i++)
			{
				IWICColorContext* ColorContext = ColorContexts[i];
				if (!ColorContext)
				{
					continue;
				}
				WICColorContextType ContextType = WICColorContextType::WICColorContextUninitialized;
				ColorContext->GetType(&ContextType);
				if (ContextType == WICColorContextType::WICColorContextExifColorSpace)
				{
					UINT ExifColorSpace = 0u;
					if (SUCCEEDED(ColorContext->GetExifColorSpace(&ExifColorSpace)))
					{
						bDeclared = TRUE;
						if (ExifColorSpace == 1u)
						{
							OutDeclaredSRGB = TRUE;
						}
					}
				}
				else if (ContextType == WICColorContextType::WICColorContextProfile)
				{
					// GetProfileBytes answers the size when it is called with no buffer, and the
					// bytes when it is called with one - there is no separate size query.
					UINT ProfileSize = 0u;
					if ((SUCCEEDED(ColorContext->GetProfileBytes(0u, nullptr, &ProfileSize))) && (ProfileSize > 0u))
					{
						TArray<BYTE, UINT32> ProfileBytes;
						ProfileBytes.SetNum(ProfileSize);
						UINT WrittenSize = 0u;
						if (SUCCEEDED(ColorContext->GetProfileBytes(ProfileSize, ProfileBytes.GetData(), &WrittenSize)))
						{
							bDeclared = TRUE;
							if (ICCProfileDescribesSRGB(ProfileBytes.GetData(), static_cast<UINT32>(WrittenSize)) != FALSE)
							{
								OutDeclaredSRGB = TRUE;
							}
							else
							{
								PE_FAILED((ENGINE_ASSET_ERROR), ("This image carries an ICC profile that is not sRGB. The engine cannot convert colour spaces, so the texture is imported as linear data; export the image as sRGB to have it read as colour."));
							}
						}
					}
				}
			}
		}

		// The contexts are handed out with a reference each, whether or not every slot was filled.
		for (UINT32 i = 0u; i < ColorContextCount; i++)
		{
			if (ColorContexts[i])
			{
				ColorContexts[i]->Release();
				ColorContexts[i] = nullptr;
			}
		}
		return bDeclared;
	}

	static BOOL32 ReadAndDecodingDigitalImage(const EString& InPathName, BYTE*& OutByteCode, UINT32& OutWidth, UINT32& OutHeight, UINT32& OutPixelByteCount, RFormatType& OutFormat, BOOL32 InFallbackSRGB, BOOL32& OutSRGB)
	{
		// Initialize output values
		{
			if (OutByteCode)
			{
				delete[]OutByteCode;
				OutByteCode = nullptr;
			}
			OutWidth = 0u;
			OutHeight = 0u;
			OutPixelByteCount = 0u;
			OutFormat = RFormatType::FORMAT_UNKNOWN;
			// The importer's choice stands until the file says otherwise.
			OutSRGB = InFallbackSRGB;
		}

		HRESULT hr = ::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
		if (FAILED(hr))
		{
			return FALSE;
		}

		IWICImagingFactory* WICImagingFactory = nullptr;
		{
			hr = ::CoCreateInstance(
				CLSID_WICImagingFactory,
				NULL,
				CLSCTX_INPROC_SERVER,
				IID_PPV_ARGS(&WICImagingFactory)
			);

			if (FAILED(hr))
			{
				if (WICImagingFactory)
				{
					WICImagingFactory->Release();
					WICImagingFactory = nullptr;
				}
				return FALSE;
			}
		}

		IWICBitmapDecoder* WICBitmapDecoder = nullptr;
		{
			INT32 WideCharSize = ::MultiByteToWideChar(CP_UTF8, 0u, *InPathName, -1, NULL, 0);
			WCHAR* WideCharPathName = new WCHAR[WideCharSize];
			::MultiByteToWideChar(CP_UTF8, 0u, *InPathName, -1, WideCharPathName, WideCharSize);

			hr = WICImagingFactory->CreateDecoderFromFilename(
				WideCharPathName,					// Image to be decoded
				NULL,								// Do not prefer a particular vendor
				GENERIC_READ,						// Desired read access to the file
				WICDecodeMetadataCacheOnDemand,		// Cache metadata when needed
				&WICBitmapDecoder					// Pointer to the decoder
			);

			delete[]WideCharPathName;

			if (FAILED(hr))
			{
				if (WICBitmapDecoder)
				{
					WICBitmapDecoder->Release();
					WICBitmapDecoder = nullptr;
				}
				if (WICImagingFactory)
				{
					WICImagingFactory->Release();
					WICImagingFactory = nullptr;
				}
				return FALSE;
			}
		}

		IWICBitmapFrameDecode* WICBitmapFrameDecode = nullptr;
		{
			hr = WICBitmapDecoder->GetFrame(0, &WICBitmapFrameDecode);

			if (FAILED(hr))
			{
				if (WICBitmapFrameDecode)
				{
					WICBitmapFrameDecode->Release();
					WICBitmapFrameDecode = nullptr;
				}
				if (WICBitmapDecoder)
				{
					WICBitmapDecoder->Release();
					WICBitmapDecoder = nullptr;
				}
				if (WICImagingFactory)
				{
					WICImagingFactory->Release();
					WICImagingFactory = nullptr;
				}
				return FALSE;
			}
		}

		BYTE* OutputRenderPixelData = nullptr;
		{
			//Execute load texture logic.
			UINT32 FrameWidth = 0u, FrameHeight = 0u;
			WICBitmapFrameDecode->GetSize(&FrameWidth, &FrameHeight);

			WICPixelFormatGUID FramePixelFormat;
			WICBitmapFrameDecode->GetPixelFormat(&FramePixelFormat);

			// What the file says about its own colour space, read here while the frame is alive.
			// bColorSpaceDeclared says whether it said anything at all, which is what decides
			// between the file's answer and the importer's fallback below. The decoder reports an
			// embedded profile or Exif colour space; a PNG sRGB chunk does not reach it, so that
			// declaration is read from the file when the decoder reported nothing.
			BOOL32 DeclaredSRGB = FALSE;
			BOOL32 bColorSpaceDeclared = DetectImageColorSpace(WICBitmapFrameDecode, DeclaredSRGB);
			if (bColorSpaceDeclared == FALSE)
			{
				bColorSpaceDeclared = DetectPNGColorSpaceDeclaration(InPathName, DeclaredSRGB);
			}

			BOOL32 NeedAllocate = TRUE;
			UINT32 FormatByteCount = 0u;
			RFormatType FormatType = RFormatType::FORMAT_UNKNOWN;
			UINT32 RenderDataByteCount = 0u;
			BOOL32 NeedSwitchRB = FALSE;

#define WICPixelFormatToRenderFormat(_WICPixelFormatType, _WICFormatByteCount, _RenderFormat, _RenderDataByteCount, _NeedSwitchRB) \
			else if (FramePixelFormat == _WICPixelFormatType)\
			{\
				FormatByteCount	= _WICFormatByteCount;\
				FormatType = _RenderFormat;\
				RenderDataByteCount = _RenderDataByteCount;\
				NeedSwitchRB = _NeedSwitchRB;\
			}\

			if (FramePixelFormat == GUID_WICPixelFormat8bppGray)
			{
				FormatByteCount = 1u;
				FormatType = RFormatType::FORMAT_R8_UNORM;
				RenderDataByteCount = 1u;
				NeedSwitchRB = FALSE;
			}
			WICPixelFormatToRenderFormat(GUID_WICPixelFormat8bppAlpha, 1u, RFormatType::FORMAT_A8_UNORM, 1u, FALSE)
			WICPixelFormatToRenderFormat(GUID_WICPixelFormat16bppBGR555, 2u, RFormatType::FORMAT_B5G5R5A1_UNORM, 2u, FALSE)
			WICPixelFormatToRenderFormat(GUID_WICPixelFormat16bppBGRA5551, 2u, RFormatType::FORMAT_B5G5R5A1_UNORM, 2u, FALSE)
			WICPixelFormatToRenderFormat(GUID_WICPixelFormat16bppBGR565, 2u, RFormatType::FORMAT_B5G6R5_UNORM, 2u, FALSE)
			WICPixelFormatToRenderFormat(GUID_WICPixelFormat16bppGray, 2u, RFormatType::FORMAT_R16_UNORM, 2u, FALSE)
			WICPixelFormatToRenderFormat(GUID_WICPixelFormat16bppGrayHalf, 2u, RFormatType::FORMAT_R16_FLOAT, 2u, FALSE)
			WICPixelFormatToRenderFormat(GUID_WICPixelFormat24bppRGB, 3u, RFormatType::FORMAT_R8G8B8A8_UNORM, 4u, FALSE)
			WICPixelFormatToRenderFormat(GUID_WICPixelFormat24bppBGR, 3u, RFormatType::FORMAT_B8G8R8A8_UNORM, 4u, FALSE)
			WICPixelFormatToRenderFormat(GUID_WICPixelFormat32bppRGB, 4u, RFormatType::FORMAT_R8G8B8A8_UNORM, 4u, FALSE)
			WICPixelFormatToRenderFormat(GUID_WICPixelFormat32bppRGBA, 4u, RFormatType::FORMAT_R8G8B8A8_UNORM, 4u, FALSE)
			WICPixelFormatToRenderFormat(GUID_WICPixelFormat32bppBGR, 4u, RFormatType::FORMAT_B8G8R8X8_UNORM, 4u, FALSE)
			WICPixelFormatToRenderFormat(GUID_WICPixelFormat32bppBGRA, 4u, RFormatType::FORMAT_B8G8R8A8_UNORM, 4u, FALSE)
			WICPixelFormatToRenderFormat(GUID_WICPixelFormat32bppBGR101010, 4u, RFormatType::FORMAT_R10G10B10A2_UNORM, 4u, TRUE)
			WICPixelFormatToRenderFormat(GUID_WICPixelFormat32bppRGBA1010102, 4u, RFormatType::FORMAT_R10G10B10A2_UNORM, 4u, FALSE)
			WICPixelFormatToRenderFormat(GUID_WICPixelFormat32bppR10G10B10A2, 4u, RFormatType::FORMAT_R10G10B10A2_UNORM, 4u, FALSE)
			WICPixelFormatToRenderFormat(GUID_WICPixelFormat32bppR10G10B10A2HDR10, 4u, RFormatType::FORMAT_R10G10B10A2_UNORM, 4u, FALSE)
			WICPixelFormatToRenderFormat(GUID_WICPixelFormat32bppGrayFloat, 4u, RFormatType::FORMAT_R32_FLOAT, 4u, FALSE)
			WICPixelFormatToRenderFormat(GUID_WICPixelFormat48bppRGB, 6u, RFormatType::FORMAT_R16G16B16A16_UNORM, 8u, FALSE)
			WICPixelFormatToRenderFormat(GUID_WICPixelFormat48bppRGBHalf, 6u, RFormatType::FORMAT_R16G16B16A16_FLOAT, 8u, FALSE)
			WICPixelFormatToRenderFormat(GUID_WICPixelFormat48bppBGR, 6u, RFormatType::FORMAT_R16G16B16A16_UNORM, 8u, TRUE)
			WICPixelFormatToRenderFormat(GUID_WICPixelFormat64bppRGB, 8u, RFormatType::FORMAT_R16G16B16A16_UNORM, 8u, FALSE)
			WICPixelFormatToRenderFormat(GUID_WICPixelFormat64bppRGBHalf, 8u, RFormatType::FORMAT_R16G16B16A16_FLOAT, 8u, FALSE)
			WICPixelFormatToRenderFormat(GUID_WICPixelFormat64bppRGBA, 8u, RFormatType::FORMAT_R16G16B16A16_UNORM, 8u, FALSE)
			WICPixelFormatToRenderFormat(GUID_WICPixelFormat64bppBGRA, 8u, RFormatType::FORMAT_R16G16B16A16_UNORM, 8u, TRUE)
			WICPixelFormatToRenderFormat(GUID_WICPixelFormat64bppRGBAHalf, 8u, RFormatType::FORMAT_R16G16B16A16_FLOAT, 8u, FALSE)
			WICPixelFormatToRenderFormat(GUID_WICPixelFormat96bppRGBFloat, 12u, RFormatType::FORMAT_R32G32B32_FLOAT, 12u, FALSE)
			WICPixelFormatToRenderFormat(GUID_WICPixelFormat128bppRGBFloat, 16u, RFormatType::FORMAT_R32G32B32A32_FLOAT, 16u, FALSE)
			WICPixelFormatToRenderFormat(GUID_WICPixelFormat128bppRGBAFloat, 16u, RFormatType::FORMAT_R32G32B32A32_FLOAT, 16u, FALSE)
			else
			{
				PE_FAILED((ENGINE_ASSET_ERROR), ("Try to load texture that pixel format is not supported."));
				NeedAllocate = FALSE;
			}

#undef WICPixelFormatToRenderFormat

			if (NeedAllocate && (FormatByteCount > 0u) && (RenderDataByteCount > 0u) && (FormatType != RFormatType::FORMAT_UNKNOWN))
			{
				TArray<BYTE, UINT32> RawPixelData;

				const UINT32 PixelStride = FrameWidth * FormatByteCount;
				const UINT32 PixelByteSize = PixelStride * FrameHeight;
				RawPixelData.SetNum(PixelByteSize);

				WICBitmapFrameDecode->CopyPixels(NULL, PixelStride, RawPixelData.Num(), RawPixelData.GetData());

				const UINT32 RenderPixelStride = FrameWidth * RenderDataByteCount;
				const UINT32 RenderPixelByteSize = RenderPixelStride * FrameHeight;
				OutputRenderPixelData = new BYTE[RenderPixelByteSize]{ 0u };

				if ((RenderPixelByteSize == PixelByteSize) && (!NeedSwitchRB))
				{
					::memcpy_s(OutputRenderPixelData, RenderPixelByteSize, RawPixelData.GetData(), PixelByteSize);
				}
				else
				{
					for (UINT32 y = 0u; y < FrameHeight; y++)
					{
						for (UINT32 x = 0u; x < FrameWidth; x++)
						{
							UINT32 RawCoord = y * PixelStride + x * FormatByteCount;
							UINT32 RenderCoord = y * RenderPixelStride + x * RenderDataByteCount;
							for (UINT32 ByteIndex = 0u; ByteIndex < RenderDataByteCount; ByteIndex++)
							{
								if ((RenderCoord + ByteIndex) < RenderPixelByteSize)
								{
									if (ByteIndex < FormatByteCount)
									{
										OutputRenderPixelData[RenderCoord + ByteIndex] = RawPixelData[RawCoord + ByteIndex];
									}
									else
									{
										OutputRenderPixelData[RenderCoord + ByteIndex] = static_cast<BYTE>(0);
									}
								}
							}

							//TODO We can not simply switch R<->B byte.
							if (NeedSwitchRB)
							{
								//if ((RenderCoord + 2u) < RenderPixelByteSize)
								//{
								//	BYTE TempValue = OutputRenderPixelData[RenderCoord + 0u];
								//	OutputRenderPixelData[RenderCoord + 0u] = OutputRenderPixelData[RenderCoord + 2u];
								//	OutputRenderPixelData[RenderCoord + 2u] = TempValue;
								//}
							}
						}
					}
				}

				// Storage output values
				{
					OutByteCode = OutputRenderPixelData;
					OutWidth = FrameWidth;
					OutHeight = FrameHeight;
					OutPixelByteCount = RenderDataByteCount;
					OutFormat = FormatType;
					// The file's declaration wins when there is one; the importer's fallback answers
					// only for a file that declared nothing.
					const BOOL32 ResolvedSRGB = (bColorSpaceDeclared != FALSE) ? DeclaredSRGB : InFallbackSRGB;
					// An sRGB reading only exists for a format family that has an sRGB member - the
					// 8 bit and block compressed ones. A 16 bit image that declares sRGB therefore
					// keeps the linear reading instead of handing the render layer a request no API
					// can honour.
					if ((ResolvedSRGB != FALSE) && (RFormatTraits::GetSRGBCounterpart(FormatType) != RFormatType::FORMAT_UNKNOWN))
					{
						OutSRGB = TRUE;
					}
					else
					{
						OutSRGB = FALSE;
					}
				}
			}
		}

		//Release COM that we created.
		{
			if (WICBitmapFrameDecode)
			{
				WICBitmapFrameDecode->Release();
				WICBitmapFrameDecode = nullptr;
			}
			if (WICBitmapDecoder)
			{
				WICBitmapDecoder->Release();
				WICBitmapDecoder = nullptr;
			}
			if (WICImagingFactory)
			{
				WICImagingFactory->Release();
				WICImagingFactory = nullptr;
			}
		}
		::CoUninitialize();
		return (!!OutputRenderPixelData);
	}
#endif

	ETexture::ETexture()
		: TextureType(ETextureType::TEXTURE_TYPE_UNKNOWN), ByteCode(nullptr), SRGB(FALSE)
	{
	}
	ETexture::ETexture(const ETexture& Other)
		: TextureType(Other.TextureType), ByteCode(nullptr), ResourceProperties(Other.ResourceProperties), SRGB(Other.SRGB)
	{
	}
	ETexture::~ETexture()
	{
		ReleaseResource();
	}
	BOOL32 ETexture::IsResourceValid()const
	{
		return ((!!ByteCode) && (ResourceProperties.Format != RFormatType::FORMAT_UNKNOWN) && (ResourceProperties.Width > 0u) && (ResourceProperties.Height > 0u) && (ResourceProperties.PixelByteCount > 0u));
	}
	BOOL32 ETexture::InitResource()
	{
		// Texture resource must init by texture manager.
		return TRUE;
	}
	void ETexture::ReleaseResource()
	{
		ResourceProperties = ETextureResourceProperty();
		SRGB = FALSE;
		if (ByteCode)
		{
			delete[]ByteCode;
			ByteCode = nullptr;
		}
	}
	void ETexture::SetTextureData(BYTE* InByteCode, const ETextureResourceProperty& InProperty, BOOL32 InSRGB)
	{
		if (ByteCode)
		{
			delete[]ByteCode;
			ByteCode = nullptr;
		}
		ByteCode			= InByteCode;
		ResourceProperties	= InProperty;
		SRGB				= InSRGB;
	}

	ETexture2D::ETexture2D()
	{
		TextureType = ETextureType::TEXTURE_TYPE_TEXTURE2D;
	}
	ETexture2D::ETexture2D(const ETexture2D& Other)
		: ETexture(Other)
	{
		ResourceProperties.Depth = 0u;
		if (Other.IsResourceValid())
		{
			const UINT32 DataStride	= Other.ResourceProperties.Width * Other.ResourceProperties.PixelByteCount;
			const UINT32 DataSize	= DataStride * Other.ResourceProperties.Height;
			ByteCode = new BYTE[DataSize];
			::memcpy_s(ByteCode, DataSize, Other.ByteCode, DataSize);
		}
	}
	ETexture2D::~ETexture2D()
	{
	}
	ETexture2D& ETexture2D::operator=(const ETexture2D& Other)
	{
		if (ByteCode)
		{
			delete[]ByteCode;
			ByteCode = nullptr;
		}
		ResourceProperties	= Other.ResourceProperties;
		SRGB				= Other.SRGB;
		ResourceProperties.Depth = 0u;
		if (Other.IsResourceValid())
		{
			const UINT32 DataStride	= Other.ResourceProperties.Width * Other.ResourceProperties.PixelByteCount;
			const UINT32 DataSize	= DataStride * Other.ResourceProperties.Height;
			ByteCode = new BYTE[DataSize];
			::memcpy_s(ByteCode, DataSize, Other.ByteCode, DataSize);
		}
		return (*this);
	}
	BOOL32 ETexture2D::IsResourceValid()const
	{
		return ((TextureType == ETextureType::TEXTURE_TYPE_TEXTURE2D) && (ResourceProperties.Depth == 0u) && ETexture::IsResourceValid());
	}
	BOOL32 ETexture2D::InitResource()
	{
		// Texture resource must init by texture manager.
		return TRUE;
	}
	void ETexture2D::ReleaseResource()
	{
		ETexture::ReleaseResource();
	}
	void ETexture2D::SetTexture2DData(BYTE* InByteCode, UINT32 InWidth, UINT32 InHeigh, UINT32 InPixelByteCount, RFormatType InFormat, BOOL32 InSRGB)
	{
		SetTextureData(InByteCode, ETextureResourceProperty(InWidth, InHeigh, 0u, InPixelByteCount, InFormat), InSRGB);
	}

	ETextureCube::ETextureCube()
	{
		TextureType = ETextureType::TEXTURE_TYPE_TEXTURECUBE;
		SRGB = FALSE;
	}
	ETextureCube::ETextureCube(const ETextureCube& Other)
		: ETexture(Other)
	{
		SRGB = Other.SRGB;
		ResourceProperties.Depth = 6u;
		if (Other.IsResourceValid())
		{
			const UINT32 DataStride	= Other.ResourceProperties.Width * Other.ResourceProperties.PixelByteCount;
			const UINT32 DataSize	= DataStride * Other.ResourceProperties.Height * 6u;
			ByteCode = new BYTE[DataSize];
			::memcpy_s(ByteCode, DataSize, Other.ByteCode, DataSize);
		}
	}
	ETextureCube::~ETextureCube()
	{
	}
	ETextureCube& ETextureCube::operator=(const ETextureCube& Other)
	{
		if (ByteCode)
		{
			delete[]ByteCode;
			ByteCode = nullptr;
		}
		ResourceProperties	= Other.ResourceProperties;
		SRGB				= Other.SRGB;
		ResourceProperties.Depth = 6u;
		if (Other.IsResourceValid())
		{
			const UINT32 DataStride = Other.ResourceProperties.Width * Other.ResourceProperties.PixelByteCount;
			const UINT32 DataSize	= DataStride * Other.ResourceProperties.Height * 6u;
			ByteCode = new BYTE[DataSize];
			::memcpy_s(ByteCode, DataSize, Other.ByteCode, DataSize);
		}
		return (*this);
	}
	BOOL32 ETextureCube::IsResourceValid()const
	{
		// A cube carries an sRGB reading the same way a 2D texture does - the flag describes the
		// data, it does not make the cube invalid.
		return ((TextureType == ETextureType::TEXTURE_TYPE_TEXTURECUBE) && (ResourceProperties.Depth == 6u) && (ResourceProperties.Width == ResourceProperties.Height) && ETexture::IsResourceValid());
	}
	BOOL32 ETextureCube::InitResource()
	{
		// Texture resource must init by texture manager.
		return TRUE;
	}
	void ETextureCube::ReleaseResource()
	{
		ETexture::ReleaseResource();
	}
	void ETextureCube::SetTextureCubeData(BYTE* InByteCode, UINT32 InWidth, UINT32 InPixelByteCount, RFormatType InFormat, BOOL32 InSRGB)
	{
		SetTextureData(InByteCode, ETextureResourceProperty(InWidth, InWidth, 6u, InPixelByteCount, InFormat), InSRGB);
	}

	ETexture2DAsset::ETexture2DAsset(const EString& InAssetPath, const EString& InAssetName
#if _EDITOR_ONLY
		, const EString& InDebugName
#endif
	) : TBaseAsset<ETexture2D>(InAssetPath, InAssetName
#if _EDITOR_ONLY
		, InDebugName
#endif
	)
	{
	}
	ETexture2DAsset::~ETexture2DAsset()
	{
	}
	BOOL32 ETexture2DAsset::InitResource()
	{
		if (IsInitialized())
		{
#if _EDITOR_ONLY
			{
				EString ErrorInfo = EString("Texture2D name=[") + GetAssetName() + "] path = [" + GetAssetPath() + "] has been Initialized.";
				PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorInfo));
			}
#endif
			return TRUE;
		}
		// The creation command runs on the render thread and may not read asset members, so
		// the cold data pointer is taken here, where the command is enqueued. The asset keeps
		// that pixel data alive for its whole lifetime.
		ETexture2D* const ColdData = ResourceData;
		if (RenderBinding.QueueCreate([this, ColdData]() -> RTexture2DResource*
		{
			return (this->CreateTextureResource(ColdData));
		}))
		{
			bIsInitialized = TRUE;
			return TRUE;
		}
		return FALSE;
	}
	RTexture2DResource* ETexture2DAsset::CreateTextureResource(ETexture2D* InResource)
	{
		RTexture2DResource* Result = nullptr;
		if (!InResource)
		{
			return Result;
		}
		if (!(InResource->IsResourceValid()))
		{
			return Result;
		}
		IRRHIDevice* RenderDevice = EMainManager::GetManagerSingleton()->GetRenderDevice();
		RRHISubresourceData TempInitData;
		{
			TempInitData.Data       = InResource->ByteCode;
			TempInitData.RowPitch   = InResource->ResourceProperties.Width * InResource->ResourceProperties.PixelByteCount;
			TempInitData.SlicePitch = InResource->ResourceProperties.Width * InResource->ResourceProperties.PixelByteCount * InResource->ResourceProperties.Height;
		}
		// The texture's colour space is the SRV's, never the resource's: the stored bytes stay as
		// they are, the render layer creates a typeless resource plus an sRGB or linear view, and
		// the hardware decodes on read. A format family with no sRGB member (16 bit, float) has no
		// such view, so the linear reading is what it gets - see RFormatTraits.
		RFormatType ViewFormat = RFormatTraits::GetFormatForColorSpace(InResource->ResourceProperties.Format, InResource->SRGB);
		if (ViewFormat == RFormatType::FORMAT_UNKNOWN)
		{
			ViewFormat = RFormatTraits::GetLinearCounterpart(InResource->ResourceProperties.Format);
		}
		Result = new RTexture2DResource();
		if (!(Result->InitFromRHI(RenderDevice,
			RTextureDesc(
				InResource->ResourceProperties.Width,
				InResource->ResourceProperties.Height,
				RBindFlagType::BIND_SHADER_RESOURCE,
				InResource->ResourceProperties.Format,
				&ViewFormat),
			&TempInitData)))
		{
			delete Result;
			Result = nullptr;
		}
		return Result;
	}

	ETextureCubeAsset::ETextureCubeAsset(const EString& InAssetPath, const EString& InAssetName
#if _EDITOR_ONLY
		, const EString& InDebugName
#endif
	) : TBaseAsset<ETextureCube>(InAssetPath, InAssetName
#if _EDITOR_ONLY
		, InDebugName
#endif
	)
	{
	}
	ETextureCubeAsset::~ETextureCubeAsset()
	{
	}
	BOOL32 ETextureCubeAsset::InitResource()
	{
		if (IsInitialized())
		{
#if _EDITOR_ONLY
			{
				EString ErrorInfo = EString("TextureCube name=[") + GetAssetName() + "] path = [" + GetAssetPath() + "] has been Initialized.";
				PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorInfo));
			}
#endif
			return TRUE;
		}
		// The creation command runs on the render thread and may not read asset members, so
		// the cold data pointer is taken here, where the command is enqueued. The asset keeps
		// that pixel data alive for its whole lifetime.
		ETextureCube* const ColdData = ResourceData;
		if (RenderBinding.QueueCreate([this, ColdData]() -> RTextureCubeResource*
		{
			return (this->CreateTextureResource(ColdData));
		}))
		{
			bIsInitialized = TRUE;
			return TRUE;
		}
		return FALSE;
	}
	RTextureCubeResource* ETextureCubeAsset::CreateTextureResource(ETextureCube* InResource)
	{
		RTextureCubeResource* Result = nullptr;
		if (!InResource)
		{
			return Result;
		}
		if (!(InResource->IsResourceValid()))
		{
			return Result;
		}
		IRRHIDevice* RenderDevice = EMainManager::GetManagerSingleton()->GetRenderDevice();
		const UINT32	SingleFaceMemStride	= InResource->ResourceProperties.Width * InResource->ResourceProperties.PixelByteCount;
		const UINT32	SingleFaceMemSize	= SingleFaceMemStride * InResource->ResourceProperties.Height;
		//const UINT32	TotalFaceMemSize	= SingleFaceMemSize * 6u;
		BYTE*			StartPtr			= InResource->ByteCode;
		RRHISubresourceData TempInitData[6] =
		{
			{ &(StartPtr[0u * SingleFaceMemSize]), SingleFaceMemSize, SingleFaceMemStride, SingleFaceMemSize },
			{ &(StartPtr[1u * SingleFaceMemSize]), SingleFaceMemSize, SingleFaceMemStride, SingleFaceMemSize },
			{ &(StartPtr[2u * SingleFaceMemSize]), SingleFaceMemSize, SingleFaceMemStride, SingleFaceMemSize },
			{ &(StartPtr[3u * SingleFaceMemSize]), SingleFaceMemSize, SingleFaceMemStride, SingleFaceMemSize },
			{ &(StartPtr[4u * SingleFaceMemSize]), SingleFaceMemSize, SingleFaceMemStride, SingleFaceMemSize },
			{ &(StartPtr[5u * SingleFaceMemSize]), SingleFaceMemSize, SingleFaceMemStride, SingleFaceMemSize }
		};
		// The cube follows the same rule as a 2D texture: the colour space is the SRV's, so an
		// sRGB cube is stored linear/typeless and decoded by the sampler. An HDR cube - the sky
		// light's environment map, for instance - has no sRGB member in its format family and
		// therefore keeps the linear reading whatever its flag says.
		RFormatType ViewFormat = RFormatTraits::GetFormatForColorSpace(InResource->ResourceProperties.Format, InResource->SRGB);
		if (ViewFormat == RFormatType::FORMAT_UNKNOWN)
		{
			ViewFormat = RFormatTraits::GetLinearCounterpart(InResource->ResourceProperties.Format);
		}

#if _EDITOR_ONLY
		/*
		* A payload of zeroes is worth saying out loud, because everything downstream of it looks like
		* a different bug: the environment bake projects it into zero coefficients (a dispatch that ran
		* and wrote zeros), the sky samples black, and the only symptom left is "the indirect term is
		* missing". Checking the cold bytes here is the one place the answer is cheap - one pass over
		* the data at creation, and it names the payload rather than the consumers.
		*/
		{
			const UINT32 PayloadSize = SingleFaceMemSize * 6u;
			const BYTE* Payload = InResource->ByteCode;
			BOOL8 bAnyNonZeroByte = FALSE;
			if (Payload != nullptr)
			{
				for (UINT32 ByteIdx = 0u; (ByteIdx < PayloadSize) && (bAnyNonZeroByte == FALSE); ByteIdx++)
				{
					bAnyNonZeroByte = ((Payload[ByteIdx] != 0u) ? TRUE : FALSE);
				}
			}
			if ((Payload == nullptr) || (bAnyNonZeroByte == FALSE))
			{
				PE_LOG_ERROR(EString("[TextureCubeAsset] '") + GetAssetName()
					+ EString("' carries an all-zero payload (") + ToString(PayloadSize)
					+ EString(" bytes for six faces): the sky and the environment bake will both read black."));
			}
		}
#endif	// _EDITOR_ONLY

		Result = new RTextureCubeResource();
		if (!(Result->InitFromRHI(RenderDevice,
			RTextureDesc(
				InResource->ResourceProperties.Width,
				InResource->ResourceProperties.Height,
				RBindFlagType::BIND_SHADER_RESOURCE,
				InResource->ResourceProperties.Format,
				&ViewFormat),
			TempInitData, 6u)))
		{
			delete Result;
			Result = nullptr;
		}
		return Result;
	}

	ETextureAssetManager::ETextureAssetManager()
	{
#if _EDITOR_ONLY
		DebugName = ENGINE_TEXTURE_ASSET_MANAGER_NAME;
#endif
	}
	ETextureAssetManager::~ETextureAssetManager()
	{
	}
	void ETextureAssetManager::Initialize()
	{
	}
	void ETextureAssetManager::ShutDown()
	{
		ReleaseAllAssets();
	}
	void ETextureAssetManager::Uninit()
	{
		ReleaseAllAssets();
	}
	void ETextureAssetManager::ReleaseAllAssets()
	{
		ClearTexture2Ds();
		ClearTextureCubes();
	}
#if _EDITOR_ONLY
	BOOL32 ETextureAssetManager::ImportTexture2D(const EString& InAssetName, const EString& InImportFullPathName, const EString& InSavePath, BOOL32 InFallbackSRGB)
	{
		EString TempFullPathName(InSavePath);
		TempFullPathName = TempFullPathName + InAssetName + EEngineSettings::ENGINE_ASSET_NAME_TYPE;
		if ((InImportFullPathName.Length() < 3u) || (TempFullPathName.Length() < 10u))
		{
#if _EDITOR_ONLY
			{
				EString ErrorData("Error file path for texture 2d importer (import file path : ");
				ErrorData = ErrorData + InImportFullPathName + ", save assset path : " + TempFullPathName + ").";
				PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorData));
			}
#endif
			return FALSE;
		}
		EString ImportPathName; EString ImportFileType;
		if (!(SplitByLastSign('.', InImportFullPathName, ImportPathName, ImportFileType)))
		{
#if _EDITOR_ONLY
			{
				EString ErrorData("Error file path for texture 2d importer (import file path : ");
				ErrorData = ErrorData + InImportFullPathName + ", save assset path : " + TempFullPathName + ").";
				PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorData));
			}
#endif
			return FALSE;
		}
		//TODO Check import type(like jpg, png, tiff...).
		if (ImportPathName.Length() <= 3u)
		{
#if _EDITOR_ONLY
			{
				EString ErrorData("Error file path for texture 2d importer (import file path : ");
				ErrorData = ErrorData + InImportFullPathName + ", save assset path : " + TempFullPathName + ").";
				PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorData));
			}
#endif
			return FALSE;
		}
		BYTE* ReadByteCode = nullptr; UINT32 ReadWidth = 0u, ReadHeight = 0u, ReadPixelByteCount = 0u; RFormatType ReadFormat = RFormatType::FORMAT_UNKNOWN;
		BOOL32 ReadSRGB = FALSE;
		if (!ReadAndDecodingDigitalImage(InImportFullPathName, ReadByteCode, ReadWidth, ReadHeight, ReadPixelByteCount, ReadFormat, InFallbackSRGB, ReadSRGB))
		{
			if (ReadByteCode)
			{
				delete[]ReadByteCode;
				ReadByteCode = nullptr;
			}
			return FALSE;
		}
		if ((!ReadByteCode) || (ReadWidth == 0u) || (ReadHeight == 0u) || (ReadPixelByteCount == 0u) || (ReadFormat == RFormatType::FORMAT_UNKNOWN))
		{
			if (ReadByteCode)
			{
				delete[]ReadByteCode;
				ReadByteCode = nullptr;
			}
			return FALSE;
		}
		ETexture2D* NeedSaveTexture2DResource = new ETexture2D();
		// The sRGB flag comes from the image itself (its profile or Exif colour space) whenever the
		// file declares one; the importer's fallback answers only for files that declare nothing.
		NeedSaveTexture2DResource->SetTexture2DData(ReadByteCode, ReadWidth, ReadHeight, ReadPixelByteCount, ReadFormat, ReadSRGB);
		// Stated in the log because the fallback is easy to leave on by accident: a data texture
		// that was imported as colour does not look like an import mistake, it looks like a
		// shading bug.
		PE_LOG_WARN(EString(PE_TEXT("Texture2D '")) + InAssetName
			+ ((ReadSRGB != FALSE) ? PE_TEXT("' imported as sRGB (colour).") : PE_TEXT("' imported as linear (data).")));

		BOOL32 Result = SaveTexture2DAsset(InSavePath, InAssetName, NeedSaveTexture2DResource);

		NeedSaveTexture2DResource->ReleaseResource();
		delete NeedSaveTexture2DResource;
		return Result;
	}
	BOOL32 ETextureAssetManager::ImportTextureCube(const EString& InAssetName, const TArray<EString>& InImportFullPathName, const EString& InSavePath, BOOL32 InFallbackSRGB)
	{
		constexpr UINT32 TextureCubeRawAssetCount = 6u;

		EString TempFullPathName(InSavePath);
		TempFullPathName = TempFullPathName + InAssetName + EEngineSettings::ENGINE_ASSET_NAME_TYPE;
		{
			BOOL32 ImportPathValid = (InImportFullPathName.Num<UINT32>() == TextureCubeRawAssetCount) && (TempFullPathName.Length() >= 10u);
			if (ImportPathValid)
			{
				for (UINT32 ImportIndex = 0u; ImportIndex < TextureCubeRawAssetCount; ImportIndex++)
				{
					ImportPathValid = ImportPathValid && (InImportFullPathName[ImportIndex].Length() >= 3u);
				}
			}
			if (!ImportPathValid)
			{
#if _EDITOR_ONLY
				{
					EString ErrorData("Error file path for texture cube importer (import file path : ");
					ErrorData = ErrorData + ", save assset path : " + TempFullPathName + ").";
					PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorData));
				}
#endif
				return FALSE;
			}
		}
		BYTE* ReadByteCode[TextureCubeRawAssetCount] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
		UINT32 ReadWidth[TextureCubeRawAssetCount] = { 0u, 0u, 0u, 0u, 0u, 0u },
			ReadHeight[TextureCubeRawAssetCount] = { 0u, 0u, 0u, 0u, 0u, 0u },
			ReadPixelByteCount[TextureCubeRawAssetCount] = { 0u, 0u, 0u, 0u, 0u, 0u };
		RFormatType ReadFormat[TextureCubeRawAssetCount] =
		{
			RFormatType::FORMAT_UNKNOWN,RFormatType::FORMAT_UNKNOWN,RFormatType::FORMAT_UNKNOWN,
			RFormatType::FORMAT_UNKNOWN,RFormatType::FORMAT_UNKNOWN,RFormatType::FORMAT_UNKNOWN
		};
		BOOL32 ReadSRGB[TextureCubeRawAssetCount] = { FALSE, FALSE, FALSE, FALSE, FALSE, FALSE };
		BOOL32 ImportFullSuccess = TRUE;
		for (UINT32 ImportIndex = 0u; ImportIndex < TextureCubeRawAssetCount; ImportIndex++)
		{
			const EString& TempImportFullPathName = InImportFullPathName[ImportIndex];
			EString ImportPathName; EString ImportFileType;
			if (!(SplitByLastSign('.', TempImportFullPathName, ImportPathName, ImportFileType)))
			{
#if _EDITOR_ONLY
				{
					EString ErrorData("Error file path for texture cube importer (import file path : ");
					ErrorData = ErrorData + TempImportFullPathName + ", save assset path : " + TempFullPathName + ").";
					PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorData));
				}
#endif
				ImportFullSuccess = FALSE;
				continue;
			}
			//TODO Check import type(like jpg, png, tiff...).
			if (ImportPathName.Length() <= 3u)
			{
#if _EDITOR_ONLY
				{
					EString ErrorData("Error file path for texture 2d importer (import file path : ");
					ErrorData = ErrorData + TempImportFullPathName + ", save assset path : " + TempFullPathName + ").";
					PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorData));
				}
#endif
				ImportFullSuccess = FALSE;
				continue;
			}
			if (!ReadAndDecodingDigitalImage(TempImportFullPathName, ReadByteCode[ImportIndex], ReadWidth[ImportIndex], ReadHeight[ImportIndex], ReadPixelByteCount[ImportIndex], ReadFormat[ImportIndex], InFallbackSRGB, ReadSRGB[ImportIndex]))
			{
				if (ReadByteCode[ImportIndex])
				{
					delete[](ReadByteCode[ImportIndex]);
					ReadByteCode[ImportIndex] = nullptr;
				}
				ImportFullSuccess = FALSE;
				continue;
			}
			if ((!(ReadByteCode[ImportIndex])) || (ReadWidth[ImportIndex] == 0u) || (ReadHeight[ImportIndex] == 0u) || (ReadWidth[ImportIndex] != ReadHeight[ImportIndex]) || (ReadPixelByteCount[ImportIndex] == 0u) || (ReadFormat[ImportIndex] == RFormatType::FORMAT_UNKNOWN))
			{
				if (ReadByteCode[ImportIndex])
				{
					delete[](ReadByteCode[ImportIndex]);
					ReadByteCode[ImportIndex] = nullptr;
				}
				ImportFullSuccess = FALSE;
				continue;
			}
		}
		BOOL32 Result = FALSE;
		if (ImportFullSuccess)
		{
			Result = TRUE;
			for (UINT32 ImportIndex = 1u; ImportIndex < TextureCubeRawAssetCount; ImportIndex++)
			{
				Result = Result && (((ReadWidth[0] == ReadWidth[ImportIndex]) && (ReadFormat[0] == ReadFormat[ImportIndex])) && (ReadPixelByteCount[0] == ReadPixelByteCount[ImportIndex]));
			}
			if (Result)
			{
				const UINT32		TotalWidth			= ReadWidth[0];
				const UINT32		TotalPixelByteCount	= ReadPixelByteCount[0];
				const RFormatType	TotalFormat			= ReadFormat[0];
			// Every face has to agree on its colour space: the six images are one cube, and the
			// cube has a single sRGB flag. Faces that disagree are reported and the first face
			// decides, which keeps the import working with the reading the author most likely
			// meant instead of refusing the whole cube.
				BOOL32 TotalSRGB = ReadSRGB[0];
				for (UINT32 ImportIndex = 1u; ImportIndex < TextureCubeRawAssetCount; ImportIndex++)
				{
					if (ReadSRGB[ImportIndex] != TotalSRGB)
					{
						PE_FAILED((ENGINE_ASSET_ERROR), ("Texture cube faces disagree about their colour space (some declare sRGB, some do not). The first face's reading is used for the cube."));
						break;
					}
				}
				const UINT32		SingleMemSize		= TotalWidth * TotalPixelByteCount * TotalWidth;
				BYTE*				TotalReadByteCode	= new BYTE[SingleMemSize * TextureCubeRawAssetCount];
				for (UINT32 ImportIndex = 0u; ImportIndex < TextureCubeRawAssetCount; ImportIndex++)
				{
					::memcpy_s(&(TotalReadByteCode[SingleMemSize * ImportIndex]), SingleMemSize, ReadByteCode[ImportIndex], SingleMemSize);
				}

				ETextureCube* NeedSaveTextureCubeResource = new ETextureCube();
				NeedSaveTextureCubeResource->SetTextureCubeData(TotalReadByteCode, TotalWidth, TotalPixelByteCount, TotalFormat, TotalSRGB);
				PE_LOG_WARN(EString(PE_TEXT("TextureCube '")) + InAssetName
					+ ((TotalSRGB != FALSE) ? PE_TEXT("' imported as sRGB (colour).") : PE_TEXT("' imported as linear (data).")));

				Result = SaveTextureCubeAsset(InSavePath, InAssetName, NeedSaveTextureCubeResource);

				NeedSaveTextureCubeResource->ReleaseResource();
				delete NeedSaveTextureCubeResource;
			}
		}
		for (UINT32 ImportIndex = 0u; ImportIndex < TextureCubeRawAssetCount; ImportIndex++)
		{
			if (ReadByteCode[ImportIndex])
			{
				delete[](ReadByteCode[ImportIndex]);
				ReadByteCode[ImportIndex] = nullptr;
			}
		}
		return Result;
	}
#endif
	BOOL32 ETextureAssetManager::LoadTexture2DAsset(const EString& InLoadPath, const EString& InLoadName, const ETexture2DAsset*& OutTextureAsset, const BOOL32 bTryLoad)
	{
		EString TempFullPathName(InLoadPath);
		TempFullPathName = TempFullPathName + InLoadName + EEngineSettings::ENGINE_ASSET_NAME_TYPE;
		ETexture2DAsset* ResultAsset = Texture2DManager.Find(TempFullPathName);
		if (ResultAsset)
		{
			OutTextureAsset = ResultAsset;
			return TRUE;
		}
		ResultAsset = LoadTexture2DAsset(InLoadPath, InLoadName, nullptr, bTryLoad);
		if (!ResultAsset)
		{
			return FALSE;
		}
		if (!(ResultAsset->InitResource()))
		{
			delete ResultAsset;
			return FALSE;
		}
		if (Texture2DManager.Add(TempFullPathName, ResultAsset, TRUE) == 0u)
		{
#if _EDITOR_ONLY
			{
				EString ErrorInfo = EString("Texture2D path = [") + TempFullPathName + "] add into manager list failed.";
				PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorInfo));
			}
#endif
			delete ResultAsset;
			return FALSE;
		}
		OutTextureAsset = ResultAsset;
		return TRUE;
	}
	BOOL32 ETextureAssetManager::LoadTextureCubeAsset(const EString& InLoadPath, const EString& InLoadName, const ETextureCubeAsset*& OutTextureAsset, const BOOL32 bTryLoad)
	{
		EString TempFullPathName(InLoadPath);
		TempFullPathName = TempFullPathName + InLoadName + EEngineSettings::ENGINE_ASSET_NAME_TYPE;
		ETextureCubeAsset* ResultAsset = TextureCubeManager.Find(TempFullPathName);
		if (ResultAsset)
		{
			OutTextureAsset = ResultAsset;
			return TRUE;
		}
		ResultAsset = LoadTextureCubeAsset(InLoadPath, InLoadName, bTryLoad);
		if (!ResultAsset)
		{
			return FALSE;
		}
		if (!(ResultAsset->InitResource()))
		{
			delete ResultAsset;
			return FALSE;
		}
		if (TextureCubeManager.Add(TempFullPathName, ResultAsset, TRUE) == 0u)
		{
#if _EDITOR_ONLY
			{
				EString ErrorInfo = EString("TextureCube path = [") + TempFullPathName + "] add into manager list failed.";
				PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorInfo));
			}
#endif
			delete ResultAsset;
			return FALSE;
		}
		OutTextureAsset = ResultAsset;
		return TRUE;
	}
	BOOL32 ETextureAssetManager::RequireLoadTexture2DAsset(const EString& InLoadPath, const EString& InLoadName
		, const ETexture2DAsset*& OutTextureAsset, const FAssetImportSource* InImportSource, EAssetLoadCompleteType InOnLoadComplete)
	{
		const EString FullPathName = MakeAssetFullPathName(InLoadPath, InLoadName);
		if (ETexture2DAsset* LoadedAsset = Texture2DManager.Find(FullPathName))
		{
			OutTextureAsset = LoadedAsset;
			return TRUE;
		}
		// Not loaded yet. One request for the load thread, a null answer for the caller, and the
		// caller keeps running: this is what lets a frame or a draw ask for a texture it does not
		// have yet without waiting for the disk.
		OutTextureAsset = nullptr;
		// The import source is copied into the request: a request outlives the frame that made it,
		// so it may not read the caller's strings.
		FAssetImportSource ImportSource;
		if (InImportSource)
		{
			ImportSource = *InImportSource;
		}
		EAssetLoadManager::GetManagerSingleton()->RequestLoad(
			MakeAssetLoadRequestKey(PE_TEXT("Texture2D"), FullPathName),
			[InLoadPath, InLoadName, ImportSource]() -> BOOL32
			{
				const ETexture2DAsset* LoadedAsset = nullptr;
				if (ImportSource.IsValid())
				{
					// The editor form: load, and when the PAsset is not on disk yet, import from the
					// raw source and load again. Both halves run here, on the load thread.
					TryLoadTexture2D(InLoadPath, InLoadName, LoadedAsset, &ImportSource.ImportPath
						, &ImportSource.ImportName, &ImportSource.ImportFileType, ImportSource.bFallbackSRGB);
				}
				else
				{
					ETextureAssetManager::GetManagerSingleton()->LoadTexture2DAsset(InLoadPath, InLoadName, LoadedAsset, TRUE);
				}
				// TryLoad* answers with a pointer rather than a result, so the outcome is read off
				// the pointer it filled in.
				return ((LoadedAsset != nullptr) ? TRUE : FALSE);
			},
			static_cast<EAssetLoadCompleteType&&>(InOnLoadComplete));
		return FALSE;
	}
	BOOL32 ETextureAssetManager::RequireLoadTextureCubeAsset(const EString& InLoadPath, const EString& InLoadName
		, const ETextureCubeAsset*& OutTextureCubeAsset, const FAssetImportCubeSource* InImportSource, EAssetLoadCompleteType InOnLoadComplete)
	{
		const EString FullPathName = MakeAssetFullPathName(InLoadPath, InLoadName);
		if (ETextureCubeAsset* LoadedAsset = TextureCubeManager.Find(FullPathName))
		{
			OutTextureCubeAsset = LoadedAsset;
			return TRUE;
		}
		OutTextureCubeAsset = nullptr;
		FAssetImportCubeSource ImportSource;
		if (InImportSource)
		{
			ImportSource = *InImportSource;
		}
		EAssetLoadManager::GetManagerSingleton()->RequestLoad(
			MakeAssetLoadRequestKey(PE_TEXT("TextureCube"), FullPathName),
			[InLoadPath, InLoadName, ImportSource]() -> BOOL32
			{
				const ETextureCubeAsset* LoadedAsset = nullptr;
				if (ImportSource.IsValid())
				{
					TryLoadTextureCube(InLoadPath, InLoadName, LoadedAsset, &ImportSource.ImportPaths
						, &ImportSource.ImportNames, &ImportSource.ImportFileTypes, ImportSource.bFallbackSRGB);
				}
				else
				{
					ETextureAssetManager::GetManagerSingleton()->LoadTextureCubeAsset(InLoadPath, InLoadName, LoadedAsset, TRUE);
				}
				return ((LoadedAsset != nullptr) ? TRUE : FALSE);
			},
			static_cast<EAssetLoadCompleteType&&>(InOnLoadComplete));
		return FALSE;
	}
	void ETextureAssetManager::ClearTexture2Ds()
	{
		Texture2DManager.Clear();
	}
	void ETextureAssetManager::ClearTextureCubes()
	{
		TextureCubeManager.Clear();
	}
	ETexture2DAsset* ETextureAssetManager::LoadTexture2DAsset(const EString& InLoadPath, const EString& InLoadName, const BOOL32* InSRGBOverride, const BOOL32 bTryLoad)
	{
		EString TempFullPathName(InLoadPath);
		TempFullPathName = TempFullPathName + InLoadName + EEngineSettings::ENGINE_ASSET_NAME_TYPE;
		if (TempFullPathName.Length() < 10u)
		{
#if _EDITOR_ONLY
			{
				EString ErrorData("Loading texture 2d asset error.(load file path : ");
				ErrorData = ErrorData + TempFullPathName + ").";
				PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorData));
			}
#endif
			return nullptr;
		}
		ETexture2DAsset* OutTextureAsset = nullptr;
		void* ReadFileMem = nullptr; ULONG ReadFileSize = 0u;
		if (!EFileHelper::ReadFileAsBinary(TempFullPathName, ReadFileMem, ReadFileSize))
		{
			if (ReadFileMem)
			{
				delete[]ReadFileMem;
			}
#if _EDITOR_ONLY
			{
				EString ErrorData("Load texture 2d asset failed (load file path : ");
				ErrorData += TempFullPathName;
				ErrorData += ").";
				if (bTryLoad) { PE_LOG_WARN(ErrorData); }
				else          { PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorData)); }
			}
#endif
			return nullptr;
		}
		PE_CHECK((ENGINE_ASSET_ERROR), ("Error read texture 2d asset file size are too small."), (ReadFileSize > (sizeof(UINT32) * 2u + sizeof(BOOL32) + sizeof(ETextureResourceProperty))));
		void* TempPtr = ReadFileMem;
		ULONG RstSize = ReadFileSize;
		EAssetType ReadAssetType = EAssetType::ASSET_TYPE_UNKNOWN;
		ETextureType ReadTextureType = ETextureType::TEXTURE_TYPE_UNKNOWN;
		BOOL32 ReadSRGB = FALSE;
		ETextureResourceProperty ReadResourceProperty;
		{
			UINT32* SavedAssetTypePtr = (UINT32*)TempPtr;
			ReadAssetType = static_cast<EAssetType>(SavedAssetTypePtr[0]);
			TempPtr = (void*)(&(SavedAssetTypePtr[1]));
			RstSize = RstSize - sizeof(UINT32);
		}
		{
			UINT32* SavedAssetTypePtr = (UINT32*)TempPtr;
			UINT8 TempTextureType = static_cast<UINT8>(SavedAssetTypePtr[0]);
			ReadTextureType = static_cast<ETextureType>(TempTextureType);
			TempPtr = (void*)(&(SavedAssetTypePtr[1]));
			RstSize = RstSize - sizeof(UINT32);
		}
		{
			BOOL32* SavedAssetTypePtr = (BOOL32*)TempPtr;
			ReadSRGB = SavedAssetTypePtr[0];
			TempPtr = (void*)(&(SavedAssetTypePtr[1]));
			RstSize = RstSize - sizeof(BOOL32);
		}
		{
			ETextureResourceProperty* SavedAssetTypePtr = (ETextureResourceProperty*)TempPtr;
			ReadResourceProperty = SavedAssetTypePtr[0];
			TempPtr = (void*)(&(SavedAssetTypePtr[1]));
			RstSize = RstSize - sizeof(ETextureResourceProperty);
		}
		if ((ReadAssetType == EAssetType::ASSET_TYPE_TEXTURE) &&
			(ReadTextureType == ETextureType::TEXTURE_TYPE_TEXTURE2D) &&
			(ReadResourceProperty.Depth == 0u) &&
			(ReadResourceProperty.Width > 0u) &&
			(ReadResourceProperty.Height > 0u) &&
			(ReadResourceProperty.PixelByteCount > 0u) &&
			(ReadResourceProperty.Format != RFormatType::FORMAT_UNKNOWN))
		{
			const UINT32 ByteCodeStride = ReadResourceProperty.Width * ReadResourceProperty.PixelByteCount;
			const UINT32 ByteCodeSize = ByteCodeStride * ReadResourceProperty.Height;
			Check((RstSize == ByteCodeSize), (ENGINE_ASSET_ERROR));
			BYTE* ReadByteCode = new BYTE[ByteCodeSize];
			::memcpy_s(ReadByteCode, ByteCodeSize, TempPtr, RstSize);
			if (InSRGBOverride)
			{
				ReadSRGB = (*InSRGBOverride);
			}
			OutTextureAsset = new ETexture2DAsset(InLoadPath, InLoadName
#if _EDITOR_ONLY
				, InLoadName
#endif
			);
			ETexture2D* StoragedResource = nullptr;
			if (!(OutTextureAsset->StorageResourceInternal(
				[ReadSRGB, ReadByteCode, &ReadResourceProperty, &StoragedResource]()->ETexture2D*
				{
					StoragedResource = new ETexture2D();
					StoragedResource->SetTexture2DData(ReadByteCode, ReadResourceProperty.Width, ReadResourceProperty.Height, ReadResourceProperty.PixelByteCount, ReadResourceProperty.Format, ReadSRGB);
					return StoragedResource;
				})))
			{
				//TODO
			}
		}
		else
		{
			PE_FAILED((ENGINE_ASSET_ERROR), ("Loaded texture 2d PAsset is not valid."));
		}
		delete[]ReadFileMem;
		return OutTextureAsset;
	}
	BOOL32 ETextureAssetManager::SaveTexture2DAsset(const EString& InSavePath, const EString& InSaveName, const ETexture2D* InTextureResource)
	{
		EString TempFullPathName(InSavePath);
		TempFullPathName = TempFullPathName + InSaveName + EEngineSettings::ENGINE_ASSET_NAME_TYPE;
		if ((TempFullPathName.Length() < 10u) || (!InTextureResource) || (!(InTextureResource->IsResourceValid())))
		{
#if _EDITOR_ONLY
			{
				EString ErrorData("Saving texture 2d asset error.(save file path : ");
				ErrorData = ErrorData + TempFullPathName + ", is asset valid : " +
					((!!InTextureResource) ? ((InTextureResource->IsResourceValid()) ? "true" : "false") : "false")
					+ ").";
				PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorData));
			}
#endif
			return FALSE;
		}

		const UINT32 ByteCodeStride = InTextureResource->ResourceProperties.Width * InTextureResource->ResourceProperties.PixelByteCount;
		const UINT32 ByteCodeSize = ByteCodeStride * InTextureResource->ResourceProperties.Height;
		ULONG NeedSavedSize = sizeof(UINT32) + sizeof(UINT32) + sizeof(ETextureResourceProperty) + sizeof(BOOL32) + ByteCodeSize;	// EAssetType + TextureType + ETextureResourceProperty + SRGB + ByteCodeSize
		void* SavedMem = new BYTE[NeedSavedSize];
		void* TempPtr = SavedMem;
		{
			UINT32* SavedAssetTypePtr = (UINT32*)TempPtr;
			SavedAssetTypePtr[0] = EAssetType::ASSET_TYPE_TEXTURE;
			TempPtr = (void*)(&(SavedAssetTypePtr[1]));
		}
		{
			UINT32* SavedAssetTypePtr = (UINT32*)TempPtr;
			SavedAssetTypePtr[0] = static_cast<UINT32>(InTextureResource->TextureType);
			TempPtr = (void*)(&(SavedAssetTypePtr[1]));
		}
		{
			BOOL32* SavedAssetTypePtr = (BOOL32*)TempPtr;
			SavedAssetTypePtr[0] = InTextureResource->SRGB;
			TempPtr = (void*)(&(SavedAssetTypePtr[1]));
		}
		{
			ETextureResourceProperty* SavedAssetTypePtr = (ETextureResourceProperty*)TempPtr;
			SavedAssetTypePtr[0] = InTextureResource->ResourceProperties;
			TempPtr = (void*)(&(SavedAssetTypePtr[1]));
		}
		::memcpy_s(TempPtr, ByteCodeSize, InTextureResource->ByteCode, ByteCodeSize);

		BOOL32 Result = EFileHelper::SaveBytesToFile(TempFullPathName, SavedMem, NeedSavedSize);

		delete[]SavedMem;
		return Result;
	}
	ETextureCubeAsset* ETextureAssetManager::LoadTextureCubeAsset(const EString& InLoadPath, const EString& InLoadName, const BOOL32 bTryLoad)
	{
		EString TempFullPathName(InLoadPath);
		TempFullPathName = TempFullPathName + InLoadName + EEngineSettings::ENGINE_ASSET_NAME_TYPE;
		if (TempFullPathName.Length() < 10u)
		{
#if _EDITOR_ONLY
			{
				EString ErrorData("Loading texture cube asset error.(load file path : ");
				ErrorData = ErrorData + TempFullPathName + ").";
				PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorData));
			}
#endif
			return nullptr;
		}
		ETextureCubeAsset* OutTextureAsset = nullptr;
		void* ReadFileMem = nullptr; ULONG ReadFileSize = 0u;
		if (!EFileHelper::ReadFileAsBinary(TempFullPathName, ReadFileMem, ReadFileSize))
		{
			if (ReadFileMem)
			{
				delete[]ReadFileMem;
			}
#if _EDITOR_ONLY
			{
				EString ErrorData("Load texture cube asset failed (load file path : ");
				ErrorData += TempFullPathName;
				ErrorData += ").";
				if (bTryLoad) { PE_LOG_WARN(ErrorData); }
				else          { PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorData)); }
			}
#endif
			return nullptr;
		}
		PE_CHECK((ENGINE_ASSET_ERROR), ("Error read texture cube asset file size are too small."), (ReadFileSize > (sizeof(UINT32) * 2u + sizeof(BOOL32) + sizeof(ETextureResourceProperty))));
		void* TempPtr = ReadFileMem;
		ULONG RstSize = ReadFileSize;
		EAssetType ReadAssetType = EAssetType::ASSET_TYPE_UNKNOWN;
		ETextureType ReadTextureType = ETextureType::TEXTURE_TYPE_UNKNOWN;
		BOOL32 ReadSRGB = FALSE;
		ETextureResourceProperty ReadResourceProperty;
		{
			UINT32* SavedAssetTypePtr = (UINT32*)TempPtr;
			ReadAssetType = static_cast<EAssetType>(SavedAssetTypePtr[0]);
			TempPtr = (void*)(&(SavedAssetTypePtr[1]));
			RstSize = RstSize - sizeof(UINT32);
		}
		{
			UINT32* SavedAssetTypePtr = (UINT32*)TempPtr;
			UINT8 TempTextureType = static_cast<UINT8>(SavedAssetTypePtr[0]);
			ReadTextureType = static_cast<ETextureType>(TempTextureType);
			TempPtr = (void*)(&(SavedAssetTypePtr[1]));
			RstSize = RstSize - sizeof(UINT32);
		}
		{
			BOOL32* SavedAssetTypePtr = (BOOL32*)TempPtr;
			ReadSRGB = SavedAssetTypePtr[0];
			TempPtr = (void*)(&(SavedAssetTypePtr[1]));
			RstSize = RstSize - sizeof(BOOL32);
		}
		{
			ETextureResourceProperty* SavedAssetTypePtr = (ETextureResourceProperty*)TempPtr;
			ReadResourceProperty = SavedAssetTypePtr[0];
			TempPtr = (void*)(&(SavedAssetTypePtr[1]));
			RstSize = RstSize - sizeof(ETextureResourceProperty);
		}
		if ((ReadAssetType == EAssetType::ASSET_TYPE_TEXTURE) &&
			(ReadTextureType == ETextureType::TEXTURE_TYPE_TEXTURECUBE) &&
			(ReadResourceProperty.Depth == 6u) &&
			(ReadResourceProperty.Width > 0u) &&
			(ReadResourceProperty.Height > 0u) &&
			(ReadResourceProperty.Width == ReadResourceProperty.Height) &&
			(ReadResourceProperty.PixelByteCount > 0u) &&
			(ReadResourceProperty.Format != RFormatType::FORMAT_UNKNOWN))
		{
			const UINT32 ByteCodeStride = ReadResourceProperty.Width * ReadResourceProperty.PixelByteCount;
			const UINT32 ByteCodeSize = ByteCodeStride * ReadResourceProperty.Height * 6u;
			Check((RstSize == ByteCodeSize), (ENGINE_ASSET_ERROR));
			BYTE* ReadByteCode = new BYTE[ByteCodeSize];
			::memcpy_s(ReadByteCode, ByteCodeSize, TempPtr, RstSize);

			OutTextureAsset = new ETextureCubeAsset(InLoadPath, InLoadName
#if _EDITOR_ONLY
				, InLoadName
#endif
			);
			ETextureCube* StoragedResource = nullptr;
			if (!(OutTextureAsset->StorageResourceInternal(
				[ReadSRGB, ReadByteCode, &ReadResourceProperty, &StoragedResource]()->ETextureCube*
				{
					StoragedResource = new ETextureCube();
					StoragedResource->SetTextureCubeData(ReadByteCode, ReadResourceProperty.Width, ReadResourceProperty.PixelByteCount, ReadResourceProperty.Format, ReadSRGB);
					return StoragedResource;
				})))
			{
				//TODO
			}
		}
		else
		{
			PE_FAILED((ENGINE_ASSET_ERROR), ("Loaded texture cube PAsset is not valid."));
		}
		delete[]ReadFileMem;
		return OutTextureAsset;
	}
	BOOL32 ETextureAssetManager::SaveTextureCubeAsset(const EString& InSavePath, const EString& InSaveName, const ETextureCube* InTextureResource)
	{
		EString TempFullPathName(InSavePath);
		TempFullPathName = TempFullPathName + InSaveName + EEngineSettings::ENGINE_ASSET_NAME_TYPE;
		if ((TempFullPathName.Length() < 10u) || (!InTextureResource) || (!(InTextureResource->IsResourceValid())))
		{
#if _EDITOR_ONLY
			{
				EString ErrorData("Saving texture cube asset error.(save file path : ");
				ErrorData = ErrorData + TempFullPathName + ", is asset valid : " +
					((!!InTextureResource) ? ((InTextureResource->IsResourceValid()) ? "true" : "false") : "false")
					+ ").";
				PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorData));
			}
#endif
			return FALSE;
		}

		const UINT32 ByteCodeStride = InTextureResource->ResourceProperties.Width * InTextureResource->ResourceProperties.PixelByteCount;
		const UINT32 ByteCodeSize = ByteCodeStride * InTextureResource->ResourceProperties.Height * 6u;
		ULONG NeedSavedSize = sizeof(UINT32) + sizeof(UINT32) + sizeof(ETextureResourceProperty) + sizeof(BOOL32) + ByteCodeSize;	// EAssetType + TextureType + ETextureResourceProperty + SRGB + ByteCodeSize
		void* SavedMem = new BYTE[NeedSavedSize];
		void* TempPtr = SavedMem;
		{
			UINT32* SavedAssetTypePtr = (UINT32*)TempPtr;
			SavedAssetTypePtr[0] = EAssetType::ASSET_TYPE_TEXTURE;
			TempPtr = (void*)(&(SavedAssetTypePtr[1]));
		}
		{
			UINT32* SavedAssetTypePtr = (UINT32*)TempPtr;
			SavedAssetTypePtr[0] = static_cast<UINT32>(InTextureResource->TextureType);
			TempPtr = (void*)(&(SavedAssetTypePtr[1]));
		}
		{
			BOOL32* SavedAssetTypePtr = (BOOL32*)TempPtr;
			SavedAssetTypePtr[0] = InTextureResource->SRGB;
			TempPtr = (void*)(&(SavedAssetTypePtr[1]));
		}
		{
			ETextureResourceProperty* SavedAssetTypePtr = (ETextureResourceProperty*)TempPtr;
			SavedAssetTypePtr[0] = InTextureResource->ResourceProperties;
			TempPtr = (void*)(&(SavedAssetTypePtr[1]));
		}
		::memcpy_s(TempPtr, ByteCodeSize, InTextureResource->ByteCode, ByteCodeSize);

		BOOL32 Result = EFileHelper::SaveBytesToFile(TempFullPathName, SavedMem, NeedSavedSize);

		delete[]SavedMem;
		return Result;
	}

	void TryLoadTexture2D(const EString& InLoadPath, const EString& InLoadName, const ETexture2DAsset*& OutTextureAsset, const EString* InImportPath, const EString* InImportName, const EString* InImportFileType, BOOL32 InFallbackSRGB)
	{
		ETextureAssetManager* TextureAssetManager = ETextureAssetManager::GetManagerSingleton();
		PE_CHECK((ENGINE_ASSET_ERROR), ("Check output texture asset pointer is initialized failed."), (!OutTextureAsset));
		if (TextureAssetManager->LoadTexture2DAsset(InLoadPath, InLoadName, OutTextureAsset, TRUE))
		{
			return;
		}
#if _EDITOR_ONLY
		const BOOL32 bHasImport = (!!InImportPath) && (!!InImportName) && (!!InImportFileType);
		if (bHasImport)
		{
			PE_LOG_WARN(EString("Texture2D '") + InLoadName + "' not yet on disk; will attempt import.");
		}
		else
		{
			PE_FAILED((ENGINE_ASSET_ERROR), ("Load texture 2d asset failed."));
		}
		if (bHasImport)
		{
			EString TempImportFullPath(*InImportPath);
			TempImportFullPath = TempImportFullPath + (*InImportName) + (".") + (*InImportFileType);
			BOOL32 Result = TextureAssetManager->ImportTexture2D(
				InLoadName,
				TempImportFullPath,
				InLoadPath,
				InFallbackSRGB);
			if (Result)
			{
				if (!(TextureAssetManager->LoadTexture2DAsset(InLoadPath, InLoadName, OutTextureAsset)))
				{
					PE_FAILED((ENGINE_ASSET_ERROR), ("Try load texture 2d asset failed."));
				}
			}
			else
			{
				PE_FAILED((ENGINE_ASSET_ERROR), ("Try import texture 2d asset failed."));
			}
		}
#else
		PE_FAILED((ENGINE_ASSET_ERROR), ("Load texture 2d asset failed."));
#endif
	}
	void TryLoadTextureCube(const EString& InLoadPath, const EString& InLoadName, const ETextureCubeAsset*& OutTextureAsset, const TArray<EString>* InImportPath, const TArray<EString>* InImportName, const TArray<EString>* InImportFileType, BOOL32 InFallbackSRGB)
	{
		ETextureAssetManager* TextureAssetManager = ETextureAssetManager::GetManagerSingleton();
		PE_CHECK((ENGINE_ASSET_ERROR), ("Check output texture asset pointer is initialized failed."), (!OutTextureAsset));
		if (TextureAssetManager->LoadTextureCubeAsset(InLoadPath, InLoadName, OutTextureAsset, TRUE))
		{
			return;
		}
#if _EDITOR_ONLY
		const BOOL32 bHasImport = (!!InImportPath) && (!!InImportName) && (!!InImportFileType);
		if (bHasImport)
		{
			PE_LOG_WARN(EString("TextureCube '") + InLoadName + "' not yet on disk; will attempt import.");
		}
		else
		{
			PE_FAILED((ENGINE_ASSET_ERROR), ("Load texture cube asset failed."));
		}
		if (bHasImport)
		{
			constexpr UINT32 TextureCubeRawAssetCount = 6u;
			if ((InImportPath->Num<UINT32>() == TextureCubeRawAssetCount) && (InImportName->Num<UINT32>() == TextureCubeRawAssetCount) && (InImportFileType->Num<UINT32>() == TextureCubeRawAssetCount))
			{
				TArray<EString> TempImportFullPath;
				for (UINT32 ImportIndex = 0u; ImportIndex < TextureCubeRawAssetCount; ImportIndex++)
				{
					TempImportFullPath.Add((*InImportPath)[ImportIndex] + (*InImportName)[ImportIndex] + (".") + (*InImportFileType)[ImportIndex]);
				}
				BOOL32 Result = TextureAssetManager->ImportTextureCube(
					InLoadName,
					TempImportFullPath,
					InLoadPath,
					InFallbackSRGB);
				if (Result)
				{
					if (!(TextureAssetManager->LoadTextureCubeAsset(InLoadPath, InLoadName, OutTextureAsset)))
					{
						PE_FAILED((ENGINE_ASSET_ERROR), ("Try load texture cube asset failed."));
					}
				}
				else
				{
					PE_FAILED((ENGINE_ASSET_ERROR), ("Try import texture cube asset failed."));
				}
			}
			else
			{
				PE_FAILED((ENGINE_ASSET_ERROR), ("Try import texture cube asset path invalid."));
			}
		}
#else
		PE_FAILED((ENGINE_ASSET_ERROR), ("Load texture cube asset failed."));
#endif
	}

};
