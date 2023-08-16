#include "ShaderAsset.h"
#include <RenderDevice/DeviceD3D11.h>

namespace PigeonEngine
{

	EVertexShaderAsset::EVertexShaderAsset(
		const EString& InShaderPath
#ifdef _EDITOR_ONLY
		, const EString& InDebugName
#endif
		, const RInputLayoutDesc* InInputLayouts
		, const UINT& InInputLayoutNum)
		: TShaderBaseAsset<RShaderFrequencyType::SHADER_FREQUENCY_VERTEX, RVertexShaderResource>(
			InShaderPath
#ifdef _EDITOR_ONLY
			, InDebugName
#endif
		), ShaderInputLayouts(nullptr), ShaderInputLayoutNum(0u)
	{
		if (InInputLayouts && InInputLayoutNum > 0u)
		{
			ShaderInputLayouts = new RInputLayoutDesc[InInputLayoutNum];
			for (UINT i = 0u; i < InInputLayoutNum; i++)
			{
				ShaderInputLayouts[i] = InInputLayouts[i];
			}
			ShaderInputLayoutNum = InInputLayoutNum;
		}
	}
	EVertexShaderAsset::~EVertexShaderAsset()
	{
		if (ShaderInputLayouts)
		{
			delete[]ShaderInputLayouts;
			ShaderInputLayouts = nullptr;
		}
	}
	BOOL EVertexShaderAsset::InitResource()
	{
		if (!ShaderInputLayouts || ShaderInputLayoutNum == 0u)
		{
#ifdef _EDITOR_ONLY
			{
				EString errorInfo = EString("Vertex shader name=[") + DebugName + "] path = [" + ShaderPath + "] does not contain input layouts when init resource.";
				PE_FAILED((ENGINE_SHADER_ERROR), errorInfo);
			}
#endif
			return FALSE;
		}
		if (IsInitialized())
		{
#ifdef _EDITOR_ONLY
			{
				EString errorInfo = EString("Vertex shader name=[") + DebugName + "] path = [" + ShaderPath + "] has been Initialized.";
				PE_FAILED((ENGINE_SHADER_ERROR), errorInfo);
			}
#endif
			return TRUE;
		}
		if (CreateRenderResourceInternal(
			[this](EShaderResource* inResource)->RVertexShaderResource*
			{
				return (this->CreateShaderRenderResource(inResource));
			},
			FALSE))
		{
			bIsInitialized = TRUE;
			return TRUE;
		}
		return FALSE;
	}
	RVertexShaderResource* EVertexShaderAsset::CreateShaderRenderResource(EShaderResource* InResource)
	{
		RVertexShaderResource* result = nullptr;
		if (InResource && InResource->ShaderByteCode && (InResource->ShaderByteCodeSize > 0u))
		{
			RDeviceD3D11* deviceD3D11 = RDeviceD3D11::GetDeviceSingleton();
			result = new RVertexShaderResource();
			if (!deviceD3D11->CreateVertexShaderResource(InResource->ShaderByteCode, InResource->ShaderByteCodeSize, *result, ShaderInputLayouts, ShaderInputLayoutNum))
			{
				delete result;
				result = nullptr;
			}
		}
		return result;
	}

	EPixelShaderAsset::EPixelShaderAsset(const EString& InShaderPath
#ifdef _EDITOR_ONLY
		, const EString& InDebugName
#endif
	) : TShaderBaseAsset<RShaderFrequencyType::SHADER_FREQUENCY_PIXEL, RPixelShaderResource>(
		InShaderPath
#ifdef _EDITOR_ONLY
		, InDebugName
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
#ifdef _EDITOR_ONLY
			{
				EString errorInfo = EString("Vertex shader name=[") + DebugName + "] path = [" + ShaderPath + "] has been Initialized.";
				PE_FAILED((ENGINE_SHADER_ERROR), errorInfo);
			}
#endif
			return TRUE;
		}
		if (CreateRenderResourceInternal(
			[this](EShaderResource* inResource)->RPixelShaderResource*
			{
				return (this->CreateShaderRenderResource(inResource));
			},
			FALSE))
		{
			bIsInitialized = TRUE;
			return TRUE;
		}
		return FALSE;
	}
	RPixelShaderResource* EPixelShaderAsset::CreateShaderRenderResource(EShaderResource* InResource)
	{
		RPixelShaderResource* result = nullptr;
		if (InResource && InResource->ShaderByteCode && (InResource->ShaderByteCodeSize > 0u))
		{
			RDeviceD3D11* deviceD3D11 = RDeviceD3D11::GetDeviceSingleton();
			result = new RPixelShaderResource();
			if (!deviceD3D11->CreatePixelShaderResource(InResource->ShaderByteCode, InResource->ShaderByteCodeSize, *result))
			{
				delete result;
				result = nullptr;
			}
		}
		return result;
	}

	EComputeShaderAsset::EComputeShaderAsset(const EString& InShaderPath
#ifdef _EDITOR_ONLY
		, const EString& InDebugName
#endif
	) : TShaderBaseAsset<RShaderFrequencyType::SHADER_FREQUENCY_COMPUTE, RComputeShaderResource>(
		InShaderPath
#ifdef _EDITOR_ONLY
		, InDebugName
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
#ifdef _EDITOR_ONLY
			{
				EString errorInfo = EString("Vertex shader name=[") + DebugName + "] path = [" + ShaderPath + "] has been Initialized.";
				PE_FAILED((ENGINE_SHADER_ERROR), errorInfo);
			}
#endif
			return TRUE;
		}
		if (CreateRenderResourceInternal(
			[this](EShaderResource* inResource)->RComputeShaderResource*
			{
				return (this->CreateShaderRenderResource(inResource));
			}, FALSE))
		{
			bIsInitialized = TRUE;
			return TRUE;
		}
		return FALSE;
	}
	RComputeShaderResource* EComputeShaderAsset::CreateShaderRenderResource(EShaderResource* InResource)
	{
		RComputeShaderResource* result = nullptr;
		if (InResource && InResource->ShaderByteCode && (InResource->ShaderByteCodeSize > 0u))
		{
			RDeviceD3D11* deviceD3D11 = RDeviceD3D11::GetDeviceSingleton();
			result = new RComputeShaderResource();
			if (!deviceD3D11->CreateComputeShaderResource(InResource->ShaderByteCode, InResource->ShaderByteCodeSize, *result))
			{
				delete result;
				result = nullptr;
			}
		}
		return result;
	}

	EShaderAssetManager::EShaderAssetManager()
	{
#ifdef _EDITOR_ONLY
		DebugName = ENGINE_SHADER_ASSET_MANAGER_NAME;
#endif
	}
	EShaderAssetManager::~EShaderAssetManager()
	{
	}
	BOOL EShaderAssetManager::ImportShaderCSO(const EString& inPath, const EString& outPath, const RInputLayoutDesc* inShaderInputLayouts, const UINT* inShaderInputLayoutNum)
	{
		const static EString importShaderNameType(ENGINE_IMPORT_SHADER_NAME_TYPE);
		const static EString importVertexShaderNameType = EString(ENGINE_IMPORT_VERTEX_SHADER_NAME_TYPE) + importShaderNameType;
		const static EString importPixelShaderNameType = EString(ENGINE_IMPORT_PIXEL_SHADER_NAME_TYPE) + importShaderNameType;
		const static EString importComputeShaderNameType = EString(ENGINE_IMPORT_COMPUTE_SHADER_NAME_TYPE) + importShaderNameType;

		Check((ENGINE_ASSET_ERROR), ("Import shader cso. Input file path is too short."), inPath.Length() > importShaderNameType.Length());
		Check((ENGINE_ASSET_ERROR), ("Import shader cso. Input vs file path is too short."), inPath.Length() > importVertexShaderNameType.Length());
		Check((ENGINE_ASSET_ERROR), ("Import shader cso. Input ps file path is too short."), inPath.Length() > importPixelShaderNameType.Length());
		Check((ENGINE_ASSET_ERROR), ("Import shader cso. Input cs file path is too short."), inPath.Length() > importComputeShaderNameType.Length());

		if (inPath.Substring(inPath.Length() - importShaderNameType.Length(), importShaderNameType.Length()) != importShaderNameType)
		{
#ifdef _EDITOR_ONLY
			{
				EString errorData("Error file type for shader importer (input file path : ");
				errorData = errorData + inPath + ").";
				PE_FAILED((ENGINE_ASSET_ERROR), errorData);
			}
#endif
			return FALSE;
		}

		RShaderFrequencyType importShaderFrequency = RShaderFrequencyType::SHADER_FREQUENCY_COUNT;
		if (inPath.Substring(inPath.Length() - importPixelShaderNameType.Length(), importPixelShaderNameType.Length()) == importPixelShaderNameType)
		{
			importShaderFrequency = RShaderFrequencyType::SHADER_FREQUENCY_PIXEL;
		}
		else if (inPath.Substring(inPath.Length() - importVertexShaderNameType.Length(), importVertexShaderNameType.Length()) == importVertexShaderNameType)
		{
			if (!inShaderInputLayouts || !inShaderInputLayoutNum)
			{
				PE_FAILED((ENGINE_ASSET_ERROR), ("Error, try to import vertex shader without input layouts."));
				return FALSE;
			}
			importShaderFrequency = RShaderFrequencyType::SHADER_FREQUENCY_VERTEX;
		}
		else if (inPath.Substring(inPath.Length() - importComputeShaderNameType.Length(), importComputeShaderNameType.Length()) == importComputeShaderNameType)
		{
			importShaderFrequency = RShaderFrequencyType::SHADER_FREQUENCY_COMPUTE;
		}
		else
		{
#ifdef _EDITOR_ONLY
			{
				EString errorData("Not support type for shader import (input file path : ");
				errorData = errorData + inPath + ").";
				PE_FAILED((ENGINE_ASSET_ERROR), errorData);
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
	BOOL EShaderAssetManager::LoadVertexShaderAsset(EString& loadPath, const EVertexShaderAsset*& outShaderAsset)
	{
		EVertexShaderAsset* resultShaderAsset = VertexShaderManager.Find(loadPath);
		if (resultShaderAsset)
		{
			outShaderAsset = resultShaderAsset;
			return TRUE;
		}
		resultShaderAsset = LoadShaderAsset<EVertexShaderAsset>(loadPath);
		if (!resultShaderAsset)
		{
			return FALSE;
		}
		if (!resultShaderAsset->InitResource())
		{
			delete resultShaderAsset;
			return FALSE;
		}
		if (VertexShaderManager.Add(loadPath, resultShaderAsset, TRUE) == 0u)
		{
#ifdef _EDITOR_ONLY
			{
				EString errorInfo = EString("Vertex shader path = [") + loadPath + "] add into manager list failed.";
				PE_FAILED((ENGINE_ASSET_ERROR), errorInfo);
			}
#endif
			delete resultShaderAsset;
			return FALSE;
		}
		outShaderAsset = resultShaderAsset;
		return TRUE;
	}
	BOOL EShaderAssetManager::LoadPixelShaderAsset(EString& loadPath, const EPixelShaderAsset*& outShaderAsset)
	{
		EPixelShaderAsset* resultShaderAsset = PixelShaderManager.Find(loadPath);
		if (resultShaderAsset)
		{
			outShaderAsset = resultShaderAsset;
			return TRUE;
		}
		resultShaderAsset = LoadShaderAsset<EPixelShaderAsset>(loadPath);
		if (!resultShaderAsset)
		{
			return FALSE;
		}
		if (!resultShaderAsset->InitResource())
		{
			delete resultShaderAsset;
			return FALSE;
		}
		if (PixelShaderManager.Add(loadPath, resultShaderAsset, TRUE) == 0u)
		{
#ifdef _EDITOR_ONLY
			{
				EString errorInfo = EString("Pixel shader path = [") + loadPath + "] add into manager list failed.";
				PE_FAILED((ENGINE_ASSET_ERROR), errorInfo);
			}
#endif
			delete resultShaderAsset;
			return FALSE;
		}
		outShaderAsset = resultShaderAsset;
		return TRUE;
	}
	BOOL EShaderAssetManager::LoadComputeShaderAsset(EString& loadPath, const EComputeShaderAsset*& outShaderAsset)
	{
		EComputeShaderAsset* resultShaderAsset = ComputeShaderManager.Find(loadPath);
		if (resultShaderAsset)
		{
			outShaderAsset = resultShaderAsset;
			return TRUE;
		}
		resultShaderAsset = LoadShaderAsset<EComputeShaderAsset>(loadPath);
		if (!resultShaderAsset)
		{
			return FALSE;
		}
		if (!resultShaderAsset->InitResource())
		{
			delete resultShaderAsset;
			return FALSE;
		}
		if (ComputeShaderManager.Add(loadPath, resultShaderAsset, TRUE) == 0u)
		{
#ifdef _EDITOR_ONLY
			{
				EString errorInfo = EString("Pixel shader path = [") + loadPath + "] add into manager list failed.";
				PE_FAILED((ENGINE_ASSET_ERROR), errorInfo);
			}
#endif
			delete resultShaderAsset;
			return FALSE;
		}
		outShaderAsset = resultShaderAsset;
		return TRUE;
	}
	template<class TShaderAssetType>
	TShaderAssetType* EShaderAssetManager::LoadShaderAsset(const EString& loadPath)
	{
		void* readFileMem = nullptr; ULONG readFileSize = 0u;
		if (!EFileHelper::ReadFileAsBinary(loadPath, readFileMem, readFileSize))
		{
			if (readFileMem)
			{
				delete[]readFileMem;
			}
#ifdef _EDITOR_ONLY
			{
				EString errorData("Load shader asset failed (load file path : ");
				errorData += loadPath;
				errorData += ").";
				PE_FAILED((ENGINE_ASSET_ERROR), (errorData));
			}
#endif
			return nullptr;
		}
		Check(ENGINE_ASSET_ERROR, "Error read shader asset file size are too small.", (readFileSize > (sizeof(INT) * 2u)));
		INT shaderFrequency = ((INT*)readFileMem)[0];
		UINT shaderInputLayoutNum = static_cast<UINT>(((INT*)readFileMem)[1]);
		ULONG shaderCSOSize = readFileSize - sizeof(INT) * 2u;
		TArray<RInputLayoutDesc> shaderInputLayouts;
		if (shaderInputLayoutNum > 0u)
		{
			shaderCSOSize = shaderCSOSize - sizeof(RInputLayoutDesc) * shaderInputLayoutNum;
			RInputLayoutDesc* tempInputlayouts = (RInputLayoutDesc*)(&(((INT*)readFileMem)[2]));
			shaderInputLayouts.Resize(shaderInputLayoutNum);
			for (UINT layoutIndex = 0u; layoutIndex < shaderInputLayoutNum; layoutIndex++)
			{
				shaderInputLayouts[layoutIndex] = tempInputlayouts[layoutIndex];
			}
		}
		TShaderAssetType* result = nullptr;
		if (shaderFrequency == RShaderFrequencyType::SHADER_FREQUENCY_VERTEX)
		{
			EVertexShaderAsset* TempShaderAsset = new EVertexShaderAsset(loadPath
#ifdef _EDITOR_ONLY
				, loadPath
#endif
				, shaderInputLayouts.RawData(), shaderInputLayoutNum);
			result = reinterpret_cast<TShaderAssetType*>(TempShaderAsset);
			if (!result)
			{
				TempShaderAsset->UninitResource();
				delete TempShaderAsset;
				TempShaderAsset = nullptr;
			}
		}
		else
		{
			result = new TShaderAssetType(loadPath
#ifdef _EDITOR_ONLY
				, loadPath
#endif
			);
		}
		if (result)
		{
			EShaderResource* storagedResource = nullptr;
			if (!result->StorageResourceInternal(
				[readFileMem, readFileSize, shaderCSOSize, &storagedResource]()->EShaderResource*
				{
					void* shaderCSO = new BYTE[shaderCSOSize];
					::memcpy_s(shaderCSO, shaderCSOSize, &(((BYTE*)readFileMem)[readFileSize - shaderCSOSize]), shaderCSOSize);
					storagedResource = new EShaderResource();
					storagedResource->ShaderByteCode = shaderCSO;
					storagedResource->ShaderByteCodeSize = shaderCSOSize;
					return storagedResource;
				}))
			{
				if (storagedResource)
				{
					storagedResource->Release();
					delete storagedResource;
				}
			}
		}
		delete[]readFileMem;
		return result;
	}
	BOOL EShaderAssetManager::SaveShaderAsset(const EString& savePath, const EShaderResource* inShaderResource, RShaderFrequencyType inShaderFrequency, const RInputLayoutDesc* inShaderInputLayouts, const UINT* inShaderInputLayoutNum)
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
					PE_FAILED((ENGINE_ASSET_ERROR), ("Try to save vertex shader asset, but do not contain input layouts."));
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
				PE_FAILED((ENGINE_ASSET_ERROR), ("Not support type for shader import or save"));
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
					::memcpy_s(cpyStart, rstSize, inShaderInputLayouts, shaderInputLayoutDescSize);
					cpyStart = (void*)(&(((BYTE*)cpyStart)[shaderInputLayoutDescSize]));
					rstSize = rstSize - shaderInputLayoutDescSize;
				}
				Check((ENGINE_ASSET_ERROR), ("New copy buffer can not contain shader code."), rstSize == inShaderResource->ShaderByteCodeSize);
				::memcpy_s(cpyStart, rstSize, inShaderResource->ShaderByteCode, inShaderResource->ShaderByteCodeSize);
			}

			if (EFileHelper::SaveBytesToFile(savePath, saveMem, shaderSaveSize))
			{
				delete[]saveMem;
				return TRUE;
			}

			delete[]saveMem;
		}

#ifdef _EDITOR_ONLY
		{
			EString errorData("Save shader asset failed (output file path : ");
			errorData = errorData + savePath + ").";
			PE_FAILED((ENGINE_ASSET_ERROR), errorData);
		}
#endif
		return FALSE;
	}
	template<class TShaderAssetType>
	BOOL EShaderAssetManager::SaveShaderAsset(const EString& savePath, const TShaderAssetType* inShaderAsset)
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