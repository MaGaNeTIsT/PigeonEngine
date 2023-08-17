#pragma once

#include <CoreMinimal.h>
#include <RenderCommon.h>
#include <RenderResource.h>
#include <EngineCommon.h>
#include <BaseAsset.h>
#include <wincodec.h>
#include <wincodecsdk.h>
#pragma comment (lib, "windowscodecs.lib")

namespace PigeonEngine
{

	class ETexture2D : public EObjectBase, public EResourceInterface
	{
	public:
		virtual BOOL IsValid()const override;
		virtual void Release()override;
		void	SetData(BYTE* InByteCode, UINT InWidth, UINT InHeight, UINT InPixelByteCount, RFormatType InFormat, BOOL InSRGB);
	protected:
		BYTE*			ByteCode;
		UINT			Width;
		UINT			Height;
		UINT			PixelByteCount;
		RFormatType		Format;
		BOOL			SRGB;
	public:
		friend class ETexture2DAsset;
	public:

		CLASS_VIRTUAL_COPY_BODY(ETexture2D)

	};

	class ETexture2DAsset : public TRenderBaseAsset<ETexture2D, RTexture2DResource>
	{
	public:
		ETexture2DAsset(
			const EString& InPath
#ifdef _EDITOR_ONLY
			, const EString& InDebugName
#endif
		);
		virtual ~ETexture2DAsset();
	public:
		virtual BOOL	InitResource()override;
		const EString&	GetTexturePath()const;
	protected:
		RTexture2DResource*	CreateTextureResource(ETexture2D* InResource);
	protected:
		EString		TexturePath;
	public:
		ETexture2DAsset() = delete;

		CLASS_REMOVE_COPY_BODY(ETexture2DAsset)

	};

	class ETextureAssetManager : public EManagerBase
	{
	public:
		typedef TAssetManager<EString, ETexture2DAsset>		ETexture2DAssetManager;
	public:

	private:

	private:
		ETexture2DAssetManager	Texture2DAssetManager;

		CLASS_MANAGER_VIRTUAL_SINGLETON_BODY(ETextureAssetManager)

	};

};