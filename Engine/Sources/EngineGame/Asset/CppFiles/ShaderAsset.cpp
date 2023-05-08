#include "../Headers/ShaderAsset.h"
#include "../../../EngineCore/Core/Headers/ErrorCaption.h"
#include "../../../EngineCore/Base/Headers/FileHelper.h"

namespace PigeonEngine
{

	EVertexShaderAsset::EVertexShaderAsset(
		const std::string& shaderPath
#ifdef _DEVELOPMENT_EDITOR
		, const std::string& name
#endif
		, const RInputLayoutDesc* inInputLayouts
		, const UINT& inInputLayoutNum)
		: TShaderBaseAsset<RShaderFrequencyType::SHADER_FREQUENCY_VERTEX, RDeviceD3D11::RVertexShaderResource>(
			shaderPath
#ifdef _DEVELOPMENT_EDITOR
			, name
#endif
		), m_ShaderInputLayouts(nullptr), m_ShaderInputLayoutNum(0u)
	{
		if (inInputLayouts && inInputLayoutNum > 0u)
		{
			m_ShaderInputLayouts = new RInputLayoutDesc[inInputLayoutNum];
			for (UINT i = 0u; i < inInputLayoutNum; i++)
			{
				m_ShaderInputLayouts[i] = inInputLayouts[i];
			}
			m_ShaderInputLayoutNum = inInputLayoutNum;
		}
	}
	EVertexShaderAsset::~EVertexShaderAsset()
	{
		if (m_ShaderInputLayouts)
		{
			delete[]m_ShaderInputLayouts;
		}
	}
	BOOL EVertexShaderAsset::InitResource()
	{
		if (!m_ShaderInputLayouts || m_ShaderInputLayoutNum == 0u)
		{
#ifdef _DEVELOPMENT_EDITOR
			{
				std::string errorInfo = "Vertex shader name=[" + m_Name + "] path = [" + m_ShaderPath + "] does not contain input layouts when init resource.";
				PE_FAILED(ENGINE_SHADER_ERROR, errorInfo);
			}
#endif
			return FALSE;
		}
		if (!StorageResourceInternal([this]()->EShaderResource*
			{
				EShaderResource* outResource = new EShaderResource();
				if (!EFileHelper::ReadFileAsBinary(m_ShaderPath, outResource->ShaderByteCode, outResource->ShaderByteCodeSize))
				{
					delete outResource;
					return nullptr;
				}
				return outResource;
			}))
		{
			return FALSE;
		}
		if (CreateRenderResourceInternal(
			[this](EShaderResource* inResource)->RDeviceD3D11::RVertexShaderResource*
			{
				return (this->CreateShaderRenderResource(inResource));
			},
			FALSE))
		{
			return TRUE;
		}
		return FALSE;
	}
	RDeviceD3D11::RVertexShaderResource* EVertexShaderAsset::CreateShaderRenderResource(EShaderResource* inResource)
	{
		RDeviceD3D11::RVertexShaderResource* result = nullptr;
		if (inResource && inResource->ShaderByteCode && inResource->ShaderByteCodeSize > 0u)
		{
			RDeviceD3D11* deviceD3D11 = RDeviceD3D11::GetRenderDeviceD3D11();
			result = new RDeviceD3D11::RVertexShaderResource();
			if (!deviceD3D11->CreateVertexShaderResource(inResource->ShaderByteCode, inResource->ShaderByteCodeSize, *result, m_ShaderInputLayouts, m_ShaderInputLayoutNum))
			{
				delete result;
				result = nullptr;
			}
		}
		return result;
	}

	EPixelShaderAsset::EPixelShaderAsset(const std::string& shaderPath
#ifdef _DEVELOPMENT_EDITOR
		, const std::string& name
#endif
	) : TShaderBaseAsset<RShaderFrequencyType::SHADER_FREQUENCY_PIXEL, RDeviceD3D11::RPixelShaderResource>(
		shaderPath
#ifdef _DEVELOPMENT_EDITOR
		, name
#endif
	)
	{
	}
	EPixelShaderAsset::~EPixelShaderAsset()
	{
	}
	BOOL EPixelShaderAsset::InitResource()
	{
		if (!StorageResourceInternal([this]()->EShaderResource*
			{
				EShaderResource* outResource = new EShaderResource();
				if (!EFileHelper::ReadFileAsBinary(m_ShaderPath, outResource->ShaderByteCode, outResource->ShaderByteCodeSize))
				{
					delete outResource;
					return nullptr;
				}
				return outResource;
			}))
		{
			return FALSE;
		}
		if (CreateRenderResourceInternal(
			[this](EShaderResource* inResource)->RDeviceD3D11::RPixelShaderResource*
			{
				return (this->CreateShaderRenderResource(inResource));
			},
			FALSE))
		{
			return TRUE;
		}
		return FALSE;
	}
	RDeviceD3D11::RPixelShaderResource* EPixelShaderAsset::CreateShaderRenderResource(EShaderResource* inResource)
	{
		RDeviceD3D11::RPixelShaderResource* result = nullptr;
		if (inResource && inResource->ShaderByteCode && inResource->ShaderByteCodeSize > 0u)
		{
			RDeviceD3D11* deviceD3D11 = RDeviceD3D11::GetRenderDeviceD3D11();
			result = new RDeviceD3D11::RPixelShaderResource();
			if (!deviceD3D11->CreatePixelShaderResource(inResource->ShaderByteCode, inResource->ShaderByteCodeSize, *result))
			{
				delete result;
				result = nullptr;
			}
		}
		return result;
	}

	EComputeShaderAsset::EComputeShaderAsset(const std::string& shaderPath
#ifdef _DEVELOPMENT_EDITOR
		, const std::string& name
#endif
	) : TShaderBaseAsset<RShaderFrequencyType::SHADER_FREQUENCY_COMPUTE, RDeviceD3D11::RComputeShaderResource>(
		shaderPath
#ifdef _DEVELOPMENT_EDITOR
		, name
#endif
	)
	{
	}
	EComputeShaderAsset::~EComputeShaderAsset()
	{
	}
	BOOL EComputeShaderAsset::InitResource()
	{
		if (!StorageResourceInternal([this]()->EShaderResource*
			{
				EShaderResource* outResource = new EShaderResource();
				if (!EFileHelper::ReadFileAsBinary(m_ShaderPath, outResource->ShaderByteCode, outResource->ShaderByteCodeSize))
				{
					delete outResource;
					return nullptr;
				}
				return outResource;
			}))
		{
			return FALSE;
		}
		if (CreateRenderResourceInternal(
			[this](EShaderResource* inResource)->RDeviceD3D11::RComputeShaderResource*
			{
				return (this->CreateShaderRenderResource(inResource));
			}, FALSE))
		{
			return TRUE;
		}
		return FALSE;
	}
	RDeviceD3D11::RComputeShaderResource* EComputeShaderAsset::CreateShaderRenderResource(EShaderResource* inResource)
	{
		RDeviceD3D11::RComputeShaderResource* result = nullptr;
		if (inResource && inResource->ShaderByteCode && inResource->ShaderByteCodeSize > 0u)
		{
			RDeviceD3D11* deviceD3D11 = RDeviceD3D11::GetRenderDeviceD3D11();
			result = new RDeviceD3D11::RComputeShaderResource();
			if (!deviceD3D11->CreateComputeShaderResource(inResource->ShaderByteCode, inResource->ShaderByteCodeSize, *result))
			{
				delete result;
				result = nullptr;
			}
		}
		return result;
	}

};