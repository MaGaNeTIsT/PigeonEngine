#include "TextureAsset.h"
#include <RenderDevice/DeviceD3D11.h>

namespace PigeonEngine
{

	template <typename _TAssetResourceType, typename _TAssetRenderResourceType, typename _AssetType>
	static void RegisterTextureClassTypes()
	{
		RegisterClassType<TBaseAsset<_TAssetResourceType>, EObjectBase>();
		RegisterClassType<TRenderBaseAsset<_TAssetResourceType, _TAssetRenderResourceType>, TBaseAsset<_TAssetResourceType>>();
		RegisterClassType<_AssetType, TRenderBaseAsset<_TAssetResourceType, _TAssetRenderResourceType>>();
	}

	static void RegisterClassTypes()
	{
		RegisterClassType<ETexture2D, EObjectBase, EResourceInterface>();

		RegisterTextureClassTypes<ETexture2D, RTexture2DResource, ETexture2DAsset>();

		RegisterClassType<ETextureAssetManager, EManagerBase>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

#if _EDITOR_ONLY
	static BOOL ReadAndDecodingDigitalImage(const EString& InPathName, BYTE*& OutByteCode, UINT& OutWidth, UINT& OutHeight, UINT& OutPixelByteCount, RFormatType& OutFormat)
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
				PE_FAILED((ENGINE_ASSET_ERROR), ("Try to load texture that pixel format is not supported."));
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
							if (NeedSwitchRB)
							{
								if ((RenderCoord + 2u) < RenderPixelByteSize)
								{
									BYTE TempValue = OutputRenderPixelData[RenderCoord + 0u];
									OutputRenderPixelData[RenderCoord + 0u] = OutputRenderPixelData[RenderCoord + 2u];
									OutputRenderPixelData[RenderCoord + 2u] = TempValue;
								}
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
					OutFormat = RFormatType::FORMAT_UNKNOWN;
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

	ETexture2D::ETexture2D()
		: TextureType(ETextureType::TEXTURE_TYPE_TEXTURE2D)
		, ByteCode(nullptr)
		, ResourceProperties()
		, SRGB(FALSE)
	{
	}
	ETexture2D::ETexture2D(const ETexture2D& Other)
		: TextureType(ETextureType::TEXTURE_TYPE_TEXTURE2D)
		, ByteCode(nullptr)
		, ResourceProperties(Other.ResourceProperties)
		, SRGB(Other.SRGB)
	{
		ResourceProperties.Depth = 0u;
		if ((!!(Other.ByteCode)) && (Other.ResourceProperties.Width > 0u) && (Other.ResourceProperties.Height > 0u) && (Other.ResourceProperties.PixelByteCount > 0u) && (Other.ResourceProperties.Format != RFormatType::FORMAT_UNKNOWN))
		{
			const UINT DataStride = Other.ResourceProperties.Width * Other.ResourceProperties.PixelByteCount;
			const UINT DataSize = DataStride * Other.ResourceProperties.Height;
			ByteCode = new BYTE[DataSize];
			::memcpy_s(ByteCode, DataSize, Other.ByteCode, DataSize);
		}
	}
	ETexture2D::~ETexture2D()
	{
		ReleaseResource();
	}
	ETexture2D& ETexture2D::operator=(const ETexture2D& Other)
	{
		TextureType = ETextureType::TEXTURE_TYPE_TEXTURE2D;
		if (ByteCode)
		{
			delete[]ByteCode;
			ByteCode = nullptr;
		}
		ResourceProperties	= Other.ResourceProperties;
		SRGB				= Other.SRGB;
		ResourceProperties.Depth = 0u;
		if ((!!(Other.ByteCode)) && (Other.ResourceProperties.Width > 0u) && (Other.ResourceProperties.Height > 0u) && (Other.ResourceProperties.PixelByteCount > 0u) && (Other.ResourceProperties.Format != RFormatType::FORMAT_UNKNOWN))
		{
			const UINT DataStride = Other.ResourceProperties.Width * Other.ResourceProperties.PixelByteCount;
			const UINT DataSize = DataStride * Other.ResourceProperties.Height;
			ByteCode = new BYTE[DataSize];
			::memcpy_s(ByteCode, DataSize, Other.ByteCode, DataSize);
		}
		return (*this);
	}
	BOOL ETexture2D::IsResourceValid()const
	{
		return ((!!ByteCode) && (ResourceProperties.Width > 0u) && (ResourceProperties.Height > 0u) && (ResourceProperties.PixelByteCount > 0u) && (ResourceProperties.Format != RFormatType::FORMAT_UNKNOWN));
	}
	BOOL ETexture2D::InitResource()
	{
		// Texture2D resource must init by texture manager.
		return TRUE;
	}
	void ETexture2D::ReleaseResource()
	{
		ResourceProperties = ETextureResourceProperty();
		SRGB = FALSE;
		if (ByteCode)
		{
			delete[]ByteCode;
			ByteCode = nullptr;
		}
	}
	void ETexture2D::SetData(BYTE* InByteCode, UINT InWidth, UINT InHeigh, UINT InPixelByteCount, RFormatType InFormat, BOOL InSRGB)
	{
		TextureType = ETextureType::TEXTURE_TYPE_TEXTURE2D;
		if (ByteCode)
		{
			delete[]ByteCode;
			ByteCode = nullptr;
		}
		ByteCode			= InByteCode;
		ResourceProperties	= ETextureResourceProperty(InWidth, InHeigh, 0u, InPixelByteCount, InFormat);
		SRGB				= InSRGB;
	}

	ETexture2DAsset::ETexture2DAsset(
		const EString& InPath
#if _EDITOR_ONLY
		, const EString& InDebugName
#endif
	) : TRenderBaseAsset<ETexture2D, RTexture2DResource>(
#if _EDITOR_ONLY
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
		if (IsInitialized())
		{
#if _EDITOR_ONLY
			{
				EString ErrorInfo = EString("Texture2D name=[") + DebugName + "] path = [" + TexturePath + "] has been Initialized.";
				PE_FAILED((ENGINE_ASSET_ERROR), (ErrorInfo));
			}
#endif
			return TRUE;
		}
		if (CreateRenderResourceInternal(
			[this](ETexture2D* InResource)->RTexture2DResource*
			{
				return (this->CreateTextureResource(InResource));
			},
			FALSE))
		{
			bIsInitialized = TRUE;
			return TRUE;
		}
		return FALSE;
	}
	const EString& ETexture2DAsset::GetTexturePath()const
	{
		return TexturePath;
	}
	RTexture2DResource* ETexture2DAsset::CreateTextureResource(ETexture2D* InResource)
	{
		RTexture2DResource* Result = nullptr;
		if ((!!InResource) && (InResource->IsResourceValid()))
		{
			RDeviceD3D11* deviceD3D11 = RDeviceD3D11::GetDeviceSingleton();
			RSubresourceDataDesc TempInitData;
			{
				TempInitData.pSysMem = InResource->ByteCode;
				TempInitData.SysMemPitch = InResource->ResourceProperties.Width * InResource->ResourceProperties.PixelByteCount;
				TempInitData.SysMemSlicePitch = InResource->ResourceProperties.Width * InResource->ResourceProperties.PixelByteCount * InResource->ResourceProperties.Height;
			}
			Result = new RTexture2DResource();
			if (!deviceD3D11->CreateTexture2D(*Result, RTextureDesc(InResource->ResourceProperties.Width, InResource->ResourceProperties.Height, RBindFlagType::BIND_SHADER_RESOURCE, InResource->ResourceProperties.Format), &TempInitData))
			{
				delete Result;
				Result = nullptr;
			}
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
	}
#if _EDITOR_ONLY
	BOOL ETextureAssetManager::ImportTexture2D(const EString& InImportPath, const EString& InSaveAssetPath)
	{
		if ((InImportPath.Length() < 3u) || (InSaveAssetPath.Length() < 3u))
		{
#if _EDITOR_ONLY
			{
				EString ErrorData("Error file path for texture 2d importer (import file path : ");
				ErrorData = ErrorData + InImportPath + ", save assset path : " + InSaveAssetPath + ").";
				PE_FAILED((ENGINE_ASSET_ERROR), (ErrorData));
			}
#endif
			return FALSE;
		}
		BYTE* ReadByteCode = nullptr; UINT ReadWidth = 0u, ReadHeight = 0u, ReadPixelByteCount = 0u; RFormatType ReadFormat = RFormatType::FORMAT_UNKNOWN;
		if (!ReadAndDecodingDigitalImage(InImportPath, ReadByteCode, ReadWidth, ReadHeight, ReadPixelByteCount, ReadFormat))
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
		NeedSaveTexture2DResource->SetData(ReadByteCode, ReadWidth, ReadHeight, ReadPixelByteCount, ReadFormat, FALSE);

		BOOL Result = SaveTexture2DAsset(InSaveAssetPath, NeedSaveTexture2DResource);

		NeedSaveTexture2DResource->ReleaseResource();
		delete NeedSaveTexture2DResource;
		return Result;
	}
#endif
	BOOL ETextureAssetManager::LoadTexture2DAsset(const EString& InLoadPath, const ETexture2DAsset*& OutTextureAsset)
	{
		ETexture2DAsset* ResultAsset = Texture2DManager.Find(InLoadPath);
		if (ResultAsset)
		{
			OutTextureAsset = ResultAsset;
			return TRUE;
		}
		ResultAsset = LoadTexture2DAsset(InLoadPath);
		if (!ResultAsset)
		{
			return FALSE;
		}
		if (!ResultAsset->InitResource())
		{
			delete ResultAsset;
			return FALSE;
		}
		if (Texture2DManager.Add(InLoadPath, ResultAsset, TRUE) == 0u)
		{
#if _EDITOR_ONLY
			{
				EString ErrorInfo = EString("Texture2D path = [") + InLoadPath + "] add into manager list failed.";
				PE_FAILED((ENGINE_ASSET_ERROR), (ErrorInfo));
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
	ETexture2DAsset* ETextureAssetManager::LoadTexture2DAsset(const EString& InLoadPath, const BOOL* InSRGBOverride)
	{
		ETexture2DAsset* OutTextureAsset = nullptr;
		void* ReadFileMem = nullptr; ULONG ReadFileSize = 0u;
		if (!EFileHelper::ReadFileAsBinary(InLoadPath, ReadFileMem, ReadFileSize))
		{
			if (ReadFileMem)
			{
				delete[]ReadFileMem;
			}
#if _EDITOR_ONLY
			{
				EString ErrorData("Load shader asset failed (load file path : ");
				ErrorData += InLoadPath;
				ErrorData += ").";
				PE_FAILED((ENGINE_ASSET_ERROR), (ErrorData));
			}
#endif
			return FALSE;
		}
		Check((ENGINE_ASSET_ERROR), ("Error read shader asset file size are too small."), (ReadFileSize > (sizeof(UINT32) * 2u + sizeof(BOOL) + sizeof(ETextureResourceProperty))));
		BOOL Result = FALSE;
		void* TempPtr = ReadFileMem;
		ULONG RstSize = ReadFileSize;
		EAssetType ReadAssetType = EAssetType::ASSET_TYPE_UNKNOWN;
		ETextureType ReadTextureType = ETextureType::TEXTURE_TYPE_UNKNOWN;
		BOOL ReadSRGB = FALSE;
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
			BOOL* SavedAssetTypePtr = (BOOL*)TempPtr;
			ReadSRGB = SavedAssetTypePtr[0];
			TempPtr = (void*)(&(SavedAssetTypePtr[1]));
			RstSize = RstSize - sizeof(BOOL);
		}
		{
			ETextureResourceProperty* SavedAssetTypePtr = (ETextureResourceProperty*)TempPtr;
			ReadResourceProperty = SavedAssetTypePtr[0];
			TempPtr = (void*)(&(SavedAssetTypePtr[1]));
			RstSize = RstSize - sizeof(ETextureResourceProperty);
		}
		if ((ReadAssetType == EAssetType::ASSET_TYPE_TEXTURE) &&
			(ReadTextureType == ETextureType::TEXTURE_TYPE_TEXTURE2D) &&
			(ReadResourceProperty.Width > 0u) &&
			(ReadResourceProperty.Height > 0u) &&
			(ReadResourceProperty.PixelByteCount > 0u) &&
			(ReadResourceProperty.Format != RFormatType::FORMAT_UNKNOWN))
		{
			const UINT ByteCodeStride = ReadResourceProperty.Width * ReadResourceProperty.PixelByteCount;
			const UINT ByteCodeSize = ByteCodeStride * ReadResourceProperty.Height;
			BYTE* ReadByteCode = new BYTE[ByteCodeSize];
			::memcpy_s(ReadByteCode, ByteCodeSize, TempPtr, RstSize);
			if (InSRGBOverride)
			{
				ReadSRGB = (*InSRGBOverride);
			}
			OutTextureAsset = new ETexture2DAsset(InLoadPath
#if _EDITOR_ONLY
				, InLoadPath
#endif
			);
			ETexture2D* StoragedResource = nullptr;
			if (!(OutTextureAsset->StorageResourceInternal(
				[ReadSRGB, ReadByteCode, &ReadResourceProperty, &StoragedResource]()->ETexture2D*
				{
					StoragedResource = new ETexture2D();
					StoragedResource->SetData(ReadByteCode, ReadResourceProperty.Width, ReadResourceProperty.Height, ReadResourceProperty.PixelByteCount, ReadResourceProperty.Format, ReadSRGB);
					return StoragedResource;
				})))
			{
				if (StoragedResource)
				{
					StoragedResource->ReleaseResource();
					delete StoragedResource;
				}
			}
		}
		delete[]ReadFileMem;
		return OutTextureAsset;
	}
	BOOL ETextureAssetManager::SaveTexture2DAsset(const EString& InSavePath, const ETexture2D* InTextureResource)
	{
		if ((InSavePath.Length() < 3u) || (!InTextureResource) || (!(InTextureResource->IsResourceValid())))
		{
#if _EDITOR_ONLY
			{
				EString ErrorData("Saving texture 2d asset error.(import file path : ");
				ErrorData = ErrorData + InSavePath + ", is asset valid : " +
					((!!InTextureResource) ? ((InTextureResource->IsResourceValid()) ? "true" : "false") : "false")
					+ ").";
				PE_FAILED((ENGINE_ASSET_ERROR), (ErrorData));
			}
#endif
			return FALSE;
		}

		const UINT ByteCodeStride = InTextureResource->ResourceProperties.Width * InTextureResource->ResourceProperties.PixelByteCount;
		const UINT ByteCodeSize = ByteCodeStride * InTextureResource->ResourceProperties.Height;
		ULONG NeedSavedSize = sizeof(UINT32) + sizeof(UINT32) + sizeof(ETextureResourceProperty) + sizeof(BOOL) + ByteCodeSize;	// EAssetType + TextureType + ETextureResourceProperty + SRGB + ByteCodeSize
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
			BOOL* SavedAssetTypePtr = (BOOL*)TempPtr;
			SavedAssetTypePtr[0] = InTextureResource->SRGB;
			TempPtr = (void*)(&(SavedAssetTypePtr[1]));
		}
		{
			ETextureResourceProperty* SavedAssetTypePtr = (ETextureResourceProperty*)TempPtr;
			SavedAssetTypePtr[0] = InTextureResource->ResourceProperties;
			TempPtr = (void*)(&(SavedAssetTypePtr[1]));
		}
		::memcpy_s(TempPtr, ByteCodeSize, InTextureResource->ByteCode, ByteCodeSize);

		BOOL Result = EFileHelper::SaveBytesToFile(InSavePath, SavedMem, NeedSavedSize);

		delete[]SavedMem;
		return Result;
	}

};