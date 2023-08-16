#include "TextureAsset.h"
#include <RenderDevice/DeviceD3D11.h>

namespace PigeonEngine
{

	static BYTE* ReadAndDecodingDigitalImage(const EString& InPathName)
	{
		HRESULT hr = ::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
		if (FAILED(hr))
		{
			return nullptr;
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
				return nullptr;
			}
		}
		IWICBitmapDecoder* WICBitmapDecoder = nullptr;
		{
			INT WideCharSize = ::MultiByteToWideChar(CP_UTF8, 0u, *InPathName, -1, NULL, 0);
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
				return nullptr;
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
				return nullptr;
			}
		}
		BYTE* OutputRenderPixelData = nullptr;
		{
			//Execute load texture logic.
			UINT FrameWidth = 0u, FrameHeight = 0u;
			WICBitmapFrameDecode->GetSize(&FrameWidth, &FrameHeight);

			WICPixelFormatGUID FramePixelFormat;
			WICBitmapFrameDecode->GetPixelFormat(&FramePixelFormat);

			BOOL NeedAllocate = TRUE;
			UINT FormatByteCount = 0u;
			RFormatType FormatType = RFormatType::FORMAT_UNKNOWN;
			UINT RenderDataByteCount = 0u;
			BOOL NeedSwitchRB = FALSE;

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
				PE_FAILED((ENGINE_TEXTURE_ERROR), ("Try to load texture that pixel format is not supported."));
				NeedAllocate = FALSE;
			}

#undef WICPixelFormatToRenderFormat

			if (NeedAllocate && (FormatByteCount > 0u) && (RenderDataByteCount > 0u) && (FormatType != RFormatType::FORMAT_UNKNOWN))
			{
				TArray<BYTE> RawPixelData;

				const UINT PixelStride = FrameWidth * FormatByteCount;
				const UINT PixelByteSize = PixelStride * FrameHeight;
				RawPixelData.Resize(PixelByteSize);

				WICBitmapFrameDecode->CopyPixels(NULL, PixelStride, RawPixelData.Length(), RawPixelData.RawData());

				const UINT RenderPixelStride = FrameWidth * RenderDataByteCount;
				const UINT RenderPixelByteSize = RenderPixelStride * FrameHeight;
				OutputRenderPixelData = new BYTE[RenderPixelByteSize];

				if ((RenderPixelByteSize == PixelByteSize) && (!NeedSwitchRB))
				{
					::memcpy_s(OutputRenderPixelData, RenderPixelByteSize, RawPixelData.RawData(), PixelByteSize);
				}
				else
				{
					for (UINT y = 0u; y < FrameHeight; y++)
					{
						for (UINT x = 0u; x < FrameWidth; x++)
						{
							UINT RawCoord = y * PixelStride + x * FormatByteCount;
							UINT RenderCoord = y * RenderPixelStride + x * RenderDataByteCount;
							for (UINT ByteIndex = 0u; ByteIndex < RenderDataByteCount; ByteIndex++)
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
							if (NeedSwitchRB)
							{
								BYTE TempValue = OutputRenderPixelData[RenderCoord + 0u];
								OutputRenderPixelData[RenderCoord + 0u] = OutputRenderPixelData[RenderCoord + 2u];
								OutputRenderPixelData[RenderCoord + 2u] = TempValue;
							}
						}
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
		return OutputRenderPixelData;
	}

	ETexture2D::ETexture2D()
		: ByteCode(nullptr)
		, Width(0u)
		, Height(0u)
		, PixelByteCount(0u)
		, Format(RFormatType::FORMAT_UNKNOWN)
		, SRGB(FALSE)
	{
	}
	ETexture2D::ETexture2D(const ETexture2D& Other)
		: ByteCode(nullptr)
		, Width(Other.Width)
		, Height(Other.Height)
		, PixelByteCount(Other.PixelByteCount)
		, Format(Other.Format)
		, SRGB(Other.SRGB)
	{
		if ((!!(Other.ByteCode)) && (Other.Width > 0u) && (Other.Height > 0u) && (Other.PixelByteCount > 0u) && (Other.Format != RFormatType::FORMAT_UNKNOWN))
		{
			const UINT DataStride = Other.Width * Other.PixelByteCount;
			const UINT DataSize = DataStride * Other.Height;
			ByteCode = new BYTE[DataSize];
			::memcpy_s(ByteCode, DataSize, Other.ByteCode, DataSize);
		}
	}
	ETexture2D::~ETexture2D()
	{
		Release();
	}
	ETexture2D& ETexture2D::operator=(const ETexture2D& Other)
	{
		if (ByteCode)
		{
			delete[]ByteCode;
			ByteCode = nullptr;
		}
		Width			= Other.Width;
		Height			= Other.Height;
		PixelByteCount	= Other.PixelByteCount;
		Format			= Other.Format;
		SRGB			= Other.SRGB;
		if ((!!(Other.ByteCode)) && (Other.Width > 0u) && (Other.Height > 0u) && (Other.PixelByteCount > 0u) && (Other.Format != RFormatType::FORMAT_UNKNOWN))
		{
			const UINT DataStride = Other.Width * Other.PixelByteCount;
			const UINT DataSize = DataStride * Other.Height;
			ByteCode = new BYTE[DataSize];
			::memcpy_s(ByteCode, DataSize, Other.ByteCode, DataSize);
		}
		return (*this);
	}
	BOOL ETexture2D::IsValid()const
	{
		return ((!!ByteCode) && (Width > 0u) && (Height > 0u) && (PixelByteCount > 0u) && (Format != RFormatType::FORMAT_UNKNOWN));
	}
	void ETexture2D::Release()
	{
		Width			= 0u;
		Height			= 0u;
		PixelByteCount	= 0u;
		Format			= RFormatType::FORMAT_UNKNOWN;
		SRGB			= FALSE;
		if (ByteCode)
		{
			delete[]ByteCode;
			ByteCode = nullptr;
		}
	}
	void ETexture2D::SetData(BYTE* InByteCode, UINT InWidth, UINT InHeight, UINT InPixelByteCount, RFormatType InFormat, BOOL InSRGB)
	{
		ByteCode		= InByteCode;
		Width			= InWidth;
		Height			= InHeight;
		PixelByteCount	= InPixelByteCount;
		Format			= InFormat;
		SRGB			= InSRGB;
	}

	ETexture2DAsset::ETexture2DAsset(
		const EString& InPath
#ifdef _EDITOR_ONLY
		, const EString& InDebugName
#endif
	) : TRenderBaseAsset<ETexture2D, RTexture2DResource>(
#ifdef _EDITOR_ONLY
		InDebugName
#endif
	), TexturePath(InPath)
	{
	}
	ETexture2DAsset::~ETexture2DAsset()
	{
	}
	BOOL ETexture2DAsset::InitResource()
	{
		return FALSE;
	}
	const EString& ETexture2DAsset::GetTexturePath()const
	{
		return TexturePath;
	}
	RTexture2DResource* ETexture2DAsset::CreateTextureResource(ETexture2D* InResource)
	{
		RTexture2DResource* Result = nullptr;
		//if ((!!InResource) && (InResource->IsValid()))
		//{
		//	RDeviceD3D11* deviceD3D11 = RDeviceD3D11::GetDeviceSingleton();
		//	Result = new RTexture2DResource();
		//	if (!deviceD3D11->CreateTexture2D())
		//	{
		//		delete Result;
		//		Result = nullptr;
		//	}
		//}
		return Result;
	}

};