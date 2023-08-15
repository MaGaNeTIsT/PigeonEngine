#pragma once

#include <CoreMinimal.h>
#include <BaseAsset.h>
#include <RenderCommon.h>
#include <RenderDevice/DeviceD3D11.h>

namespace PigeonEngine
{

	struct EShaderResource
	{
		EShaderResource() : ShaderByteCode(nullptr), ShaderByteCodeSize(0u) {}
		void Release()
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
		typedef TRenderBaseAsset<EShaderResource, TShaderRenderResourceType> Super;

		EClass(TShaderBaseAsset, Super)

	public:
		TShaderBaseAsset(
			const EString& shaderPath
#ifdef _EDITOR_ONLY
			, const EString& name
#endif
		) : TRenderBaseAsset<EShaderResource, TShaderRenderResourceType>(
#ifdef _EDITOR_ONLY
			name
#endif
		), ShaderPath(shaderPath), ShaderFrequency(_ShaderFrequency)
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

	class EVertexShaderAsset : public TShaderBaseAsset<RShaderFrequencyType::SHADER_FREQUENCY_VERTEX, RDeviceD3D11::RVertexShaderResource>
	{
	public:
		typedef TShaderBaseAsset<RShaderFrequencyType::SHADER_FREQUENCY_VERTEX, RDeviceD3D11::RVertexShaderResource> Super;

		EClass(EVertexShaderAsset, Super)

	public:
		EVertexShaderAsset(const EString& shaderPath
#ifdef _EDITOR_ONLY
			, const EString& name
#endif
			, const RInputLayoutDesc* inInputLayouts = nullptr, const UINT& inInputLayoutNum = 0u);
		virtual ~EVertexShaderAsset();
	public:
		const RInputLayoutDesc*		GetShaderInputLayouts()const { return ShaderInputLayouts; }
		const UINT&					GetShaderInputLayoutNum()const { return ShaderInputLayoutNum; }
	public:
		virtual BOOL	InitResource()override;
	protected:
		RDeviceD3D11::RVertexShaderResource*	CreateShaderRenderResource(EShaderResource* inResource);
	protected:
		RInputLayoutDesc*	ShaderInputLayouts;
		UINT				ShaderInputLayoutNum;
	private:
		friend class EShaderAssetManager;

	public:
		EVertexShaderAsset() = delete;

		CLASS_REMOVE_COPY_BODY(EVertexShaderAsset)

	};

	class EPixelShaderAsset : public TShaderBaseAsset<RShaderFrequencyType::SHADER_FREQUENCY_PIXEL, RDeviceD3D11::RPixelShaderResource>
	{
	public:
		typedef TShaderBaseAsset<RShaderFrequencyType::SHADER_FREQUENCY_PIXEL, RDeviceD3D11::RPixelShaderResource> Super;

		EClass(EPixelShaderAsset, Super)

	public:
		EPixelShaderAsset(const EString& shaderPath
#ifdef _EDITOR_ONLY
			, const EString& name
#endif
		);
		virtual ~EPixelShaderAsset();
	public:
		virtual BOOL	InitResource()override;
	protected:
		RDeviceD3D11::RPixelShaderResource*		CreateShaderRenderResource(EShaderResource* inResource);
	private:
		friend class EShaderAssetManager;

	public:
		EPixelShaderAsset() = delete;

		CLASS_REMOVE_COPY_BODY(EPixelShaderAsset)

	};

	class EComputeShaderAsset : public TShaderBaseAsset<RShaderFrequencyType::SHADER_FREQUENCY_COMPUTE, RDeviceD3D11::RComputeShaderResource>
	{
	public:
		typedef TShaderBaseAsset<RShaderFrequencyType::SHADER_FREQUENCY_COMPUTE, RDeviceD3D11::RComputeShaderResource> Super;

		EClass(EComputeShaderAsset, Super)

	public:
		EComputeShaderAsset(const EString& shaderPath
#ifdef _EDITOR_ONLY
			, const EString& name
#endif
		);
		virtual ~EComputeShaderAsset();
	public:
		virtual BOOL	InitResource()override;
	protected:
		RDeviceD3D11::RComputeShaderResource*	CreateShaderRenderResource(EShaderResource* inResource);
	private:
		friend class EShaderAssetManager;

	public:
		EComputeShaderAsset() = delete;

		CLASS_REMOVE_COPY_BODY(EComputeShaderAsset)

	};

	class EShaderAssetManager : public EManagerBase
	{

		EClass(EShaderAssetManager, EManagerBase)

	public:
		typedef TAssetManager<EString, EVertexShaderAsset>		EVertexShaderManager;
		typedef TAssetManager<EString, EPixelShaderAsset>		EPixelShaderManager;
		typedef TAssetManager<EString, EComputeShaderAsset>		EComputeShaderManager;
	public:
		BOOL ImportShaderCSO(const EString& inPath, const EString& outPath, const RInputLayoutDesc* inShaderInputLayouts = nullptr, const UINT* inShaderInputLayoutNum = nullptr);
		BOOL LoadVertexShaderAsset(EString& loadPath, const EVertexShaderAsset*& outShaderAsset);
		BOOL LoadPixelShaderAsset(EString& loadPath, const EPixelShaderAsset*& outShaderAsset);
		BOOL LoadComputeShaderAsset(EString& loadPath, const EComputeShaderAsset*& outShaderAsset);
	private:
		template<class TShaderAssetType>
		TShaderAssetType* LoadShaderAsset(const EString& loadPath);
		BOOL SaveShaderAsset(const EString& savePath, const EShaderResource* inShaderResource, RShaderFrequencyType inShaderFrequency, const RInputLayoutDesc* inShaderInputLayouts = nullptr, const UINT* inShaderInputLayoutNum = nullptr);
		template<class TShaderAssetType>
		BOOL SaveShaderAsset(const EString& savePath, const TShaderAssetType* inShaderAsset);
	private:
		EVertexShaderManager	VertexShaderManager;
		EPixelShaderManager		PixelShaderManager;
		EComputeShaderManager	ComputeShaderManager;

		CLASS_MANAGER_VIRTUAL_SINGLETON_BODY(EShaderAssetManager)

	};

};