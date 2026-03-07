#include "TextureAsset.h"
#include <RenderDevice/DeviceD3D11.h>

namespace PigeonEngine
{

	template <typename _TAssetResourceType, typename _TAssetRenderResourceType, typename _AssetType>
	static void RegisterTextureClassTypes()
	{
		RegisterRenderBaseAssetClassTypes<_TAssetResourceType, _TAssetRenderResourceType>();
		RegisterClassType<_AssetType, TRenderBaseAsset<_TAssetResourceType, _TAssetRenderResourceType>>();
	}

	static void RegisterClassTypes()
	{
		RegisterClassType<ETexture, EObjectBase, EResourceInterface>();

		RegisterClassType<ETexture2D, ETexture>();
		RegisterClassType<ETextureCube, ETexture>();

		RegisterTextureClassTypes<ETexture2D, RTexture2DResource, ETexture2DAsset>();
		RegisterTextureClassTypes<ETextureCube, RTextureCubeResource, ETextureCubeAsset>();

		RegisterClassType<ETextureAssetManager, EManagerBase>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

#if _EDITOR_ONLY
	static BOOL32 ReadAndDecodingDigitalImage(const EString& InPathName, BYTE*& OutByteCode, UINT32& OutWidth, UINT32& OutHeight, UINT32& OutPixelByteCount, RFormatType& OutFormat)
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
		SRGB = FALSE;
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
		SRGB				= FALSE;
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
		return ((TextureType == ETextureType::TEXTURE_TYPE_TEXTURECUBE) && (ResourceProperties.Depth == 6u) && (ResourceProperties.Width == ResourceProperties.Height) && (SRGB == FALSE) && ETexture::IsResourceValid());
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
	void ETextureCube::SetTextureCubeData(BYTE* InByteCode, UINT32 InWidth, UINT32 InPixelByteCount, RFormatType InFormat)
	{
		SetTextureData(InByteCode, ETextureResourceProperty(InWidth, InWidth, 6u, InPixelByteCount, InFormat), FALSE);
	}

	ETexture2DAsset::ETexture2DAsset(const EString& InAssetPath, const EString& InAssetName
#if _EDITOR_ONLY
		, const EString& InDebugName
#endif
	) : TRenderBaseAsset<ETexture2D, RTexture2DResource>(InAssetPath, InAssetName
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
		if (CreateRenderResourceInternal(
			[this](ETexture2D* InResource)->RTexture2DResource*
			{
				return (this->CreateTextureResource(InResource));
			}, FALSE, [this](ETexture2D* InResource)->void
			{
				this->ReleaseResourceInternal();
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
		RDeviceD3D11* RenderDevice = RDeviceD3D11::GetDeviceSingleton();
		RSubresourceDataDesc TempInitData;
		{
			TempInitData.pSysMem = InResource->ByteCode;
			TempInitData.SysMemPitch = InResource->ResourceProperties.Width * InResource->ResourceProperties.PixelByteCount;
			TempInitData.SysMemSlicePitch = InResource->ResourceProperties.Width * InResource->ResourceProperties.PixelByteCount * InResource->ResourceProperties.Height;
		}
		Result = new RTexture2DResource();
		if (!(RenderDevice->CreateTexture2D(*Result,
			RTextureDesc(
				InResource->ResourceProperties.Width,
				InResource->ResourceProperties.Height,
				RBindFlagType::BIND_SHADER_RESOURCE,
				InResource->ResourceProperties.Format),
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
	) : TRenderBaseAsset<ETextureCube, RTextureCubeResource>(InAssetPath, InAssetName
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
		if (CreateRenderResourceInternal(
			[this](ETextureCube* InResource)->RTextureCubeResource*
			{
				return (this->CreateTextureResource(InResource));
			}, FALSE, [this](ETextureCube* InResource)->void
			{
				this->ReleaseResourceInternal();
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
		RDeviceD3D11* RenderDevice = RDeviceD3D11::GetDeviceSingleton();
		const UINT32	SingleFaceMemStride	= InResource->ResourceProperties.Width * InResource->ResourceProperties.PixelByteCount;
		const UINT32	SingleFaceMemSize	= SingleFaceMemStride * InResource->ResourceProperties.Height;
		//const UINT32	TotalFaceMemSize	= SingleFaceMemSize * 6u;
		BYTE*			StartPtr			= InResource->ByteCode;
		RSubresourceDataDesc TempInitData[6] =
		{
			{ &(StartPtr[0u * SingleFaceMemSize]), SingleFaceMemStride, SingleFaceMemSize},
			{ &(StartPtr[1u * SingleFaceMemSize]), SingleFaceMemStride, SingleFaceMemSize},
			{ &(StartPtr[2u * SingleFaceMemSize]), SingleFaceMemStride, SingleFaceMemSize},
			{ &(StartPtr[3u * SingleFaceMemSize]), SingleFaceMemStride, SingleFaceMemSize},
			{ &(StartPtr[4u * SingleFaceMemSize]), SingleFaceMemStride, SingleFaceMemSize},
			{ &(StartPtr[5u * SingleFaceMemSize]), SingleFaceMemStride, SingleFaceMemSize}
		};
		Result = new RTextureCubeResource();
		if (!(RenderDevice->CreateTextureCube(*Result,
			RTextureDesc(
				InResource->ResourceProperties.Width,
				InResource->ResourceProperties.Height,
				RBindFlagType::BIND_SHADER_RESOURCE,
				InResource->ResourceProperties.Format),
			TempInitData)))
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
		ClearTexture2Ds();
		ClearTextureCubes();
	}
#if _EDITOR_ONLY
	BOOL32 ETextureAssetManager::ImportTexture2D(const EString& InAssetName, const EString& InImportFullPathName, const EString& InSavePath)
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
		if (!ReadAndDecodingDigitalImage(InImportFullPathName, ReadByteCode, ReadWidth, ReadHeight, ReadPixelByteCount, ReadFormat))
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
		NeedSaveTexture2DResource->SetTexture2DData(ReadByteCode, ReadWidth, ReadHeight, ReadPixelByteCount, ReadFormat, FALSE);

		BOOL32 Result = SaveTexture2DAsset(InSavePath, InAssetName, NeedSaveTexture2DResource);

		NeedSaveTexture2DResource->ReleaseResource();
		delete NeedSaveTexture2DResource;
		return Result;
	}
	BOOL32 ETextureAssetManager::ImportTextureCube(const EString& InAssetName, const TArray<EString>& InImportFullPathName, const EString& InSavePath)
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
			if (!ReadAndDecodingDigitalImage(TempImportFullPathName, ReadByteCode[ImportIndex], ReadWidth[ImportIndex], ReadHeight[ImportIndex], ReadPixelByteCount[ImportIndex], ReadFormat[ImportIndex]))
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
				const UINT32		SingleMemSize		= TotalWidth * TotalPixelByteCount * TotalWidth;
				BYTE*				TotalReadByteCode	= new BYTE[SingleMemSize * TextureCubeRawAssetCount];
				for (UINT32 ImportIndex = 0u; ImportIndex < TextureCubeRawAssetCount; ImportIndex++)
				{
					::memcpy_s(&(TotalReadByteCode[SingleMemSize * ImportIndex]), SingleMemSize, ReadByteCode[ImportIndex], SingleMemSize);
				}

				ETextureCube* NeedSaveTextureCubeResource = new ETextureCube();
				NeedSaveTextureCubeResource->SetTextureCubeData(TotalReadByteCode, TotalWidth, TotalPixelByteCount, TotalFormat);

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
	BOOL32 ETextureAssetManager::LoadTexture2DAsset(const EString& InLoadPath, const EString& InLoadName, const ETexture2DAsset*& OutTextureAsset)
	{
		EString TempFullPathName(InLoadPath);
		TempFullPathName = TempFullPathName + InLoadName + EEngineSettings::ENGINE_ASSET_NAME_TYPE;
		ETexture2DAsset* ResultAsset = Texture2DManager.Find(TempFullPathName);
		if (ResultAsset)
		{
			OutTextureAsset = ResultAsset;
			return TRUE;
		}
		ResultAsset = LoadTexture2DAsset(InLoadPath, InLoadName);
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
	BOOL32 ETextureAssetManager::LoadTextureCubeAsset(const EString& InLoadPath, const EString& InLoadName, const ETextureCubeAsset*& OutTextureAsset)
	{
		EString TempFullPathName(InLoadPath);
		TempFullPathName = TempFullPathName + InLoadName + EEngineSettings::ENGINE_ASSET_NAME_TYPE;
		ETextureCubeAsset* ResultAsset = TextureCubeManager.Find(TempFullPathName);
		if (ResultAsset)
		{
			OutTextureAsset = ResultAsset;
			return TRUE;
		}
		ResultAsset = LoadTextureCubeAsset(InLoadPath, InLoadName);
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
	void ETextureAssetManager::ClearTexture2Ds()
	{
		Texture2DManager.Clear();
	}
	void ETextureAssetManager::ClearTextureCubes()
	{
		TextureCubeManager.Clear();
	}
	ETexture2DAsset* ETextureAssetManager::LoadTexture2DAsset(const EString& InLoadPath, const EString& InLoadName, const BOOL32* InSRGBOverride)
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
				PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorData));
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
	ETextureCubeAsset* ETextureAssetManager::LoadTextureCubeAsset(const EString& InLoadPath, const EString& InLoadName)
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
				PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorData));
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
					StoragedResource->SetTextureCubeData(ReadByteCode, ReadResourceProperty.Width, ReadResourceProperty.PixelByteCount, ReadResourceProperty.Format);
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

	void TryLoadTexture2D(const EString& InLoadPath, const EString& InLoadName, const ETexture2DAsset*& OutTextureAsset, const EString* InImportPath, const EString* InImportName, const EString* InImportFileType)
	{
		ETextureAssetManager* TextureAssetManager = ETextureAssetManager::GetManagerSingleton();
		PE_CHECK((ENGINE_ASSET_ERROR), ("Check output texture asset pointer is initialized failed."), (!OutTextureAsset));
		if (TextureAssetManager->LoadTexture2DAsset(InLoadPath, InLoadName, OutTextureAsset))
		{
			return;
		}
		PE_FAILED((ENGINE_ASSET_ERROR), ("Load texture 2d asset failed."));
#if _EDITOR_ONLY
		if ((!!InImportPath) && (!!InImportName) && (!!InImportFileType))
		{
			EString TempImportFullPath(*InImportPath);
			TempImportFullPath = TempImportFullPath + (*InImportName) + (".") + (*InImportFileType);
			BOOL32 Result = TextureAssetManager->ImportTexture2D(
				InLoadName,
				TempImportFullPath,
				InLoadPath);
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
#endif
	}
	void TryLoadTextureCube(const EString& InLoadPath, const EString& InLoadName, const ETextureCubeAsset*& OutTextureAsset, const TArray<EString>* InImportPath, const TArray<EString>* InImportName, const TArray<EString>* InImportFileType)
	{
		ETextureAssetManager* TextureAssetManager = ETextureAssetManager::GetManagerSingleton();
		PE_CHECK((ENGINE_ASSET_ERROR), ("Check output texture asset pointer is initialized failed."), (!OutTextureAsset));
		if (TextureAssetManager->LoadTextureCubeAsset(InLoadPath, InLoadName, OutTextureAsset))
		{
			return;
		}
		PE_FAILED((ENGINE_ASSET_ERROR), ("Load texture cube asset failed."));
#if _EDITOR_ONLY
		if ((!!InImportPath) && (!!InImportName) && (!!InImportFileType))
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
					InLoadPath);
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
#endif
	}

};