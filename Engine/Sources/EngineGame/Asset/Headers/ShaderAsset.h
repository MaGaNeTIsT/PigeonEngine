#pragma once

#include "../../../EngineCore/Core/Headers/Main.h"
#include "./BaseAsset.h"
#include "../../../EngineCore/RenderCore/Headers/Common.h"
#include "../../../EngineCore/RenderCore/Headers/DeviceD3D11.h"

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
		//EClass(TShaderBaseAsset<_ShaderFrequency, TShaderRenderResourceType>, TRenderBaseAsset<EShaderResource, TShaderRenderResourceType>)

	public:
		TShaderBaseAsset(
			const std::string& shaderPath
#ifdef _DEVELOPMENT_EDITOR
			, const std::string& name
#endif
		) : TRenderBaseAsset<EShaderResource, TShaderRenderResourceType>(
#ifdef _DEVELOPMENT_EDITOR
			name
#endif
		), m_ShaderPath(shaderPath), m_ShaderFrequency(_ShaderFrequency)
		{
		}
		virtual ~TShaderBaseAsset()
		{
		}
	public:
		const std::string&		GetShaderPath()const { return m_ShaderPath; }
		RShaderFrequencyType	GetShaderFrequency()const { return m_ShaderFrequency; }
	protected:
		std::string				m_ShaderPath;
		RShaderFrequencyType	m_ShaderFrequency;

	public:
		TShaderBaseAsset() = delete;

		CLASS_REMOVE_COPY_BODY(TShaderBaseAsset)

	};

	class EVertexShaderAsset : public TShaderBaseAsset<RShaderFrequencyType::SHADER_FREQUENCY_VERTEX, RDeviceD3D11::RVertexShaderResource>
	{
		//EClass(EVertexShaderAsset, TShaderBaseAsset<RShaderFrequencyType::SHADER_FREQUENCY_VERTEX, RDeviceD3D11::RVertexShaderResource>)

	public:
		EVertexShaderAsset(const std::string& shaderPath
#ifdef _DEVELOPMENT_EDITOR
			, const std::string& name
#endif
			, const RInputLayoutDesc* inInputLayouts, const UINT& inInputLayoutNum);
		virtual ~EVertexShaderAsset();
	public:
		const RInputLayoutDesc*		GetShaderInputLayouts()const { return m_ShaderInputLayouts; }
		const UINT&					GetShaderInputLayoutNum()const { return m_ShaderInputLayoutNum; }
	public:
		virtual BOOL	InitResource()override;
	protected:
		RDeviceD3D11::RVertexShaderResource*	CreateShaderRenderResource(EShaderResource* inResource);
	protected:
		RInputLayoutDesc*	m_ShaderInputLayouts;
		UINT				m_ShaderInputLayoutNum;
	private:
		friend class EShaderAssetManager;

	public:
		EVertexShaderAsset() = delete;

		CLASS_REMOVE_COPY_BODY(EVertexShaderAsset)

	};

	class EPixelShaderAsset : public TShaderBaseAsset<RShaderFrequencyType::SHADER_FREQUENCY_PIXEL, RDeviceD3D11::RPixelShaderResource>
	{
		//EClass(EPixelShaderAsset, TShaderBaseAsset<RShaderFrequencyType::SHADER_FREQUENCY_PIXEL, RDeviceD3D11::RPixelShaderResource>)

	public:
		EPixelShaderAsset(const std::string& shaderPath
#ifdef _DEVELOPMENT_EDITOR
			, const std::string& name
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
		//EClass(EComputeShaderAsset, TShaderBaseAsset<RShaderFrequencyType::SHADER_FREQUENCY_COMPUTE, RDeviceD3D11::RComputeShaderResource>)

	public:
		EComputeShaderAsset(const std::string& shaderPath
#ifdef _DEVELOPMENT_EDITOR
			, const std::string& name
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
		typedef TAssetManager<std::string, EVertexShaderAsset>		EVertexShaderManager;
		typedef TAssetManager<std::string, EPixelShaderAsset>		EPixelShaderManager;
		typedef TAssetManager<std::string, EComputeShaderAsset>		EComputeShaderManager;
	public:
		BOOL ImportShaderCSO(const std::string& inPath, const std::string& outPath, const RInputLayoutDesc* inShaderInputLayouts = nullptr, const UINT* inShaderInputLayoutNum = nullptr);
		BOOL LoadVertexShaderAsset(std::string& loadPath, const EVertexShaderAsset*& outShaderAsset);
		BOOL LoadPixelShaderAsset(std::string& loadPath, const EPixelShaderAsset*& outShaderAsset);
		BOOL LoadComputeShaderAsset(std::string& loadPath, const EComputeShaderAsset*& outShaderAsset);
	private:
		template<class TShaderAssetType>
		TShaderAssetType* LoadShaderAsset(const std::string& loadPath);
		BOOL SaveShaderAsset(const std::string& savePath, const EShaderResource* inShaderResource, RShaderFrequencyType inShaderFrequency, const RInputLayoutDesc* inShaderInputLayouts = nullptr, const UINT* inShaderInputLayoutNum = nullptr);
		template<class TShaderAssetType>
		BOOL SaveShaderAsset(const std::string& savePath, const TShaderAssetType* inShaderAsset);
	private:
		EVertexShaderManager	m_VSManager;
		EPixelShaderManager		m_PSManager;
		EComputeShaderManager	m_CSManager;

		CLASS_MANAGER_VIRTUAL_SINGLETON_BODY(EShaderAssetManager)

	};

};