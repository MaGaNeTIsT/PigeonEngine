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
			m_ShaderInputLayouts = nullptr;
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
		if (IsInitialized())
		{
#ifdef _DEVELOPMENT_EDITOR
			{
				std::string errorInfo = "Vertex shader name=[" + m_Name + "] path = [" + m_ShaderPath + "] has been Initialized.";
				PE_FAILED(ENGINE_SHADER_ERROR, errorInfo);
			}
#endif
			return TRUE;
		}
		if (CreateRenderResourceInternal(
			[this](EShaderResource* inResource)->RDeviceD3D11::RVertexShaderResource*
			{
				return (this->CreateShaderRenderResource(inResource));
			},
			FALSE))
		{
			m_IsInitialized = TRUE;
			return TRUE;
		}
		return FALSE;
	}
	RDeviceD3D11::RVertexShaderResource* EVertexShaderAsset::CreateShaderRenderResource(EShaderResource* inResource)
	{
		RDeviceD3D11::RVertexShaderResource* result = nullptr;
		if (inResource && inResource->ShaderByteCode && (inResource->ShaderByteCodeSize > 0u))
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
		if (IsInitialized())
		{
#ifdef _DEVELOPMENT_EDITOR
			{
				std::string errorInfo = "Vertex shader name=[" + m_Name + "] path = [" + m_ShaderPath + "] has been Initialized.";
				PE_FAILED(ENGINE_SHADER_ERROR, errorInfo);
			}
#endif
			return TRUE;
		}
		if (CreateRenderResourceInternal(
			[this](EShaderResource* inResource)->RDeviceD3D11::RPixelShaderResource*
			{
				return (this->CreateShaderRenderResource(inResource));
			},
			FALSE))
		{
			m_IsInitialized = TRUE;
			return TRUE;
		}
		return FALSE;
	}
	RDeviceD3D11::RPixelShaderResource* EPixelShaderAsset::CreateShaderRenderResource(EShaderResource* inResource)
	{
		RDeviceD3D11::RPixelShaderResource* result = nullptr;
		if (inResource && inResource->ShaderByteCode && (inResource->ShaderByteCodeSize > 0u))
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
		if (IsInitialized())
		{
#ifdef _DEVELOPMENT_EDITOR
			{
				std::string errorInfo = "Vertex shader name=[" + m_Name + "] path = [" + m_ShaderPath + "] has been Initialized.";
				PE_FAILED(ENGINE_SHADER_ERROR, errorInfo);
			}
#endif
			return TRUE;
		}
		if (CreateRenderResourceInternal(
			[this](EShaderResource* inResource)->RDeviceD3D11::RComputeShaderResource*
			{
				return (this->CreateShaderRenderResource(inResource));
			}, FALSE))
		{
			m_IsInitialized = TRUE;
			return TRUE;
		}
		return FALSE;
	}
	RDeviceD3D11::RComputeShaderResource* EComputeShaderAsset::CreateShaderRenderResource(EShaderResource* inResource)
	{
		RDeviceD3D11::RComputeShaderResource* result = nullptr;
		if (inResource && inResource->ShaderByteCode && (inResource->ShaderByteCodeSize > 0u))
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

	BOOL EShaderAssetManager::ImportShaderCSO(const std::string& inPath, const std::string& outPath, const RInputLayoutDesc* inShaderInputLayouts, const UINT* inShaderInputLayoutNum)
	{
		const static std::string importShaderNameType(ENGINE_IMPORT_SHADER_NAME_TYPE);
		const static std::string importVertexShaderNameType = ENGINE_IMPORT_VERTEX_SHADER_NAME_TYPE + importShaderNameType;
		const static std::string importPixelShaderNameType = ENGINE_IMPORT_PIXEL_SHADER_NAME_TYPE + importShaderNameType;
		const static std::string importComputeShaderNameType = ENGINE_IMPORT_COMPUTE_SHADER_NAME_TYPE + importShaderNameType;

		Check(ENGINE_ASSET_ERROR, "Import shader cso. Input file path is too short.", inPath.length() > importShaderNameType.length());
		Check(ENGINE_ASSET_ERROR, "Import shader cso. Input vs file path is too short.", inPath.length() > importVertexShaderNameType.length());
		Check(ENGINE_ASSET_ERROR, "Import shader cso. Input ps file path is too short.", inPath.length() > importPixelShaderNameType.length());
		Check(ENGINE_ASSET_ERROR, "Import shader cso. Input cs file path is too short.", inPath.length() > importComputeShaderNameType.length());

		if (inPath.substr(inPath.length() - importShaderNameType.length(), importShaderNameType.length()) != importShaderNameType)
		{
#ifdef _DEVELOPMENT_EDITOR
			{
				std::string errorData("Error file type for shader importer (input file path : ");
				errorData += inPath + ").";
				PE_FAILED(ENGINE_ASSET_ERROR, errorData);
			}
#endif
			return FALSE;
		}

		RShaderFrequencyType importShaderFrequency = RShaderFrequencyType::SHADER_FREQUENCY_COUNT;
		if (inPath.substr(inPath.length() - importPixelShaderNameType.length(), importPixelShaderNameType.length()) == importPixelShaderNameType)
		{
			importShaderFrequency = RShaderFrequencyType::SHADER_FREQUENCY_PIXEL;
		}
		else if (inPath.substr(inPath.length() - importVertexShaderNameType.length(), importVertexShaderNameType.length()) == importVertexShaderNameType)
		{
			if (!inShaderInputLayouts || !inShaderInputLayoutNum)
			{
				PE_FAILED(ENGINE_ASSET_ERROR, "Error, try to import vertex shader without input layouts.");
				return FALSE;
			}
			importShaderFrequency = RShaderFrequencyType::SHADER_FREQUENCY_VERTEX;
		}
		else if (inPath.substr(inPath.length() - importComputeShaderNameType.length(), importComputeShaderNameType.length()) == importComputeShaderNameType)
		{
			importShaderFrequency = RShaderFrequencyType::SHADER_FREQUENCY_COMPUTE;
		}
		else
		{
#ifdef _DEVELOPMENT_EDITOR
			{
				std::string errorData("Not support type for shader import (input file path : ");
				errorData += inPath + ").";
				PE_FAILED(ENGINE_ASSET_ERROR, errorData);
			}
#endif
			return FALSE;
		}

		EShaderResource* outResource = new EShaderResource();
		if (!EFileHelper::ReadFileAsBinary(inPath, outResource->ShaderByteCode, outResource->ShaderByteCodeSize))
		{
			outResource->Release();
			delete outResource;
			return FALSE;
		}

		if (SaveShaderAsset(outPath, outResource, importShaderFrequency, inShaderInputLayouts, inShaderInputLayoutNum))
		{
			outResource->Release();
			delete outResource;
			return TRUE;
		}

		outResource->Release();
		delete outResource;
		return FALSE;
	}
	BOOL EShaderAssetManager::LoadVertexShaderAsset(const std::string& loadPath, const EVertexShaderAsset*& outShaderAsset)
	{

	}
	BOOL EShaderAssetManager::LoadPixelShaderAsset(const std::string& loadPath, const EPixelShaderAsset*& outShaderAsset)
	{

	}
	BOOL EShaderAssetManager::LoadComputeShaderAsset(const std::string& loadPath, const EComputeShaderAsset*& outShaderAsset)
	{

	}
	template<class TShaderAssetType>
	TShaderAssetType* EShaderAssetManager::LoadShaderAsset(const std::string& loadPath)
	{
		void* readFileMem = nullptr; ULONG readFileSize = 0u;
		if (!EFileHelper::ReadFileAsBinary(loadPath, readFileMem, readFileSize))
		{
			if (readFileMem)
			{
				delete[]readFileMem;
			}
#ifdef _DEVELOPMENT_EDITOR
			{
				std::string errorData("Load shader asset failed (load file path : ");
				errorData += loadPath + ").";
				PE_FAILED(ENGINE_ASSET_ERROR, errorData);
			}
#endif
			return FALSE;
		}
		INT shaderFrequency = ((INT*)readFileMem)[0];
		INT shaderInputLayoutNum = ((INT*)readFileMem)[1];
		ULONG shaderSaveSize = inShaderResource->ShaderByteCodeSize + sizeof(INT) * 2u;
		RShaderFrequencyType saveShaderFrequency = inShaderAsset->GetShaderFrequency();
		return FALSE;
	}
	BOOL EShaderAssetManager::SaveShaderAsset(const std::string& savePath, const EShaderResource* inShaderResource, RShaderFrequencyType inShaderFrequency, const RInputLayoutDesc* inShaderInputLayouts, const UINT* inShaderInputLayoutNum)
	{
		if (inShaderResource && inShaderResource->ShaderByteCode && (inShaderResource->ShaderByteCodeSize > 0u))
		{
			const INT shaderFrequency = static_cast<INT>(inShaderFrequency);
			INT shaderInputLayoutNum = 0;
			ULONG shaderSaveSize = inShaderResource->ShaderByteCodeSize + sizeof(INT) * 2u;
			switch (inShaderFrequency)
			{
			case RShaderFrequencyType::SHADER_FREQUENCY_PIXEL:
			{
				//TODO We do not separate pixel & compute shader save and load for now.
			}
			break;
			case RShaderFrequencyType::SHADER_FREQUENCY_VERTEX:
			{
				if (!inShaderInputLayouts || !inShaderInputLayoutNum)
				{
					PE_FAILED(ENGINE_ASSET_ERROR, "Try to save vertex shader asset, but do not contain input layouts.");
					return FALSE;
				}
				shaderInputLayoutNum = static_cast<INT>(*inShaderInputLayoutNum);
				shaderSaveSize += sizeof(RInputLayoutDesc) * static_cast<UINT>(shaderInputLayoutNum);
			}
			break;
			case RShaderFrequencyType::SHADER_FREQUENCY_COMPUTE:
			{
				//TODO We do not separate pixel & compute shader save and load for now.
			}
			break;
			default:
			{
				PE_FAILED(ENGINE_ASSET_ERROR, "Not support type for shader import or save");
			}
			break;
			}

			//TODO We copy whole cso memory into a new memory that only for header info. So we could change save code for reduce cost.
			void* saveMem = new BYTE[shaderSaveSize];
			{
				const UINT shaderInputLayoutDescSize = sizeof(RInputLayoutDesc) * static_cast<UINT>(shaderInputLayoutNum);
				((INT*)saveMem)[0] = shaderFrequency;
				((INT*)saveMem)[1] = shaderInputLayoutNum;
				void* cpyStart = (void*)(&(((INT*)saveMem)[2]));
				ULONG rstSize = shaderSaveSize - sizeof(INT) * 2u;
				if (shaderInputLayoutNum > 0)
				{
					memcpy_s(cpyStart, rstSize, inShaderInputLayouts, shaderInputLayoutDescSize);
					cpyStart = (void*)(&(((BYTE*)cpyStart)[shaderInputLayoutDescSize]));
					rstSize = rstSize - shaderInputLayoutDescSize;
				}
				Check(ENGINE_ASSET_ERROR, "New copy buffer can not contain shader code.", rstSize == inShaderResource->ShaderByteCodeSize);
				memcpy_s(cpyStart, rstSize, inShaderResource->ShaderByteCode, inShaderResource->ShaderByteCodeSize);
			}

			if (EFileHelper::SaveBytesToFile(savePath, saveMem, shaderSaveSize))
			{
				delete[]saveMem;
				return TRUE;
			}

			delete[]saveMem;
		}

#ifdef _DEVELOPMENT_EDITOR
		{
			std::string errorData("Save shader asset failed (output file path : ");
			errorData += savePath + ").";
			PE_FAILED(ENGINE_ASSET_ERROR, errorData);
		}
#endif
		return FALSE;
	}
	template<class TShaderAssetType>
	BOOL EShaderAssetManager::SaveShaderAsset(const std::string& savePath, const TShaderAssetType* inShaderAsset)
	{
		if (!inShaderAsset)
		{
			PE_FAILED(ENGINE_ASSET_ERROR, "Error try to save a null shader asset.");
			return FALSE;
		}
		const EShaderResource* saveShaderResource = inShaderAsset->GetStoragedResource();
		if (!saveShaderResource)
		{
			PE_FAILED(ENGINE_ASSET_ERROR, "Error try to save a shader asset without resource.");
			return FALSE;
		}
		RShaderFrequencyType saveShaderFrequency = inShaderAsset->GetShaderFrequency();
		const RInputLayoutDesc* saveShaderInputLayouts = nullptr;
		UINT saveShaderInputLayoutNum = 0u;
		if (saveShaderFrequency == RShaderFrequencyType::SHADER_FREQUENCY_VERTEX)
		{
			const EVertexShaderAsset* tempVertexShaderAsset = reinterpret_cast<const EVertexShaderAsset*>(inShaderAsset);
			if (!tempVertexShaderAsset)
			{
				PE_FAILED(ENGINE_ASSET_ERROR, "Error try to saving a shader asset frequency is vertex, but actually is not vertex shader asset.");
				return FALSE;
			}
			saveShaderInputLayouts = tempVertexShaderAsset->GetShaderInputLayouts();
			saveShaderInputLayoutNum = tempVertexShaderAsset->GetShaderInputLayoutNum();
		}
		if (SaveShaderAsset(savePath, saveShaderResource, saveShaderFrequency, saveShaderInputLayouts, &saveShaderInputLayoutNum))
		{
			return TRUE;
		}
		return FALSE;
	}

};