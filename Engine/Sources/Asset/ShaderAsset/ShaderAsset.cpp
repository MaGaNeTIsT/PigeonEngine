#include "ShaderAsset.h"
#include <RenderDevice/DeviceD3D11.h>

namespace PigeonEngine
{

	template <typename _TAssetRenderResourceType, RShaderFrequencyType _ShaderFrequency, typename _AssetType>
	static void RegisterShaderClassTypes()
	{
		RegisterClassType<TRenderBaseAsset<EShaderResource, _TAssetRenderResourceType>, TBaseAsset<EShaderResource>>();
		RegisterClassType<TShaderBaseAsset<_ShaderFrequency, _TAssetRenderResourceType>, TRenderBaseAsset<EShaderResource, _TAssetRenderResourceType>>();
		RegisterClassType<_AssetType, TShaderBaseAsset<_ShaderFrequency, _TAssetRenderResourceType>>();
	}

	static void RegisterClassTypes()
	{
		RegisterClassType<EShaderResource, EResourceInterface>();
		RegisterClassType<TBaseAsset<EShaderResource>, EObjectBase>();

		RegisterShaderClassTypes<RVertexShaderResource, RShaderFrequencyType::SHADER_FREQUENCY_VERTEX, EVertexShaderAsset>();
		RegisterShaderClassTypes<RPixelShaderResource, RShaderFrequencyType::SHADER_FREQUENCY_PIXEL, EPixelShaderAsset>();
		RegisterShaderClassTypes<RComputeShaderResource, RShaderFrequencyType::SHADER_FREQUENCY_COMPUTE, EComputeShaderAsset>();

		RegisterClassType<EShaderAssetManager, EManagerBase>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

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
		RVertexShaderResource* Result = nullptr;
		if (InResource && InResource->ShaderByteCode && (InResource->ShaderByteCodeSize > 0u))
		{
			RDeviceD3D11* deviceD3D11 = RDeviceD3D11::GetDeviceSingleton();
			Result = new RVertexShaderResource();
			if (!deviceD3D11->CreateVertexShaderResource(InResource->ShaderByteCode, InResource->ShaderByteCodeSize, *Result, ShaderInputLayouts, ShaderInputLayoutNum))
			{
				delete Result;
				Result = nullptr;
			}
		}
		return Result;
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
		RPixelShaderResource* Result = nullptr;
		if (InResource && InResource->ShaderByteCode && (InResource->ShaderByteCodeSize > 0u))
		{
			RDeviceD3D11* deviceD3D11 = RDeviceD3D11::GetDeviceSingleton();
			Result = new RPixelShaderResource();
			if (!deviceD3D11->CreatePixelShaderResource(InResource->ShaderByteCode, InResource->ShaderByteCodeSize, *Result))
			{
				delete Result;
				Result = nullptr;
			}
		}
		return Result;
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
		RComputeShaderResource* Result = nullptr;
		if (InResource && InResource->ShaderByteCode && (InResource->ShaderByteCodeSize > 0u))
		{
			RDeviceD3D11* deviceD3D11 = RDeviceD3D11::GetDeviceSingleton();
			Result = new RComputeShaderResource();
			if (!deviceD3D11->CreateComputeShaderResource(InResource->ShaderByteCode, InResource->ShaderByteCodeSize, *Result))
			{
				delete Result;
				Result = nullptr;
			}
		}
		return Result;
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
	void EShaderAssetManager::Initialize()
	{
	}
	void EShaderAssetManager::ShutDown()
	{
	}
	BOOL EShaderAssetManager::ImportShaderCSO(const EString& InPath, const EString& OutPath, const RInputLayoutDesc* inShaderInputLayouts, const UINT* inShaderInputLayoutNum)
	{
		const static EString importShaderNameType(ENGINE_IMPORT_SHADER_NAME_TYPE);
		const static EString importVertexShaderNameType = EString(ENGINE_IMPORT_VERTEX_SHADER_NAME_TYPE) + importShaderNameType;
		const static EString importPixelShaderNameType = EString(ENGINE_IMPORT_PIXEL_SHADER_NAME_TYPE) + importShaderNameType;
		const static EString importComputeShaderNameType = EString(ENGINE_IMPORT_COMPUTE_SHADER_NAME_TYPE) + importShaderNameType;

		Check((ENGINE_ASSET_ERROR), ("Import shader cso. Input file path is too short."), InPath.Length() > importShaderNameType.Length());
		Check((ENGINE_ASSET_ERROR), ("Import shader cso. Input vs file path is too short."), InPath.Length() > importVertexShaderNameType.Length());
		Check((ENGINE_ASSET_ERROR), ("Import shader cso. Input ps file path is too short."), InPath.Length() > importPixelShaderNameType.Length());
		Check((ENGINE_ASSET_ERROR), ("Import shader cso. Input cs file path is too short."), InPath.Length() > importComputeShaderNameType.Length());

		if (InPath.Substring(InPath.Length() - importShaderNameType.Length(), importShaderNameType.Length()) != importShaderNameType)
		{
#ifdef _EDITOR_ONLY
			{
				EString ErrorData("Error file type for shader importer (input file path : ");
				ErrorData = ErrorData + InPath + ").";
				PE_FAILED((ENGINE_ASSET_ERROR), ErrorData);
			}
#endif
			return FALSE;
		}

		RShaderFrequencyType importShaderFrequency = RShaderFrequencyType::SHADER_FREQUENCY_COUNT;
		if (InPath.Substring(InPath.Length() - importPixelShaderNameType.Length(), importPixelShaderNameType.Length()) == importPixelShaderNameType)
		{
			importShaderFrequency = RShaderFrequencyType::SHADER_FREQUENCY_PIXEL;
		}
		else if (InPath.Substring(InPath.Length() - importVertexShaderNameType.Length(), importVertexShaderNameType.Length()) == importVertexShaderNameType)
		{
			if (!inShaderInputLayouts || !inShaderInputLayoutNum)
			{
				PE_FAILED((ENGINE_ASSET_ERROR), ("Error, try to import vertex shader without input layouts."));
				return FALSE;
			}
			importShaderFrequency = RShaderFrequencyType::SHADER_FREQUENCY_VERTEX;
		}
		else if (InPath.Substring(InPath.Length() - importComputeShaderNameType.Length(), importComputeShaderNameType.Length()) == importComputeShaderNameType)
		{
			importShaderFrequency = RShaderFrequencyType::SHADER_FREQUENCY_COMPUTE;
		}
		else
		{
#ifdef _EDITOR_ONLY
			{
				EString ErrorData("Not support type for shader import (input file path : ");
				ErrorData = ErrorData + InPath + ").";
				PE_FAILED((ENGINE_ASSET_ERROR), ErrorData);
			}
#endif
			return FALSE;
		}

		EShaderResource* outResource = new EShaderResource();
		if (!EFileHelper::ReadFileAsBinary(InPath, outResource->ShaderByteCode, outResource->ShaderByteCodeSize))
		{
			outResource->Release();
			delete outResource;
			return FALSE;
		}

		BOOL Result = SaveShaderAsset(OutPath, outResource, importShaderFrequency, inShaderInputLayouts, inShaderInputLayoutNum);

		outResource->Release();
		delete outResource;
		return Result;
	}
	BOOL EShaderAssetManager::LoadVertexShaderAsset(EString& InLoadPath, const EVertexShaderAsset*& outShaderAsset)
	{
		EVertexShaderAsset* resultShaderAsset = VertexShaderManager.Find(InLoadPath);
		if (resultShaderAsset)
		{
			outShaderAsset = resultShaderAsset;
			return TRUE;
		}
		resultShaderAsset = LoadShaderAsset<EVertexShaderAsset>(InLoadPath);
		if (!resultShaderAsset)
		{
			return FALSE;
		}
		if (!resultShaderAsset->InitResource())
		{
			delete resultShaderAsset;
			return FALSE;
		}
		if (VertexShaderManager.Add(InLoadPath, resultShaderAsset, TRUE) == 0u)
		{
#ifdef _EDITOR_ONLY
			{
				EString errorInfo = EString("Vertex shader path = [") + InLoadPath + "] add into manager list failed.";
				PE_FAILED((ENGINE_ASSET_ERROR), errorInfo);
			}
#endif
			delete resultShaderAsset;
			return FALSE;
		}
		outShaderAsset = resultShaderAsset;
		return TRUE;
	}
	BOOL EShaderAssetManager::LoadPixelShaderAsset(EString& InLoadPath, const EPixelShaderAsset*& outShaderAsset)
	{
		EPixelShaderAsset* resultShaderAsset = PixelShaderManager.Find(InLoadPath);
		if (resultShaderAsset)
		{
			outShaderAsset = resultShaderAsset;
			return TRUE;
		}
		resultShaderAsset = LoadShaderAsset<EPixelShaderAsset>(InLoadPath);
		if (!resultShaderAsset)
		{
			return FALSE;
		}
		if (!resultShaderAsset->InitResource())
		{
			delete resultShaderAsset;
			return FALSE;
		}
		if (PixelShaderManager.Add(InLoadPath, resultShaderAsset, TRUE) == 0u)
		{
#ifdef _EDITOR_ONLY
			{
				EString errorInfo = EString("Pixel shader path = [") + InLoadPath + "] add into manager list failed.";
				PE_FAILED((ENGINE_ASSET_ERROR), errorInfo);
			}
#endif
			delete resultShaderAsset;
			return FALSE;
		}
		outShaderAsset = resultShaderAsset;
		return TRUE;
	}
	BOOL EShaderAssetManager::LoadComputeShaderAsset(EString& InLoadPath, const EComputeShaderAsset*& outShaderAsset)
	{
		EComputeShaderAsset* resultShaderAsset = ComputeShaderManager.Find(InLoadPath);
		if (resultShaderAsset)
		{
			outShaderAsset = resultShaderAsset;
			return TRUE;
		}
		resultShaderAsset = LoadShaderAsset<EComputeShaderAsset>(InLoadPath);
		if (!resultShaderAsset)
		{
			return FALSE;
		}
		if (!resultShaderAsset->InitResource())
		{
			delete resultShaderAsset;
			return FALSE;
		}
		if (ComputeShaderManager.Add(InLoadPath, resultShaderAsset, TRUE) == 0u)
		{
#ifdef _EDITOR_ONLY
			{
				EString errorInfo = EString("Pixel shader path = [") + InLoadPath + "] add into manager list failed.";
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
	TShaderAssetType* EShaderAssetManager::LoadShaderAsset(const EString& InLoadPath)
	{
		void* ReadFileMem = nullptr; ULONG ReadFileSize = 0u;
		if (!EFileHelper::ReadFileAsBinary(InLoadPath, ReadFileMem, ReadFileSize))
		{
			if (ReadFileMem)
			{
				delete[]ReadFileMem;
			}
#ifdef _EDITOR_ONLY
			{
				EString ErrorData("Load shader asset failed (load file path : ");
				ErrorData += InLoadPath;
				ErrorData += ").";
				PE_FAILED((ENGINE_ASSET_ERROR), (ErrorData));
			}
#endif
			return nullptr;
		}
		Check(ENGINE_ASSET_ERROR, "Error read shader asset file size are too small.", (ReadFileSize > (sizeof(INT) * 2u)));
		void* TempPtr = ReadFileMem;
		ULONG RstSize = ReadFileSize;
		EAssetType ReadAssetType = EAssetType::ASSET_TYPE_UNKNOWN;
		RShaderFrequencyType ReadShaderFrequencyType = RShaderFrequencyType::SHADER_FREQUENCY_COUNT;
		UINT ReadInputLayoutNum = 0u;
		{
			UINT32* SavedAssetTypePtr = (UINT32*)TempPtr;
			ReadAssetType = static_cast<EAssetType>(SavedAssetTypePtr[0]);
			TempPtr = (void*)(&(SavedAssetTypePtr[1]));
			RstSize = RstSize - sizeof(UINT32);
		}
		{
			UINT32* SavedAssetTypePtr = (UINT32*)TempPtr;
			UINT8 TempType = static_cast<UINT8>(SavedAssetTypePtr[0]);
			ReadShaderFrequencyType = static_cast<RShaderFrequencyType>(TempType);
			TempPtr = (void*)(&(SavedAssetTypePtr[1]));
			RstSize = RstSize - sizeof(UINT32);
		}
		{
			UINT32* SavedAssetTypePtr = (UINT32*)TempPtr;
			ReadInputLayoutNum = SavedAssetTypePtr[0];
			TempPtr = (void*)(&(SavedAssetTypePtr[1]));
			RstSize = RstSize - sizeof(UINT32);
		}
		TShaderAssetType* OutShaderAsset = nullptr;
		if ((ReadAssetType == EAssetType::ASSET_TYPE_SHADER) &&
			(ReadShaderFrequencyType < RShaderFrequencyType::SHADER_FREQUENCY_COUNT) &&
			(RstSize > 0u))
		{
			TArray<RInputLayoutDesc> TempInputLayouts;
			if (ReadInputLayoutNum > 0u)
			{
				TempInputLayouts.Resize(ReadInputLayoutNum);
				for (UINT LayoutIndex = 0u; LayoutIndex < ReadInputLayoutNum; LayoutIndex++)
				{
					RInputLayoutDesc* SavedAssetTypePtr = (RInputLayoutDesc*)TempPtr;
					TempInputLayouts[LayoutIndex] = SavedAssetTypePtr[0];
					TempPtr = (void*)(&(SavedAssetTypePtr[1]));
					RstSize = RstSize - sizeof(RInputLayoutDesc);
				}
			}
			if (ReadShaderFrequencyType == RShaderFrequencyType::SHADER_FREQUENCY_VERTEX)
			{
				EVertexShaderAsset* TempShaderAsset = new EVertexShaderAsset(InLoadPath
#ifdef _EDITOR_ONLY
					, InLoadPath
#endif
					, TempInputLayouts.RawData(), ReadInputLayoutNum);
				OutShaderAsset = dynamic_cast<TShaderAssetType*>(TempShaderAsset);
				if (!OutShaderAsset)
				{
					TempShaderAsset->UninitResource();
					delete TempShaderAsset;
					TempShaderAsset = nullptr;
				}
			}
			else
			{
				OutShaderAsset = new TShaderAssetType(InLoadPath
#ifdef _EDITOR_ONLY
					, InLoadPath
#endif
				);
			}
			if (OutShaderAsset)
			{
				EShaderResource* StoragedResource = nullptr;
				if (!(OutShaderAsset->StorageResourceInternal(
					[TempPtr, RstSize, &StoragedResource]()->EShaderResource*
					{
						void* TempShaderCSO = new BYTE[RstSize];
						::memcpy_s(TempShaderCSO, RstSize, TempPtr, RstSize);
						StoragedResource = new EShaderResource();
						StoragedResource->ShaderByteCode = TempShaderCSO;
						StoragedResource->ShaderByteCodeSize = RstSize;
						return StoragedResource;
					})))
				{
					if (StoragedResource)
					{
						StoragedResource->Release();
						delete StoragedResource;
					}
				}
			}
		}
		delete[]ReadFileMem;
		return OutShaderAsset;
	}
	BOOL EShaderAssetManager::SaveShaderAsset(const EString& InSavePath, const EShaderResource* InShaderResource, RShaderFrequencyType InShaderFrequency, const RInputLayoutDesc* InShaderInputLayouts, const UINT* InShaderInputLayoutNum)
	{
		if (InShaderResource && InShaderResource->ShaderByteCode && (InShaderResource->ShaderByteCodeSize > 0u))
		{
			const UINT32 shaderFrequency = static_cast<UINT32>(InShaderFrequency);
			UINT32 ShaderInputLayoutNum = 0u;
			ULONG ShaderSaveSize = sizeof(UINT32) + sizeof(UINT32) + sizeof(UINT32) + InShaderResource->ShaderByteCodeSize;
			switch (InShaderFrequency)
			{
			case RShaderFrequencyType::SHADER_FREQUENCY_PIXEL:
			{
				//TODO We do not separate pixel & compute shader save and load for now.
			}
			break;
			case RShaderFrequencyType::SHADER_FREQUENCY_VERTEX:
			{
				if (!InShaderInputLayouts || !InShaderInputLayoutNum)
				{
					PE_FAILED((ENGINE_ASSET_ERROR), ("Try to save vertex shader asset, but do not contain input layouts."));
					return FALSE;
				}
				ShaderInputLayoutNum = static_cast<UINT32>(*InShaderInputLayoutNum);
				ShaderSaveSize += sizeof(RInputLayoutDesc) * static_cast<ULONG>(ShaderInputLayoutNum);
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
			void* SaveMem = new BYTE[ShaderSaveSize];
			void* TempPtr = SaveMem;
			ULONG RstSize = ShaderSaveSize;
			{
				UINT32* SavedAssetTypePtr = (UINT32*)TempPtr;
				SavedAssetTypePtr[0] = EAssetType::ASSET_TYPE_SHADER;
				TempPtr = (void*)(&(SavedAssetTypePtr[1]));
				RstSize = RstSize - sizeof(UINT32);
			}
			{
				UINT32* SavedAssetTypePtr = (UINT32*)TempPtr;
				SavedAssetTypePtr[0] = shaderFrequency;
				TempPtr = (void*)(&(SavedAssetTypePtr[1]));
				RstSize = RstSize - sizeof(UINT32);
			}
			{
				UINT32* SavedAssetTypePtr = (UINT32*)TempPtr;
				SavedAssetTypePtr[0] = ShaderInputLayoutNum;
				TempPtr = (void*)(&(SavedAssetTypePtr[1]));
				RstSize = RstSize - sizeof(UINT32);
			}
			if (ShaderInputLayoutNum > 0u)
			{
				const UINT ShaderInputLayoutDescSize = sizeof(RInputLayoutDesc) * static_cast<ULONG>(ShaderInputLayoutNum);
				::memcpy_s(TempPtr, RstSize, InShaderInputLayouts, ShaderInputLayoutDescSize);
				RInputLayoutDesc* SavedAssetTypePtr = (RInputLayoutDesc*)TempPtr;
				TempPtr = (void*)(&(SavedAssetTypePtr[ShaderInputLayoutNum]));
				RstSize = RstSize - ShaderInputLayoutDescSize;
			}
			Check((ENGINE_ASSET_ERROR), ("New copy buffer can not contain shader code."), RstSize == InShaderResource->ShaderByteCodeSize);
			::memcpy_s(TempPtr, RstSize, InShaderResource->ShaderByteCode, InShaderResource->ShaderByteCodeSize);

			if (EFileHelper::SaveBytesToFile(InSavePath, SaveMem, ShaderSaveSize))
			{
				delete[]SaveMem;
				return TRUE;
			}

			delete[]SaveMem;
		}

#ifdef _EDITOR_ONLY
		{
			EString ErrorData("Save shader asset failed (output file path : ");
			ErrorData = ErrorData + InSavePath + ").";
			PE_FAILED((ENGINE_ASSET_ERROR), ErrorData);
		}
#endif
		return FALSE;
	}
	template<class TShaderAssetType>
	BOOL EShaderAssetManager::SaveShaderAsset(const EString& InSavePath, const TShaderAssetType* InShaderAsset)
	{
		if (!InShaderAsset)
		{
			PE_FAILED(ENGINE_ASSET_ERROR, "Error try to save a null shader asset.");
			return FALSE;
		}
		const EShaderResource* saveShaderResource = InShaderAsset->GetStoragedResource();
		if (!saveShaderResource)
		{
			PE_FAILED(ENGINE_ASSET_ERROR, "Error try to save a shader asset without resource.");
			return FALSE;
		}
		RShaderFrequencyType saveShaderFrequency = InShaderAsset->GetShaderFrequency();
		const RInputLayoutDesc* saveShaderInputLayouts = nullptr;
		UINT saveShaderInputLayoutNum = 0u;
		if (saveShaderFrequency == RShaderFrequencyType::SHADER_FREQUENCY_VERTEX)
		{
			const EVertexShaderAsset* tempVertexShaderAsset = dynamic_cast<const EVertexShaderAsset*>(InShaderAsset);
			if (!tempVertexShaderAsset)
			{
				PE_FAILED(ENGINE_ASSET_ERROR, "Error try to saving a shader asset frequency is vertex, but actually is not vertex shader asset.");
				return FALSE;
			}
			saveShaderInputLayouts = tempVertexShaderAsset->GetShaderInputLayouts();
			saveShaderInputLayoutNum = tempVertexShaderAsset->GetShaderInputLayoutNum();
		}
		if (SaveShaderAsset(InSavePath, saveShaderResource, saveShaderFrequency, saveShaderInputLayouts, &saveShaderInputLayoutNum))
		{
			return TRUE;
		}
		return FALSE;
	}

};