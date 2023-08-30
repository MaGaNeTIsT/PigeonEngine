#pragma once

#include <CoreMinimal.h>
#include <BaseAsset.h>
#include <EngineCommon.h>
#include <RenderCommon.h>
#include <RenderResource.h>

namespace PigeonEngine
{

	class EShaderResource : public EResourceInterface
	{
	public:
		EShaderResource() : ShaderByteCode(nullptr), ShaderByteCodeSize(0u) {}
		EShaderResource(const EShaderResource& Other)
			: ShaderByteCode(nullptr), ShaderByteCodeSize(0u)
		{
			if ((!!(Other.ShaderByteCode)) && (Other.ShaderByteCodeSize > 0u))
			{
				ShaderByteCodeSize = Other.ShaderByteCodeSize;
				ShaderByteCode = new BYTE[Other.ShaderByteCodeSize];
				::memcpy_s(ShaderByteCode, Other.ShaderByteCodeSize, Other.ShaderByteCode, Other.ShaderByteCodeSize);
			}
		}
		virtual ~EShaderResource() { ReleaseResource(); }
		EShaderResource& operator=(const EShaderResource& Other)
		{
			ReleaseResource();
			if ((!!(Other.ShaderByteCode)) && (Other.ShaderByteCodeSize > 0u))
			{
				ShaderByteCodeSize = Other.ShaderByteCodeSize;
				ShaderByteCode = new BYTE[Other.ShaderByteCodeSize];
				::memcpy_s(ShaderByteCode, Other.ShaderByteCodeSize, Other.ShaderByteCode, Other.ShaderByteCodeSize);
			}
			return (*this);
		}
		virtual BOOL IsResourceValid()const override
		{
			return ((ShaderByteCode != nullptr) && (ShaderByteCodeSize > 0u));
		}
		virtual BOOL InitResource()override
		{
			// Shader resource must init by shader manager.
			return TRUE;
		}
		virtual void ReleaseResource()override
		{
			if (ShaderByteCode != nullptr)
			{
				delete[]ShaderByteCode;
				ShaderByteCode = nullptr;
			}
			ShaderByteCodeSize = 0u;
		}
		void*	ShaderByteCode;
		ULONG	ShaderByteCodeSize;
	};

	template<RShaderFrequencyType _ShaderFrequency, typename TShaderRenderResourceType>
	class TShaderBaseAsset : public TRenderBaseAsset<EShaderResource, TShaderRenderResourceType>
	{
	public:
		TShaderBaseAsset(const EString& InAssetPath, const EString& InAssetName
#if _EDITOR_ONLY
			, const EString& InDebugName
#endif
		) : TRenderBaseAsset<EShaderResource, TShaderRenderResourceType>(InAssetPath, InAssetName
#if _EDITOR_ONLY
			, InDebugName
#endif
		), ShaderPath(InShaderPath), ShaderFrequency(_ShaderFrequency)
		{
		}
		virtual ~TShaderBaseAsset()
		{
		}
	public:
		RShaderFrequencyType	GetShaderFrequency()const { return ShaderFrequency; }
	protected:
		RShaderFrequencyType	ShaderFrequency;

	public:
		TShaderBaseAsset() = delete;

		CLASS_REMOVE_COPY_BODY(TShaderBaseAsset)

	};

	class EVertexShaderAsset : public TShaderBaseAsset<RShaderFrequencyType::SHADER_FREQUENCY_VERTEX, RVertexShaderResource>
	{
	public:
		EVertexShaderAsset(const EString& InAssetPath, const EString& InAssetName
#if _EDITOR_ONLY
			, const EString& InDebugName
#endif
			, const RInputLayoutDesc* InInputLayouts = nullptr, const UINT& InInputLayoutNum = 0u);
		virtual ~EVertexShaderAsset();
	public:
		const RInputLayoutDesc*		GetShaderInputLayouts()const { return ShaderInputLayouts; }
		const UINT&					GetShaderInputLayoutNum()const { return ShaderInputLayoutNum; }
	public:
		virtual BOOL	InitResource()override;
	protected:
		RVertexShaderResource*		CreateShaderRenderResource(EShaderResource* InResource);
	protected:
		RInputLayoutDesc*	ShaderInputLayouts;
		UINT				ShaderInputLayoutNum;
	private:
		friend class EShaderAssetManager;

	public:
		EVertexShaderAsset() = delete;

		CLASS_REMOVE_COPY_BODY(EVertexShaderAsset)

	};

	class EPixelShaderAsset : public TShaderBaseAsset<RShaderFrequencyType::SHADER_FREQUENCY_PIXEL, RPixelShaderResource>
	{
	public:
		EPixelShaderAsset(const EString& InAssetPath, const EString& InAssetName
#if _EDITOR_ONLY
			, const EString& InDebugName
#endif
		);
		virtual ~EPixelShaderAsset();
	public:
		virtual BOOL	InitResource()override;
	protected:
		RPixelShaderResource*	CreateShaderRenderResource(EShaderResource* InResource);
	private:
		friend class EShaderAssetManager;

	public:
		EPixelShaderAsset() = delete;

		CLASS_REMOVE_COPY_BODY(EPixelShaderAsset)

	};

	class EComputeShaderAsset : public TShaderBaseAsset<RShaderFrequencyType::SHADER_FREQUENCY_COMPUTE, RComputeShaderResource>
	{
	public:
		EComputeShaderAsset(const EString& InAssetPath, const EString& InAssetName
#if _EDITOR_ONLY
			, const EString& InDebugName
#endif
		);
		virtual ~EComputeShaderAsset();
	public:
		virtual BOOL	InitResource()override;
	protected:
		RComputeShaderResource*	CreateShaderRenderResource(EShaderResource* InResource);
	private:
		friend class EShaderAssetManager;

	public:
		EComputeShaderAsset() = delete;

		CLASS_REMOVE_COPY_BODY(EComputeShaderAsset)

	};

	class EShaderAssetManager : public EManagerBase
	{
	public:
		typedef TAssetManager<EString, EVertexShaderAsset>		EVertexShaderManager;
		typedef TAssetManager<EString, EPixelShaderAsset>		EPixelShaderManager;
		typedef TAssetManager<EString, EComputeShaderAsset>		EComputeShaderManager;
	public:
		virtual void	Initialize()override;
		virtual void	ShutDown()override;
	public:
#if _EDITOR_ONLY
		BOOL	ImportVertexShader(const EString& InAssetName, const EString& InImportFullPathName, const EString& InSavePath, const RInputLayoutDesc* InShaderInputLayouts = nullptr, const UINT* InShaderInputLayoutNum = nullptr);
		BOOL	ImportPixelShader(const EString& InAssetName, const EString& InImportFullPathName, const EString& InSavePath);
		BOOL	ImportComputeShader(const EString& InAssetName, const EString& InImportFullPathName, const EString& InSavePath);
#endif
		BOOL	LoadVertexShaderAsset(const EString& InLoadPath, const EString& InLoadName, const EVertexShaderAsset*& OutShaderAsset);
		BOOL	LoadPixelShaderAsset(const EString& InLoadPath, const EString& InLoadName, const EPixelShaderAsset*& OutShaderAsset);
		BOOL	LoadComputeShaderAsset(const EString& InLoadPath, const EString& InLoadName, const EComputeShaderAsset*& OutShaderAsset);
	private:
		void	ClearVertexShaders();
		void	ClearPixelShaders();
		void	ClearComputeShaders();
	private:
		template<class TShaderAssetType>
		TShaderAssetType* LoadShaderAsset(const EString& InLoadPath, const EString& InLoadName);
		BOOL SaveShaderResource(const EString& InSavePath, const EString& InSaveName, const EShaderResource* InShaderResource, RShaderFrequencyType InShaderFrequency, const RInputLayoutDesc* InShaderInputLayouts = nullptr, const UINT* InShaderInputLayoutNum = nullptr);
		template<class TShaderAssetType>
		BOOL SaveShaderAsset(const EString& InSavePath, const EString& InSaveName, const TShaderAssetType* InShaderAsset);
	private:
		EVertexShaderManager	VertexShaderManager;
		EPixelShaderManager		PixelShaderManager;
		EComputeShaderManager	ComputeShaderManager;

		CLASS_MANAGER_VIRTUAL_SINGLETON_BODY(EShaderAssetManager)

	};

};