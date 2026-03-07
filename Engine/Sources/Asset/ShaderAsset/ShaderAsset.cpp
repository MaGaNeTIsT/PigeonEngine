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
		RegisterClassType<EShaderResource, ERTTIObject, EResourceInterface>();

		RegisterClassType<TBaseAsset<EShaderResource>, EObjectBase>();

		RegisterShaderClassTypes<RVertexShaderResource, RShaderFrequencyType::SHADER_FREQUENCY_VERTEX, EVertexShaderAsset>();
		RegisterShaderClassTypes<RPixelShaderResource, RShaderFrequencyType::SHADER_FREQUENCY_PIXEL, EPixelShaderAsset>();
		RegisterShaderClassTypes<RComputeShaderResource, RShaderFrequencyType::SHADER_FREQUENCY_COMPUTE, EComputeShaderAsset>();

		RegisterClassType<EShaderAssetManager, EManagerBase>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	EVertexShaderAsset::EVertexShaderAsset(const EString& InAssetPath, const EString& InAssetName
#if _EDITOR_ONLY
		, const EString& InDebugName
#endif
		, const RInputLayoutDesc* InInputLayouts
		, const UINT32& InInputLayoutNum)
		: TShaderBaseAsset<RShaderFrequencyType::SHADER_FREQUENCY_VERTEX, RVertexShaderResource>(InAssetPath, InAssetName
#if _EDITOR_ONLY
			, InDebugName
#endif
		), ShaderInputLayouts(nullptr), ShaderInputLayoutNum(0u)
	{
		if (InInputLayouts && InInputLayoutNum > 0u)
		{
			ShaderInputLayouts = new RInputLayoutDesc[InInputLayoutNum];
			for (UINT32 i = 0u; i < InInputLayoutNum; i++)
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
	BOOL32 EVertexShaderAsset::InitResource()
	{
		if (!ShaderInputLayouts || ShaderInputLayoutNum == 0u)
		{
#if _EDITOR_ONLY
			{
				EString ErrorInfo = EString("Vertex shader name=[") + GetAssetName() + "] path = [" + GetAssetPath() + "] does not contain input layouts when init resource.";
				PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorInfo));
			}
#endif
			return FALSE;
		}
		if (IsInitialized())
		{
#if _EDITOR_ONLY
			{
				EString ErrorInfo = EString("Vertex shader name=[") + GetAssetName() + "] path = [" + GetAssetPath() + "] has been Initialized.";
				PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorInfo));
			}
#endif
			return TRUE;
		}
		if (CreateRenderResourceInternal(
			[this](EShaderResource* InResource)->RVertexShaderResource*
			{
				return (this->CreateShaderRenderResource(InResource));
			}, FALSE, [this](EShaderResource* InResource)->void
			{
				this->ReleaseResourceInternal();
			}))
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
			RDeviceD3D11* DeviceD3D11 = RDeviceD3D11::GetDeviceSingleton();
			Result = new RVertexShaderResource();
#if _EDITOR_ONLY
			if (ShaderInputLayoutNum > 0u)
#endif
			{
				Result->RawLayouts = new RInputLayoutDesc[ShaderInputLayoutNum];
				Result->LayoutNum = ShaderInputLayoutNum;
				for (UINT32 LayoutIndex = 0u; LayoutIndex < ShaderInputLayoutNum; LayoutIndex++)
				{
					Result->RawLayouts[LayoutIndex] = ShaderInputLayouts[LayoutIndex];
				}
			}
			if (!DeviceD3D11->CreateVertexShaderResource(InResource->ShaderByteCode, InResource->ShaderByteCodeSize, *Result, ShaderInputLayouts, ShaderInputLayoutNum))
			{
				delete Result;
				Result = nullptr;
			}
		}
		return Result;
	}

	EPixelShaderAsset::EPixelShaderAsset(const EString& InAssetPath, const EString& InAssetName
#if _EDITOR_ONLY
		, const EString& InDebugName
#endif
	) : TShaderBaseAsset<RShaderFrequencyType::SHADER_FREQUENCY_PIXEL, RPixelShaderResource>(InAssetPath, InAssetName
#if _EDITOR_ONLY
		, InDebugName
#endif
	)
	{
	}
	EPixelShaderAsset::~EPixelShaderAsset()
	{
	}
	BOOL32 EPixelShaderAsset::InitResource()
	{
		if (IsInitialized())
		{
#if _EDITOR_ONLY
			{
				EString ErrorInfo = EString("Vertex shader name=[") + GetAssetName() + "] path = [" + GetAssetPath() + "] has been Initialized.";
				PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorInfo));
			}
#endif
			return TRUE;
		}
		if (CreateRenderResourceInternal(
			[this](EShaderResource* InResource)->RPixelShaderResource*
			{
				return (this->CreateShaderRenderResource(InResource));
			}, FALSE, [this](EShaderResource* InResource)->void
			{
				this->ReleaseResourceInternal();
			}))
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
			RDeviceD3D11* DeviceD3D11 = RDeviceD3D11::GetDeviceSingleton();
			Result = new RPixelShaderResource();
			if (!DeviceD3D11->CreatePixelShaderResource(InResource->ShaderByteCode, InResource->ShaderByteCodeSize, *Result))
			{
				delete Result;
				Result = nullptr;
			}
		}
		return Result;
	}

	EComputeShaderAsset::EComputeShaderAsset(const EString& InAssetPath, const EString& InAssetName
#if _EDITOR_ONLY
		, const EString& InDebugName
#endif
	) : TShaderBaseAsset<RShaderFrequencyType::SHADER_FREQUENCY_COMPUTE, RComputeShaderResource>(InAssetPath, InAssetName
#if _EDITOR_ONLY
		, InDebugName
#endif
	)
	{
	}
	EComputeShaderAsset::~EComputeShaderAsset()
	{
	}
	BOOL32 EComputeShaderAsset::InitResource()
	{
		if (IsInitialized())
		{
#if _EDITOR_ONLY
			{
				EString ErrorInfo = EString("Vertex shader name=[") + GetAssetName() + "] path = [" + GetAssetPath() + "] has been Initialized.";
				PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorInfo));
			}
#endif
			return TRUE;
		}
		if (CreateRenderResourceInternal(
			[this](EShaderResource* InResource)->RComputeShaderResource*
			{
				return (this->CreateShaderRenderResource(InResource));
			}, FALSE, [this](EShaderResource* InResource)->void
			{
				this->ReleaseResourceInternal();
			}))
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
			RDeviceD3D11* DeviceD3D11 = RDeviceD3D11::GetDeviceSingleton();
			Result = new RComputeShaderResource();
			if (!DeviceD3D11->CreateComputeShaderResource(InResource->ShaderByteCode, InResource->ShaderByteCodeSize, *Result))
			{
				delete Result;
				Result = nullptr;
			}
		}
		return Result;
	}

	EShaderAssetManager::EShaderAssetManager()
	{
#if _EDITOR_ONLY
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
		ClearVertexShaders();
		ClearPixelShaders();
		ClearComputeShaders();
	}
#if _EDITOR_ONLY
	BOOL32 EShaderAssetManager::ImportVertexShader(const EString& InAssetName, const EString& InImportFullPathName, const EString& InSavePath, const RInputLayoutDesc* InShaderInputLayouts, const UINT32* InShaderInputLayoutNum)
	{
		if ((!InShaderInputLayouts) || (!InShaderInputLayoutNum) || ((*InShaderInputLayoutNum) < 1u))
		{
			PE_FAILED((ENGINE_ASSET_ERROR), ("Error try to import vertex shader without input layouts."));
			return FALSE;
		}
		EString TempFullSavePathName(InSavePath);
		TempFullSavePathName = TempFullSavePathName + InAssetName + EEngineSettings::ENGINE_ASSET_NAME_TYPE;
		if ((InImportFullPathName.Length() < 3u) || (TempFullSavePathName.Length() < 10u))
		{
#if _EDITOR_ONLY
			{
				EString ErrorData("Import shader asset error (import file path : ");
				ErrorData = ErrorData + InImportFullPathName + ", save assset path : " + TempFullSavePathName + ").";
				PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorData));
			}
#endif
			return FALSE;
		}
		EString ImportPathName; EString ImportFileType;
		if (!(SplitByLastSign('.', InImportFullPathName, ImportPathName, ImportFileType)))
		{
#if _EDITOR_ONLY
			{
				EString ErrorData("Error file path for shader importer (import file path : ");
				ErrorData = ErrorData + InImportFullPathName + ", save assset path : " + TempFullSavePathName + ").";
				PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorData));
			}
#endif
			return FALSE;
		}
		if (EString TempShaderNameType = ImportPathName.Substring(ImportPathName.Length() - 4u, 4u);
			((EString(".") + ImportFileType) != EString(EEngineSettings::ENGINE_IMPORT_SHADER_NAME_TYPE)) ||
			(TempShaderNameType != EString(EEngineSettings::ENGINE_IMPORT_VERTEX_SHADER_NAME_TYPE)))
		{
#if _EDITOR_ONLY
			{
				EString ErrorData("Error file type for shader importer (import shader name type : ");
				ErrorData = ErrorData + TempShaderNameType + ", import file type : " + ImportFileType + ").";
				PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorData));
			}
#endif
			return FALSE;
		}
		EShaderResource* OutResource = new EShaderResource();
		if (!EFileHelper::ReadFileAsBinary(InImportFullPathName, OutResource->ShaderByteCode, OutResource->ShaderByteCodeSize))
		{
			OutResource->ReleaseResource();
			delete OutResource;
			return FALSE;
		}

		BOOL32 Result = SaveShaderResource(InSavePath, InAssetName, OutResource, RShaderFrequencyType::SHADER_FREQUENCY_VERTEX, InShaderInputLayouts, InShaderInputLayoutNum);

		OutResource->ReleaseResource();
		delete OutResource;
		return Result;
	}
	BOOL32 EShaderAssetManager::ImportPixelShader(const EString& InAssetName, const EString& InImportFullPathName, const EString& InSavePath)
	{
		EString TempFullSavePathName(InSavePath);
		TempFullSavePathName = TempFullSavePathName + InAssetName + EEngineSettings::ENGINE_ASSET_NAME_TYPE;
		if ((InImportFullPathName.Length() < 3u) || (TempFullSavePathName.Length() < 10u))
		{
#if _EDITOR_ONLY
			{
				EString ErrorData("Import shader asset error (import file path : ");
				ErrorData = ErrorData + InImportFullPathName + ", save assset path : " + TempFullSavePathName + ").";
				PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorData));
			}
#endif
			return FALSE;
		}
		EString ImportPathName; EString ImportFileType;
		if (!(SplitByLastSign('.', InImportFullPathName, ImportPathName, ImportFileType)))
		{
#if _EDITOR_ONLY
			{
				EString ErrorData("Error file path for shader importer (import file path : ");
				ErrorData = ErrorData + InImportFullPathName + ", save assset path : " + TempFullSavePathName + ").";
				PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorData));
			}
#endif
			return FALSE;
		}
		if (EString TempShaderNameType = ImportPathName.Substring(ImportPathName.Length() - 4u, 4u);
			((EString(".") + ImportFileType) != EString(EEngineSettings::ENGINE_IMPORT_SHADER_NAME_TYPE)) ||
			(TempShaderNameType != EString(EEngineSettings::ENGINE_IMPORT_PIXEL_SHADER_NAME_TYPE)))
		{
#if _EDITOR_ONLY
			{
				EString ErrorData("Error file type for shader importer (import shader name type : ");
				ErrorData = ErrorData + TempShaderNameType + ", import file type : " + ImportFileType + ").";
				PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorData));
			}
#endif
			return FALSE;
		}
		EShaderResource* OutResource = new EShaderResource();
		if (!EFileHelper::ReadFileAsBinary(InImportFullPathName, OutResource->ShaderByteCode, OutResource->ShaderByteCodeSize))
		{
			OutResource->ReleaseResource();
			delete OutResource;
			return FALSE;
		}

		BOOL32 Result = SaveShaderResource(InSavePath, InAssetName, OutResource, RShaderFrequencyType::SHADER_FREQUENCY_PIXEL);

		OutResource->ReleaseResource();
		delete OutResource;
		return Result;
	}
	BOOL32 EShaderAssetManager::ImportComputeShader(const EString& InAssetName, const EString& InImportFullPathName, const EString& InSavePath)
	{
		EString TempFullSavePathName(InSavePath);
		TempFullSavePathName = TempFullSavePathName + InAssetName + EEngineSettings::ENGINE_ASSET_NAME_TYPE;
		if ((InImportFullPathName.Length() < 3u) || (TempFullSavePathName.Length() < 10u))
		{
#if _EDITOR_ONLY
			{
				EString ErrorData("Import shader asset error (import file path : ");
				ErrorData = ErrorData + InImportFullPathName + ", save assset path : " + TempFullSavePathName + ").";
				PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorData));
			}
#endif
			return FALSE;
		}
		EString ImportPathName; EString ImportFileType;
		if (!(SplitByLastSign('.', InImportFullPathName, ImportPathName, ImportFileType)))
		{
#if _EDITOR_ONLY
			{
				EString ErrorData("Error file path for shader importer (import file path : ");
				ErrorData = ErrorData + InImportFullPathName + ", save assset path : " + TempFullSavePathName + ").";
				PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorData));
			}
#endif
			return FALSE;
		}
		if (EString TempShaderNameType = ImportPathName.Substring(ImportPathName.Length() - 4u, 4u);
			((EString(".") + ImportFileType) != EString(EEngineSettings::ENGINE_IMPORT_SHADER_NAME_TYPE)) ||
			(TempShaderNameType != EString(EEngineSettings::ENGINE_IMPORT_COMPUTE_SHADER_NAME_TYPE)))
		{
#if _EDITOR_ONLY
			{
				EString ErrorData("Error file type for shader importer (import shader name type : ");
				ErrorData = ErrorData + TempShaderNameType + ", import file type : " + ImportFileType + ").";
				PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorData));
			}
#endif
			return FALSE;
		}
		EShaderResource* OutResource = new EShaderResource();
		if (!EFileHelper::ReadFileAsBinary(InImportFullPathName, OutResource->ShaderByteCode, OutResource->ShaderByteCodeSize))
		{
			OutResource->ReleaseResource();
			delete OutResource;
			return FALSE;
		}

		BOOL32 Result = SaveShaderResource(InSavePath, InAssetName, OutResource, RShaderFrequencyType::SHADER_FREQUENCY_COMPUTE);

		OutResource->ReleaseResource();
		delete OutResource;
		return Result;
	}
#endif
	BOOL32 EShaderAssetManager::LoadVertexShaderAsset(const EString& InLoadPath, const EString& InLoadName, const EVertexShaderAsset*& OutShaderAsset)
	{
		EString TempFullLoadPathName(InLoadPath);
		TempFullLoadPathName = TempFullLoadPathName + InLoadName + EEngineSettings::ENGINE_ASSET_NAME_TYPE;
		EVertexShaderAsset* ResultShaderAsset = VertexShaderManager.Find(TempFullLoadPathName);
		if (ResultShaderAsset)
		{
			OutShaderAsset = ResultShaderAsset;
			return TRUE;
		}
		ResultShaderAsset = LoadShaderAsset<EVertexShaderAsset>(InLoadPath, InLoadName);
		if (!ResultShaderAsset)
		{
			return FALSE;
		}
		if (!ResultShaderAsset->InitResource())
		{
			delete ResultShaderAsset;
			return FALSE;
		}
		if (VertexShaderManager.Add(TempFullLoadPathName, ResultShaderAsset, TRUE) == 0u)
		{
#if _EDITOR_ONLY
			{
				EString ErrorInfo = EString("Vertex shader path = [") + TempFullLoadPathName + "] add into manager list failed.";
				PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorInfo));
			}
#endif
			delete ResultShaderAsset;
			return FALSE;
		}
		OutShaderAsset = ResultShaderAsset;
		return TRUE;
	}
	BOOL32 EShaderAssetManager::LoadPixelShaderAsset(const EString& InLoadPath, const EString& InLoadName, const EPixelShaderAsset*& OutShaderAsset)
	{
		EString TempFullLoadPathName(InLoadPath);
		TempFullLoadPathName = TempFullLoadPathName + InLoadName + EEngineSettings::ENGINE_ASSET_NAME_TYPE;
		EPixelShaderAsset* ResultShaderAsset = PixelShaderManager.Find(TempFullLoadPathName);
		if (ResultShaderAsset)
		{
			OutShaderAsset = ResultShaderAsset;
			return TRUE;
		}
		ResultShaderAsset = LoadShaderAsset<EPixelShaderAsset>(InLoadPath, InLoadName);
		if (!ResultShaderAsset)
		{
			return FALSE;
		}
		if (!ResultShaderAsset->InitResource())
		{
			delete ResultShaderAsset;
			return FALSE;
		}
		if (PixelShaderManager.Add(TempFullLoadPathName, ResultShaderAsset, TRUE) == 0u)
		{
#if _EDITOR_ONLY
			{
				EString ErrorInfo = EString("Pixel shader path = [") + TempFullLoadPathName + "] add into manager list failed.";
				PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorInfo));
			}
#endif
			delete ResultShaderAsset;
			return FALSE;
		}
		OutShaderAsset = ResultShaderAsset;
		return TRUE;
	}
	BOOL32 EShaderAssetManager::LoadComputeShaderAsset(const EString& InLoadPath, const EString& InLoadName, const EComputeShaderAsset*& OutShaderAsset)
	{
		EString TempFullLoadPathName(InLoadPath);
		TempFullLoadPathName = TempFullLoadPathName + InLoadName + EEngineSettings::ENGINE_ASSET_NAME_TYPE;
		EComputeShaderAsset* ResultShaderAsset = ComputeShaderManager.Find(TempFullLoadPathName);
		if (ResultShaderAsset)
		{
			OutShaderAsset = ResultShaderAsset;
			return TRUE;
		}
		ResultShaderAsset = LoadShaderAsset<EComputeShaderAsset>(InLoadPath, InLoadName);
		if (!ResultShaderAsset)
		{
			return FALSE;
		}
		if (!ResultShaderAsset->InitResource())
		{
			delete ResultShaderAsset;
			return FALSE;
		}
		if (ComputeShaderManager.Add(TempFullLoadPathName, ResultShaderAsset, TRUE) == 0u)
		{
#if _EDITOR_ONLY
			{
				EString ErrorInfo = EString("Pixel shader path = [") + TempFullLoadPathName + "] add into manager list failed.";
				PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorInfo));
			}
#endif
			delete ResultShaderAsset;
			return FALSE;
		}
		OutShaderAsset = ResultShaderAsset;
		return TRUE;
	}
	void EShaderAssetManager::ClearVertexShaders()
	{
		VertexShaderManager.Clear();
	}
	void EShaderAssetManager::ClearPixelShaders()
	{
		PixelShaderManager.Clear();
	}
	void EShaderAssetManager::ClearComputeShaders()
	{
		ComputeShaderManager.Clear();
	}
	template<class TShaderAssetType>
	TShaderAssetType* EShaderAssetManager::LoadShaderAsset(const EString& InLoadPath, const EString& InLoadName)
	{
		EString TempFullLoadPathName(InLoadPath);
		TempFullLoadPathName = TempFullLoadPathName + InLoadName + EEngineSettings::ENGINE_ASSET_NAME_TYPE;
		if (TempFullLoadPathName.Length() < 10u)
		{
#if _EDITOR_ONLY
			{
				EString ErrorData("Load shader asset path name check failed (load file path : ");
				ErrorData += InLoadPath;
				ErrorData += ", load file name : ";
				ErrorData += InLoadName;
				ErrorData += ").";
				PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorData));
			}
#endif
			return nullptr;
		}
		void* ReadFileMem = nullptr; ULONG ReadFileSize = 0u;
		if (!EFileHelper::ReadFileAsBinary(TempFullLoadPathName, ReadFileMem, ReadFileSize))
		{
			if (ReadFileMem)
			{
				delete[]ReadFileMem;
			}
#if _EDITOR_ONLY
			{
				EString ErrorData("Load shader asset failed (load file path : ");
				ErrorData += TempFullLoadPathName;
				ErrorData += ").";
				PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorData));
			}
#endif
			return nullptr;
		}
		PE_CHECK((ENGINE_ASSET_ERROR), ("Error read shader asset file size are too small."), (ReadFileSize > (sizeof(UINT32) * 3u)));
		void* TempPtr = ReadFileMem;
		ULONG RstSize = ReadFileSize;
		EAssetType ReadAssetType = EAssetType::ASSET_TYPE_UNKNOWN;
		RShaderFrequencyType ReadShaderFrequencyType = RShaderFrequencyType::SHADER_FREQUENCY_COUNT;
		UINT32 ReadInputLayoutNum = 0u;
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
				TempInputLayouts.SetNum(ReadInputLayoutNum);
				for (UINT32 LayoutIndex = 0u; LayoutIndex < ReadInputLayoutNum; LayoutIndex++)
				{
					RInputLayoutDesc* SavedAssetTypePtr = (RInputLayoutDesc*)TempPtr;
					TempInputLayouts[LayoutIndex] = SavedAssetTypePtr[0];
					TempPtr = (void*)(&(SavedAssetTypePtr[1]));
					RstSize = RstSize - sizeof(RInputLayoutDesc);
				}
			}
			if (ReadShaderFrequencyType == RShaderFrequencyType::SHADER_FREQUENCY_VERTEX)
			{
				EVertexShaderAsset* TempShaderAsset = new EVertexShaderAsset(InLoadPath, InLoadName
#if _EDITOR_ONLY
					, InLoadName
#endif
					, TempInputLayouts.GetData(), ReadInputLayoutNum);
				OutShaderAsset = TempShaderAsset->AsType<TShaderAssetType>();
				if (!OutShaderAsset)
				{
					TempShaderAsset->UninitResource();
					delete TempShaderAsset;
					TempShaderAsset = nullptr;
				}
			}
			else
			{
				OutShaderAsset = new TShaderAssetType(InLoadPath, InLoadName
#if _EDITOR_ONLY
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
					//TODO
				}
			}
		}
		delete[]ReadFileMem;
		return OutShaderAsset;
	}
	BOOL32 EShaderAssetManager::SaveShaderResource(const EString& InSavePath, const EString& InSaveName, const EShaderResource* InShaderResource, RShaderFrequencyType InShaderFrequency, const RInputLayoutDesc* InShaderInputLayouts, const UINT32* InShaderInputLayoutNum)
	{
		EString TempFullSavePathName(InSavePath);
		TempFullSavePathName = TempFullSavePathName + InSaveName + EEngineSettings::ENGINE_ASSET_NAME_TYPE;
		if (TempFullSavePathName.Length() < 10u)
		{
#if _EDITOR_ONLY
			{
				EString ErrorData("Save shader asset path name check failed (save file path : ");
				ErrorData += InSavePath;
				ErrorData += ", save file name : ";
				ErrorData += InSaveName;
				ErrorData += ").";
				PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorData));
			}
#endif
			return FALSE;
		}
		if (InShaderResource && InShaderResource->ShaderByteCode && (InShaderResource->ShaderByteCodeSize > 0u))
		{
			const UINT32 ShaderFrequency = static_cast<UINT32>(InShaderFrequency);
			UINT32 ShaderInputLayoutNum = 0u;
			ULONG ShaderSaveSize = sizeof(UINT32) + sizeof(UINT32) + sizeof(UINT32) + InShaderResource->ShaderByteCodeSize;	// EAssetType + RShaderFrequencyType + ShaderInputLayoutNum + ShaderByteCodeSize
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
				SavedAssetTypePtr[0] = ShaderFrequency;
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
				const UINT32 ShaderInputLayoutDescSize = sizeof(RInputLayoutDesc) * static_cast<ULONG>(ShaderInputLayoutNum);
				::memcpy_s(TempPtr, RstSize, InShaderInputLayouts, ShaderInputLayoutDescSize);
				RInputLayoutDesc* SavedAssetTypePtr = (RInputLayoutDesc*)TempPtr;
				TempPtr = (void*)(&(SavedAssetTypePtr[ShaderInputLayoutNum]));
				RstSize = RstSize - ShaderInputLayoutDescSize;
			}
			PE_CHECK((ENGINE_ASSET_ERROR), ("New copy buffer can not contain shader code."), (RstSize == InShaderResource->ShaderByteCodeSize));
			::memcpy_s(TempPtr, RstSize, InShaderResource->ShaderByteCode, InShaderResource->ShaderByteCodeSize);

			if (EFileHelper::SaveBytesToFile(TempFullSavePathName, SaveMem, ShaderSaveSize))
			{
				delete[]SaveMem;
				return TRUE;
			}

			delete[]SaveMem;
		}

#if _EDITOR_ONLY
		{
			EString ErrorData("Save shader asset failed (output file path : ");
			ErrorData = ErrorData + TempFullSavePathName + ").";
			PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorData));
		}
#endif
		return FALSE;
	}
	template<class TShaderAssetType>
	BOOL32 EShaderAssetManager::SaveShaderAsset(const EString& InSavePath, const EString& InSaveName, const TShaderAssetType* InShaderAsset)
	{
		EString TempFullSavePathName(InSavePath);
		TempFullSavePathName = TempFullSavePathName + InSaveName + EEngineSettings::ENGINE_ASSET_NAME_TYPE;
		if (TempFullSavePathName.Length() < 10u)
		{
#if _EDITOR_ONLY
			{
				EString ErrorData("Save shader asset path name check failed (save file path : ");
				ErrorData += InSavePath;
				ErrorData += ", save file name : ";
				ErrorData += InSaveName;
				ErrorData += ").";
				PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorData));
			}
#endif
			return FALSE;
		}
		if (!InShaderAsset)
		{
			PE_FAILED((ENGINE_ASSET_ERROR), ("Error try to save a null shader asset."));
			return FALSE;
		}
		const EShaderResource* SaveShaderResource = InShaderAsset->GetStoragedResource();
		if ((!SaveShaderResource) || (!(SaveShaderResource->IsResourceValid())))
		{
			PE_FAILED((ENGINE_ASSET_ERROR), ("Error try to save a shader asset without resource."));
			return FALSE;
		}
		RShaderFrequencyType SaveShaderFrequency = InShaderAsset->GetShaderFrequency();
		const RInputLayoutDesc* SaveShaderInputLayouts = nullptr;
		UINT32 SaveShaderInputLayoutNum = 0u;
		if (SaveShaderFrequency == RShaderFrequencyType::SHADER_FREQUENCY_VERTEX)
		{
			const EVertexShaderAsset* TempVertexShaderAsset = InShaderAsset->AsType<EVertexShaderAsset>();
			if (!TempVertexShaderAsset)
			{
				PE_FAILED((ENGINE_ASSET_ERROR), ("Error try to saving a shader asset frequency is vertex, but actually is not vertex shader asset."));
				return FALSE;
			}
			SaveShaderInputLayouts = TempVertexShaderAsset->GetShaderInputLayouts();
			SaveShaderInputLayoutNum = TempVertexShaderAsset->GetShaderInputLayoutNum();
		}
		return (SaveShaderAsset(InSavePath, InSaveName, SaveShaderResource, SaveShaderFrequency, SaveShaderInputLayouts, &SaveShaderInputLayoutNum));
	}

	void TryLoadVertexShader(const EString& InLoadPath, const EString& InLoadName, const EVertexShaderAsset*& OutShaderAsset, const EString* InImportPath, const EString* InImportName, const RInputLayoutDesc* InShaderInputLayouts, const UINT32* InShaderInputLayoutNum)
	{
		EShaderAssetManager* ShaderAssetManager = EShaderAssetManager::GetManagerSingleton();
		PE_CHECK((ENGINE_ASSET_ERROR), ("Check output shader asset pointer is initialized failed."), (!OutShaderAsset));
		if (ShaderAssetManager->LoadVertexShaderAsset(InLoadPath, InLoadName, OutShaderAsset))
		{
			return;
		}
		PE_FAILED((ENGINE_ASSET_ERROR), ("Load vertex shader asset failed."));
#if _EDITOR_ONLY
		if ((!!InImportPath) && (!!InImportName) && (!!InShaderInputLayouts) && (!!InShaderInputLayoutNum) && ((*InShaderInputLayoutNum) > 0u))
		{
			TArray<RInputLayoutDesc> UsedShaderInputLayouts;
			for (UINT32 i = 0u, n = (*InShaderInputLayoutNum); i < n; i++)
			{
				RInputLayoutDesc& TempDesc = UsedShaderInputLayouts.AddDefaultGetRef();
				TempDesc = InShaderInputLayouts[i];
			}
			const UINT32 UsedShaderInputLayoutNum = UsedShaderInputLayouts.Num<UINT32>();
			EString TempImportFullPath(*InImportPath);
			TempImportFullPath = TempImportFullPath + (*InImportName) + EEngineSettings::ENGINE_IMPORT_SHADER_NAME_TYPE;
			BOOL32 Result = ShaderAssetManager->ImportVertexShader(
				InLoadName,
				TempImportFullPath,
				InLoadPath,
				UsedShaderInputLayouts.GetData(), &UsedShaderInputLayoutNum);
			if (Result)
			{
				if (!(ShaderAssetManager->LoadVertexShaderAsset(InLoadPath, InLoadName, OutShaderAsset)))
				{
					PE_FAILED((ENGINE_ASSET_ERROR), ("Try load vertex shader asset failed."));
				}
			}
			else
			{
				PE_FAILED((ENGINE_ASSET_ERROR), ("Try import vertex shader asset failed."));
			}
		}
#endif
	}
	void TryLoadPixelShader(const EString& InLoadPath, const EString& InLoadName, const EPixelShaderAsset*& OutShaderAsset, const EString* InImportPath, const EString* InImportName)
	{
		EShaderAssetManager* ShaderAssetManager = EShaderAssetManager::GetManagerSingleton();
		PE_CHECK((ENGINE_ASSET_ERROR), ("Check output shader asset pointer is initialized failed."), (!OutShaderAsset));
		if (ShaderAssetManager->LoadPixelShaderAsset(InLoadPath, InLoadName, OutShaderAsset))
		{
			return;
		}
		PE_FAILED((ENGINE_ASSET_ERROR), ("Load vertex shader asset failed."));
#if _EDITOR_ONLY
		if ((!!InImportPath) && (!!InImportName))
		{
			EString TempImportFullPath(*InImportPath);
			TempImportFullPath = TempImportFullPath + (*InImportName) + EEngineSettings::ENGINE_IMPORT_SHADER_NAME_TYPE;
			BOOL32 Result = ShaderAssetManager->ImportPixelShader(
				InLoadName,
				TempImportFullPath,
				InLoadPath);
			if (Result)
			{
				if (!(ShaderAssetManager->LoadPixelShaderAsset(InLoadPath, InLoadName, OutShaderAsset)))
				{
					PE_FAILED((ENGINE_ASSET_ERROR), ("Try load vertex shader asset failed."));
				}
			}
			else
			{
				PE_FAILED((ENGINE_ASSET_ERROR), ("Try import vertex shader asset failed."));
			}
		}
#endif
	}

};