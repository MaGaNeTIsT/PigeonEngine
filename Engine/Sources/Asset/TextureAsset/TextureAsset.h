#pragma once

#include <CoreMinimal.h>
#include <RenderCommon.h>
#include <RenderResource.h>
#include <EngineCommon.h>
#include <BaseAsset.h>
#if _EDITOR_ONLY
#include <wincodec.h>
#include <wincodecsdk.h>
#pragma comment (lib, "windowscodecs.lib")
#endif

namespace PigeonEngine
{

	enum ETextureType : UINT8
	{
		TEXTURE_TYPE_UNKNOWN	= 0,
		TEXTURE_TYPE_TEXTURE2D,
		TEXTURE_TYPE_COUNT
	};

	struct ETextureResourceProperty
	{
		ETextureResourceProperty()noexcept : Width(0u), Height(0u), Depth(0u), PixelByteCount(0u), Format(RFormatType::FORMAT_UNKNOWN) {}
		ETextureResourceProperty(const ETextureResourceProperty& Other)noexcept : Width(Other.Width), Height(Other.Height), Depth(Other.Depth), PixelByteCount(Other.PixelByteCount), Format(Other.Format) {}
		ETextureResourceProperty(UINT InWidth, UINT InHeight, UINT InDepth, UINT InPixelByteCount, RFormatType InFormat)noexcept : Width(InWidth), Height(InHeight), Depth(InDepth), PixelByteCount(InPixelByteCount), Format(InFormat) {}
		ETextureResourceProperty& operator=(const ETextureResourceProperty& Other)
		{
			Width			= Other.Width;
			Height			= Other.Height;
			Depth			= Other.Depth;
			PixelByteCount	= Other.PixelByteCount;
			Format			= Other.Format;
			return (*this);
		}

		UINT			Width;
		UINT			Height;
		UINT			Depth;
		UINT			PixelByteCount;
		RFormatType		Format;
	};

	class ETexture2D : public EObjectBase, public EResourceInterface
	{
	public:
		virtual BOOL	IsResourceValid()const override;
		virtual BOOL	InitResource()override;
		virtual void	ReleaseResource()override;
	public:
		ETextureType	GetTextureType()const { return TextureType; }
		void			SetData(BYTE* InByteCode, UINT InWidth, UINT InHeigh, UINT InPixelByteCount, RFormatType InFormat, BOOL InSRGB);
	protected:
		ETextureType				TextureType;
		BYTE*						ByteCode;
		ETextureResourceProperty	ResourceProperties;
		BOOL						SRGB;
	public:
		friend class ETexture2DAsset;
		friend class ETextureAssetManager;
	public:

		CLASS_VIRTUAL_COPY_BODY(ETexture2D)

	};

	class ETexture2DAsset : public TRenderBaseAsset<ETexture2D, RTexture2DResource>
	{
	public:
		ETexture2DAsset(const EString& InAssetPath, const EString& InAssetName
#if _EDITOR_ONLY
			, const EString& InDebugName
#endif
		);
		virtual ~ETexture2DAsset();
	public:
		virtual BOOL	InitResource()override;
	protected:
		RTexture2DResource*	CreateTextureResource(ETexture2D* InResource);
	public:
		ETexture2DAsset() = delete;

		CLASS_REMOVE_COPY_BODY(ETexture2DAsset)

	};

	class ETextureAssetManager : public EManagerBase
	{
	public:
		typedef TAssetManager<EString, ETexture2DAsset>		ETexture2DAssetManager;
	public:
		virtual void	Initialize()override;
		virtual void	ShutDown()override;
	public:
#if _EDITOR_ONLY
		BOOL	ImportTexture2D(const EString& InAssetName, const EString& InImportFullPathName, const EString& InSavePath);
#endif
		BOOL	LoadTexture2DAsset(const EString& InLoadPath, const EString& InLoadName, const ETexture2DAsset*& OutTextureAsset);
	private:
		void	ClearTexture2Ds();
	private:
		ETexture2DAsset* LoadTexture2DAsset(const EString& InLoadPath, const EString& InLoadName, const BOOL* InSRGBOverride = nullptr);
		BOOL SaveTexture2DAsset(const EString& InSavePath, const EString& InSaveName, const ETexture2D* InTextureResource);
	private:
		ETexture2DAssetManager	Texture2DManager;

		CLASS_MANAGER_VIRTUAL_SINGLETON_BODY(ETextureAssetManager)

	};

};