#include "TextureAsset.h"

namespace PigeonEngine
{

	BOOL DecodingDigitalImage(const EString& InPathName)
	{
		BOOL Result = FALSE;
		HRESULT hr = ::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
		if (FAILED(hr))
		{
			return Result;
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
				return Result;
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
				return Result;
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
				return Result;
			}
		}
		//Execute load texture logic.
		{
			UINT FrameWidth = 0u, FrameHeight = 0u;
			WICBitmapFrameDecode->GetSize(&FrameWidth, &FrameHeight);

			WICPixelFormatGUID FramePixelFormat;
			WICBitmapFrameDecode->GetPixelFormat(&FramePixelFormat);

			Result = TRUE;
			UINT FormatByteCount = 0u;
			RFormatType FormatType = RFormatType::FORMAT_UNKNOWN;



			;
			;
			;
			;

			;
			GUID_WICPixelFormat48bppBGR;

			GUID_WICPixelFormat64bppRGB;
			GUID_WICPixelFormat64bppBGRA;

			GUID_WICPixelFormat128bppRGBFloat;
			GUID_WICPixelFormat64bppRGBHalf;
			GUID_WICPixelFormat48bppRGBHalf;

			if ()
			{

			}
			else if (FramePixelFormat == GUID_WICPixelFormat8bppGray)
			{
				FormatByteCount = 1u;
				FormatType = FORMAT_R8_UNORM;
			}
			else if (FramePixelFormat == GUID_WICPixelFormat8bppAlpha)
			{
				FormatByteCount = 1u;
				FormatType = FORMAT_A8_UNORM;
			}
			else if (FramePixelFormat == GUID_WICPixelFormat16bppBGR555)
			{
				//TODO
				FormatByteCount = 2u;
				FormatType = FORMAT_B5G5R5A1_UNORM;
			}
			else if (FramePixelFormat == GUID_WICPixelFormat16bppBGRA5551)
			{
				FormatByteCount = 2u;
				FormatType = FORMAT_B5G5R5A1_UNORM;
			}
			else if (FramePixelFormat == GUID_WICPixelFormat16bppBGR565)
			{
				FormatByteCount = 2u;
				FormatType = FORMAT_B5G6R5_UNORM;
			}
			else if (FramePixelFormat == GUID_WICPixelFormat16bppGray)
			{
				FormatByteCount = 2u;
				FormatType = FORMAT_R16_UNORM;
			}
			else if (FramePixelFormat == GUID_WICPixelFormat16bppGrayHalf)
			{
				FormatByteCount = 2u;
				FormatType = FORMAT_R16_FLOAT;
			}
			else if (FramePixelFormat == GUID_WICPixelFormat24bppRGB)
			{
				//TODO
				FormatByteCount = 3u;
				FormatType = FORMAT_R8G8B8A8_UNORM;
			}
			else if (FramePixelFormat == GUID_WICPixelFormat24bppBGR)
			{
				//TODO
				FormatByteCount = 3u;
				FormatType = FORMAT_B8G8R8A8_UNORM;
			}
			else if (FramePixelFormat == GUID_WICPixelFormat32bppRGB)
			{
				//TODO
				FormatByteCount = 4u;
				FormatType = FORMAT_R8G8B8A8_UNORM;
			}
			else if (FramePixelFormat == GUID_WICPixelFormat32bppRGBA)
			{
				FormatByteCount = 4u;
				FormatType = FORMAT_R8G8B8A8_UNORM;
			}
			else if (FramePixelFormat == GUID_WICPixelFormat32bppBGR)
			{
				FormatByteCount = 4u;
				FormatType = FORMAT_B8G8R8X8_UNORM;
			}
			else if (FramePixelFormat == GUID_WICPixelFormat32bppBGRA)
			{
				FormatByteCount = 4u;
				FormatType = FORMAT_B8G8R8A8_UNORM;
			}
			else if (FramePixelFormat == GUID_WICPixelFormat32bppBGR101010)
			{
				//TODO
				FormatByteCount = 4u;
				FormatType = FORMAT_R10G10B10A2_UNORM;
			}
			else if (FramePixelFormat == GUID_WICPixelFormat32bppRGBA1010102)
			{
				FormatByteCount = 4u;
				FormatType = FORMAT_R10G10B10A2_UNORM;
			}
			else if (FramePixelFormat == GUID_WICPixelFormat32bppR10G10B10A2)
			{
				FormatByteCount = 4u;
				FormatType = FORMAT_R10G10B10A2_UNORM;
			}
			else if (FramePixelFormat == GUID_WICPixelFormat32bppR10G10B10A2HDR10)
			{
				//TODO Does we need this format?
				FormatByteCount = 4u;
				FormatType = FORMAT_R10G10B10A2_UNORM;
			}
			else if (FramePixelFormat == GUID_WICPixelFormat32bppGrayFloat)
			{
				FormatByteCount = 4u;
				FormatType = FORMAT_R32_FLOAT;
			}
			else if (FramePixelFormat == GUID_WICPixelFormat48bppRGB)
			{
				//TODO
				FormatByteCount = 6u;
				FormatType = FORMAT_R16G16B16A16_UNORM;
			}
			else if (FramePixelFormat == GUID_WICPixelFormat64bppRGBA)
			{
				FormatByteCount = 8u;
				FormatType = FORMAT_R16G16B16A16_UNORM;
			}
			else if (FramePixelFormat == GUID_WICPixelFormat64bppRGBAHalf)
			{
				FormatByteCount = 8u;
				FormatType = FORMAT_R16G16B16A16_FLOAT;
			}
			else if (FramePixelFormat == GUID_WICPixelFormat96bppRGBFloat)
			{
				FormatByteCount = 12u;
				FormatType = FORMAT_R32G32B32_FLOAT;
			}
			else if (FramePixelFormat == GUID_WICPixelFormat128bppRGBAFloat)
			{
				FormatByteCount = 16u;
				FormatType = FORMAT_R32G32B32A32_FLOAT;
			}
			else
			{
				PE_FAILED((ENGINE_TEXTURE_ERROR), ("Try to load texture that pixel format is not supported."));
				Result = FALSE;
			}
			if (Result)
			{

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
		return Result;
	}

	ETexture::ETexture()
	{
	}
	ETexture::ETexture(const EString& InTexName)
	{
	}
	ETexture::ETexture(const ETexture& Other)
	{
	}
	ETexture::~ETexture()
	{
	}
	ETexture& ETexture::operator=(const ETexture& Other)
	{
		return (*this);
	}

};