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
		TShaderBaseAsset(
			const EString& InShaderPath
#ifdef _EDITOR_ONLY
			, const EString& InDebugName
#endif
		) : TRenderBaseAsset<EShaderResource, TShaderRenderResourceType>(
#ifdef _EDITOR_ONLY
			InDebugName
#endif
		), ShaderPath(InShaderPath), ShaderFrequency(_ShaderFrequency)
		{
		}
		virtual ~TShaderBaseAsset()
		{
		}
	public:
		const EString&			GetShaderPath()const { return ShaderPath; }
		RShaderFrequencyType	GetShaderFrequency()const { return ShaderFrequency; }
	protected:
		EString					ShaderPath;
		RShaderFrequencyType	ShaderFrequency;

	public:
		TShaderBaseAsset() = delete;

		CLASS_REMOVE_COPY_BODY(TShaderBaseAsset)

	};

	class EVertexShaderAsset : public TShaderBaseAsset<RShaderFrequencyType::SHADER_FREQUENCY_VERTEX, RVertexShaderResource>
	{
	public:
		EVertexShaderAsset(const EString& InShaderPath
#ifdef _EDITOR_ONLY
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
		EPixelShaderAsset(const EString& InShaderPath
#ifdef _EDITOR_ONLY
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
		EComputeShaderAsset(const EString& InShaderPath
#ifdef _EDITOR_ONLY
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
		BOOL	ImportShaderCSO(const EString& InPath, const EString& OutPath, const RInputLayoutDesc* InShaderInputLayouts = nullptr, const UINT* InShaderInputLayoutNum = nullptr);
		BOOL	LoadVertexShaderAsset(const EString& InLoadPath, const EVertexShaderAsset*& OutShaderAsset);
		BOOL	LoadPixelShaderAsset(const EString& InLoadPath, const EPixelShaderAsset*& OutShaderAsset);
		BOOL	LoadComputeShaderAsset(const EString& InLoadPath, const EComputeShaderAsset*& OutShaderAsset);
	private:
		void	ClearVertexShaders();
		void	ClearPixelShaders();
		void	ClearComputeShaders();
	private:
		template<class TShaderAssetType>
		TShaderAssetType* LoadShaderAsset(const EString& InLoadPath);
		BOOL SaveShaderAsset(const EString& InSavePath, const EShaderResource* InShaderResource, RShaderFrequencyType InShaderFrequency, const RInputLayoutDesc* InShaderInputLayouts = nullptr, const UINT* InShaderInputLayoutNum = nullptr);
		template<class TShaderAssetType>
		BOOL SaveShaderAsset(const EString& InSavePath, const TShaderAssetType* InShaderAsset);
	private:
		EVertexShaderManager	VertexShaderManager;
		EPixelShaderManager		PixelShaderManager;
		EComputeShaderManager	ComputeShaderManager;

		CLASS_MANAGER_VIRTUAL_SINGLETON_BODY(EShaderAssetManager)

	};

};