#pragma once

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
		~TShaderBaseAsset()
		{
		}
	public:
		const std::string&		GetShaderPath()const { return m_ShaderPath; }
		RShaderFrequencyType	GetShaderFrequency()const { return m_ShaderFrequency; }
	protected:
		std::string				m_ShaderPath;
		RShaderFrequencyType	m_ShaderFrequency;
	};

	class EVertexShaderAsset : public TShaderBaseAsset<RShaderFrequencyType::SHADER_FREQUENCY_VERTEX, RDeviceD3D11::RVertexShaderResource>
	{
	public:
		EVertexShaderAsset(const std::string& shaderPath
#ifdef _DEVELOPMENT_EDITOR
			, const std::string& name
#endif
			, const RInputLayoutDesc* inInputLayouts, const UINT& inInputLayoutNum);
		~EVertexShaderAsset();
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
	};

	class EPixelShaderAsset : public TShaderBaseAsset<RShaderFrequencyType::SHADER_FREQUENCY_PIXEL, RDeviceD3D11::RPixelShaderResource>
	{
	public:
		EPixelShaderAsset(const std::string& shaderPath
#ifdef _DEVELOPMENT_EDITOR
			, const std::string& name
#endif
		);
		~EPixelShaderAsset();
	public:
		virtual BOOL	InitResource()override;
	protected:
		RDeviceD3D11::RPixelShaderResource*		CreateShaderRenderResource(EShaderResource* inResource);
	};

	class EComputeShaderAsset : public TShaderBaseAsset<RShaderFrequencyType::SHADER_FREQUENCY_COMPUTE, RDeviceD3D11::RComputeShaderResource>
	{
	public:
		EComputeShaderAsset(const std::string& shaderPath
#ifdef _DEVELOPMENT_EDITOR
			, const std::string& name
#endif
		);
		~EComputeShaderAsset();
	public:
		virtual BOOL	InitResource()override;
	protected:
		RDeviceD3D11::RComputeShaderResource*	CreateShaderRenderResource(EShaderResource* inResource);
	};

	class EShaderAssetManager
	{
	public:
		typedef TAssetManager<std::string, EVertexShaderAsset>		EVertexShaderManager;
		typedef TAssetManager<std::string, EPixelShaderAsset>		EPixelShaderManager;
		typedef TAssetManager<std::string, EComputeShaderAsset>		EComputeShaderManager;
	public:
		template<class TShaderAssetType>
		TShaderAssetType* LoadShaderAsset(const std::string& loadPath);

		template<class TShaderAssetType>
		void SaveShaderAsset(TShaderAssetType* inShaderAsset, const std::string& savePath);

		BOOL ImportShaderCSO(const std::string& inPath, const std::string& outPath);
	private:
		EVertexShaderManager	m_VSManager;
		EPixelShaderManager		m_PSManager;
		EComputeShaderManager	m_CSManager;
	};

};