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
		TEXTURE_TYPE_UNKNOWN		= 0,
		TEXTURE_TYPE_TEXTURE2D,
		TEXTURE_TYPE_TEXTURECUBE,
		TEXTURE_TYPE_COUNT
	};

	struct ETextureResourceProperty
	{
		ETextureResourceProperty()noexcept : Width(0u), Height(0u), Depth(0u), PixelByteCount(0u), Format(RFormatType::FORMAT_UNKNOWN) {}
		ETextureResourceProperty(const ETextureResourceProperty& Other)noexcept : Width(Other.Width), Height(Other.Height), Depth(Other.Depth), PixelByteCount(Other.PixelByteCount), Format(Other.Format) {}
		ETextureResourceProperty(UINT32 InWidth, UINT32 InHeight, UINT32 InDepth, UINT32 InPixelByteCount, RFormatType InFormat)noexcept : Width(InWidth), Height(InHeight), Depth(InDepth), PixelByteCount(InPixelByteCount), Format(InFormat) {}
		ETextureResourceProperty& operator=(const ETextureResourceProperty& Other)
		{
			Width			= Other.Width;
			Height			= Other.Height;
			Depth			= Other.Depth;
			PixelByteCount	= Other.PixelByteCount;
			Format			= Other.Format;
			return (*this);
		}

		UINT32			Width;
		UINT32			Height;
		UINT32			Depth;
		UINT32			PixelByteCount;
		RFormatType		Format;
	};

	class ETexture : public EObjectBase, public EResourceInterface
	{
	public:
		virtual BOOL32	IsResourceValid()const override;
		virtual BOOL32	InitResource()override;
		virtual void	ReleaseResource()override;
	public:
		ETextureType	GetTextureType()const { return TextureType; }
	protected:
		void			SetTextureData(BYTE* InByteCode, const ETextureResourceProperty& InProperty, BOOL32 InSRGB);
	public:
		ETextureType				TextureType;
		BYTE*						ByteCode;
		ETextureResourceProperty	ResourceProperties;
		BOOL32						SRGB;
	public:
		ETexture();
		ETexture(const ETexture& Other);
		virtual ~ETexture();
		ETexture& operator=(const ETexture& Other) = delete;
	public:
		friend class ETextureAssetManager;
	};

	class ETexture2D : public ETexture
	{
	public:
		virtual BOOL32	IsResourceValid()const override;
		virtual BOOL32	InitResource()override;
		virtual void	ReleaseResource()override;
	public:
		void			SetTexture2DData(BYTE* InByteCode, UINT32 InWidth, UINT32 InHeigh, UINT32 InPixelByteCount, RFormatType InFormat, BOOL32 InSRGB);
	public:
		friend class ETexture2DAsset;
		friend class ETextureAssetManager;
	public:

		CLASS_VIRTUAL_COPY_BODY(ETexture2D)

	};

	class ETextureCube : public ETexture
	{
	public:
		virtual BOOL32	IsResourceValid()const override;
		virtual BOOL32	InitResource()override;
		virtual void	ReleaseResource()override;
	public:
		void			SetTextureCubeData(BYTE* InByteCode, UINT32 InWidth, UINT32 InPixelByteCount, RFormatType InFormat);
	public:
		friend class ETextureCubeAsset;
		friend class ETextureAssetManager;
	public:

		CLASS_VIRTUAL_COPY_BODY(ETextureCube)
	};

	/// <summary>
	/// A texture type that contains only 1 member. Shader resource view is texture2d.
	/// </summary>
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
		virtual BOOL32	InitResource()override;
	protected:
		RTexture2DResource*	CreateTextureResource(ETexture2D* InResource);
	public:
		ETexture2DAsset() = delete;

		CLASS_REMOVE_COPY_BODY(ETexture2DAsset)

	};

	/// <summary>
	/// A texture array type that contains 6 members. Shader resource view is texture cube.
	/// Face[0] : +X Right face.
	/// Face[1] : -X Left face.
	/// Face[2] : +Y Top face.
	/// Face[3] : -Y Bottom face.
	/// Face[4] : +Z Forward face.
	/// Face[5] : -Z Back face.
	/// </summary>
	class ETextureCubeAsset : public TRenderBaseAsset<ETextureCube, RTextureCubeResource>
	{
	public:
		ETextureCubeAsset(const EString& InAssetPath, const EString& InAssetName
#if _EDITOR_ONLY
			, const EString& InDebugName
#endif
		);
		virtual ~ETextureCubeAsset();
	public:
		virtual BOOL32	InitResource()override;
	protected:
		RTextureCubeResource* CreateTextureResource(ETextureCube* InResource);
	public:
		ETextureCubeAsset() = delete;

		CLASS_REMOVE_COPY_BODY(ETextureCubeAsset)
	};

	class ETextureAssetManager : public EManagerBase
	{
	public:
		typedef TAssetManager<EString, ETexture2DAsset>		ETexture2DAssetManager;
		typedef TAssetManager<EString, ETextureCubeAsset>	ETextureCubeAssetManager;
	public:
		virtual void	Initialize()override;
		virtual void	ShutDown()override;
	public:
#if _EDITOR_ONLY
		BOOL32	ImportTexture2D(const EString& InAssetName, const EString& InImportFullPathName, const EString& InSavePath);
#endif
		BOOL32	LoadTexture2DAsset(const EString& InLoadPath, const EString& InLoadName, const ETexture2DAsset*& OutTextureAsset);
	private:
		void	ClearTexture2Ds();
		void	ClearTextureCubes();
	private:
		ETexture2DAsset* LoadTexture2DAsset(const EString& InLoadPath, const EString& InLoadName, const BOOL32* InSRGBOverride = nullptr);
		BOOL32 SaveTexture2DAsset(const EString& InSavePath, const EString& InSaveName, const ETexture2D* InTextureResource);
		ETextureCubeAsset* LoadTextureCubeAsset(const EString& InLoadPath, const EString& InLoadName);
		BOOL32 SaveTextureCubeAsset(const EString& InSavePath, const EString& InSaveName, const ETextureCube* InTextureResource);
	private:
		ETexture2DAssetManager		Texture2DManager;
		ETextureCubeAssetManager	TextureCubeManager;

		CLASS_MANAGER_VIRTUAL_SINGLETON_BODY(ETextureAssetManager)

	};

};