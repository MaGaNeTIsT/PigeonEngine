#include "../Headers/ShaderAsset.h"

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
		if (CreateRenderResourceInternal(
			[this](EShaderResource* inResource)->RDeviceD3D11::RVertexShaderResource*
			{
				return (this->CreateShaderRenderResource());
			},
			FALSE))
		{
			return TRUE;
		}
		return FALSE;
	}
	RDeviceD3D11::RVertexShaderResource* EVertexShaderAsset::CreateShaderRenderResource()
	{
		RDeviceD3D11* deviceD3D11 = RDeviceD3D11::GetRenderDeviceD3D11();
		RDeviceD3D11::RVertexShaderResource* Result = new RDeviceD3D11::RVertexShaderResource();
		if (!deviceD3D11->LoadVertexShader(m_ShaderPath, *Result, m_ShaderInputLayouts, m_ShaderInputLayoutNum))
		{
			delete Result;
			return nullptr;
		}
		return Result;
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
		if (CreateRenderResourceInternal(
			[this](EShaderResource* inResource)->RDeviceD3D11::RPixelShaderResource*
			{
				return (this->CreateShaderRenderResource());
			},
			FALSE))
		{
			return TRUE;
		}
		return FALSE;
	}
	RDeviceD3D11::RPixelShaderResource* EPixelShaderAsset::CreateShaderRenderResource()
	{
		RDeviceD3D11* deviceD3D11 = RDeviceD3D11::GetRenderDeviceD3D11();
		RDeviceD3D11::RPixelShaderResource* Result = new RDeviceD3D11::RPixelShaderResource();
		if (!deviceD3D11->LoadPixelShader(m_ShaderPath, *Result))
		{
			delete Result;
			return nullptr;
		}
		return Result;
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

	}
	RDeviceD3D11::RComputeShaderResource* EComputeShaderAsset::CreateShaderRenderResource()
	{

	}

};