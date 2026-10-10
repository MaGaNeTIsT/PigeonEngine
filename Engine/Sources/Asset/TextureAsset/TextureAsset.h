#pragma once

#include <CoreMinimal.h>
#include <RenderCommon.h>
#include <RenderResource.h>
#include <EngineCommon.h>
#include <BaseAsset.h>
#include <AssetLoadManager.h>
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
		// InSRGB states whether the stored bytes are sRGB encoded, exactly as it does for a 2D
		// texture: the render layer decodes through the SRV, the stored bytes are never converted.
		void			SetTextureCubeData(BYTE* InByteCode, UINT32 InWidth, UINT32 InPixelByteCount, RFormatType InFormat, BOOL32 InSRGB);
	public:
		friend class ETextureCubeAsset;
		friend class ETextureAssetManager;
	public:

		CLASS_VIRTUAL_COPY_BODY(ETextureCube)
	};

	/// <summary>
	/// A texture type that contains only 1 member. Shader resource view is texture2d.
	/// </summary>
	class ETexture2DAsset : public TBaseAsset<ETexture2D>
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
	public:
		// Render thread only: the proxy this asset's cold data was turned into, or null while
		// the creation command is still queued.
		PE_NODISCARD const RTexture2DResource*	GetRenderResource()const
		{
			return static_cast<const RTexture2DResource*>(RenderBinding.Resolve());
		}
		PE_NODISCARD BOOL32	IsRenderResourceValid()const
		{
			const RTexture2DResource* Resource = GetRenderResource();
			return ((!!Resource) && (Resource->IsRenderResourceValid()));
		}
		virtual void	UninitResource()override
		{
			TBaseAsset<ETexture2D>::UninitResource();
			RenderBinding.Release();
		}
	protected:
		RTexture2DResource*	CreateTextureResource(ETexture2D* InResource);
	private:
		// The single connection between this asset and its render side proxy.
		RAssetRenderBinding	RenderBinding;
	public:
		ETexture2DAsset() = delete;

		CLASS_REMOVE_COPY_BODY(ETexture2DAsset)

	};

	/// <summary>
	/// A texture array type that contains 6 members. Shader resource view is texture cube.
	/// </summary>
	class ETextureCubeAsset : public TBaseAsset<ETextureCube>
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
	public:
		// Render thread only: the proxy this asset's cold data was turned into, or null while
		// the creation command is still queued.
		PE_NODISCARD const RTextureCubeResource*	GetRenderResource()const
		{
			return static_cast<const RTextureCubeResource*>(RenderBinding.Resolve());
		}
		PE_NODISCARD BOOL32	IsRenderResourceValid()const
		{
			const RTextureCubeResource* Resource = GetRenderResource();
			return ((!!Resource) && (Resource->IsRenderResourceValid()));
		}
		virtual void	UninitResource()override
		{
			TBaseAsset<ETextureCube>::UninitResource();
			RenderBinding.Release();
		}
	protected:
		RTextureCubeResource* CreateTextureResource(ETextureCube* InResource);
	private:
		// The single connection between this asset and its render side proxy.
		RAssetRenderBinding	RenderBinding;
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
		virtual void	Uninit()override;
	public:
#if _EDITOR_ONLY
		// InFallbackSRGB answers for an image that declares no colour space of its own (TGA and
		// other formats carry none). A file that does declare one - an sRGB profile, or an Exif
		// colour space of sRGB - decides by itself, and the fallback is not consulted. The value
		// is stored in the asset, so an import can be redone with the other answer.
		BOOL32	ImportTexture2D(const EString& InAssetName, const EString& InImportFullPathName, const EString& InSavePath, BOOL32 InFallbackSRGB = FALSE);

		/// <summary>
		/// Face[0] : +X Right face.
		/// Face[1] : -X Left face.
		/// Face[2] : +Y Top face.
		/// Face[3] : -Y Bottom face.
		/// Face[4] : +Z Forward face.
		/// Face[5] : -Z Back face.
		/// </summary>
		BOOL32	ImportTextureCube(const EString& InAssetName, const TArray<EString>& InImportFullPathName, const EString& InSavePath, BOOL32 InFallbackSRGB = FALSE);
#endif
	public:
		BOOL32	LoadTexture2DAsset(const EString& InLoadPath, const EString& InLoadName, const ETexture2DAsset*& OutTextureAsset, const BOOL32 bTryLoad = FALSE);
		BOOL32	LoadTextureCubeAsset(const EString& InLoadPath, const EString& InLoadName, const ETextureCubeAsset*& OutTextureAsset, const BOOL32 bTryLoad = FALSE);
		/*
		* Any thread: the texture when it is already loaded, otherwise FALSE and a null output
		* after recording one load request for the load thread (EAssetLoadManager). It never
		* blocks and never reads the disk on the calling thread, which is what lets a frame, a
		* draw or an editor panel ask for a texture without waiting for it; ask again next frame
		* and it will be there. Load* above is the load thread's synchronous half.
		*
		* InImportSource is the editor's fallback for an image that was never imported: the load
		* thread imports from the raw source and loads again, which is the pair TryLoad* performs.
		* InOnLoadComplete runs on the load thread once the request is done; see
		* EAssetLoadCompleteType for what a completion may touch.
		*/
		BOOL32	RequireLoadTexture2DAsset(const EString& InLoadPath, const EString& InLoadName, const ETexture2DAsset*& OutTextureAsset
			, const FAssetImportSource* InImportSource = nullptr, EAssetLoadCompleteType InOnLoadComplete = nullptr);
		BOOL32	RequireLoadTextureCubeAsset(const EString& InLoadPath, const EString& InLoadName, const ETextureCubeAsset*& OutTextureCubeAsset
			, const FAssetImportCubeSource* InImportSource = nullptr, EAssetLoadCompleteType InOnLoadComplete = nullptr);
	private:
		void	ClearTexture2Ds();
		void	ClearTextureCubes();
		// Releases every asset this manager caches. Shared by ShutDown and Uninit so the two
		// paths cannot drift apart.
		void	ReleaseAllAssets();
	private:
		ETexture2DAsset* LoadTexture2DAsset(const EString& InLoadPath, const EString& InLoadName, const BOOL32* InSRGBOverride = nullptr, const BOOL32 bTryLoad = FALSE);
		BOOL32 SaveTexture2DAsset(const EString& InSavePath, const EString& InSaveName, const ETexture2D* InTextureResource);
		ETextureCubeAsset* LoadTextureCubeAsset(const EString& InLoadPath, const EString& InLoadName, const BOOL32 bTryLoad = FALSE);
		BOOL32 SaveTextureCubeAsset(const EString& InSavePath, const EString& InSaveName, const ETextureCube* InTextureResource);
	private:
		ETexture2DAssetManager		Texture2DManager;
		ETextureCubeAssetManager	TextureCubeManager;

		CLASS_MANAGER_VIRTUAL_SINGLETON_BODY(ETextureAssetManager)

	};

	/// InFallbackSRGB is handed to the import that runs when the asset is not on disk yet; it
	/// answers only for an image that declares no colour space of its own.
	extern void TryLoadTexture2D(const EString& InLoadPath, const EString& InLoadName,
		const ETexture2DAsset*& OutTextureAsset,
		const EString* InImportPath = nullptr, const EString* InImportName = nullptr, const EString* InImportFileType = nullptr,
		BOOL32 InFallbackSRGB = FALSE);

	/// <summary>
	/// Face[0] : +X Right face.
	/// Face[1] : -X Left face.
	/// Face[2] : +Y Top face.
	/// Face[3] : -Y Bottom face.
	/// Face[4] : +Z Forward face.
	/// Face[5] : -Z Back face.
	/// </summary>
	extern void TryLoadTextureCube(const EString& InLoadPath, const EString& InLoadName,
		const ETextureCubeAsset*& OutTextureAsset,
		const TArray<EString>* InImportPath = nullptr, const TArray<EString>* InImportName = nullptr, const TArray<EString>* InImportFileType = nullptr,
		BOOL32 InFallbackSRGB = FALSE);

};
