#include "MCDxcCompiler.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <Unknwn.h>
#include <dxcapi.h>
#include <wrl/client.h>
#include <d3dcompiler.h>
#include <d3d11shader.h>
#include <d3d12shader.h>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <unordered_map>


namespace MC
{

	// ---------------------------------------------------------------------------
	// ID3DInclude that handles relative paths (../ and ./) correctly by tracking
	// each opened file's directory and using it to resolve child includes.
	// ---------------------------------------------------------------------------
	struct D3DIncludeHandler : public ID3DInclude
	{
		std::vector<std::string>							SearchDirs;
		std::unordered_map<const void*, std::string>		DataToDir;		// raw ptr -> directory of that file

		HRESULT __stdcall Open(D3D_INCLUDE_TYPE InIncludeType, LPCSTR InFileName, LPCVOID InParentData, LPCVOID* InDataPtr, UINT* InBytes) override
		{
			std::vector<std::string> DirsToTry;

			// Local include: try parent file's directory first so relative paths work
			if (InIncludeType == D3D_INCLUDE_LOCAL && InParentData != nullptr)
			{
				auto Iter = DataToDir.find(InParentData);
				if (Iter != DataToDir.end())
				{
					DirsToTry.push_back(Iter->second);
				}
			}
			for (auto& SearchDir : SearchDirs)
			{
				DirsToTry.push_back(SearchDir);
			}

			for (auto& Dir : DirsToTry)
			{
				std::string RawStr = Dir;
				if (!RawStr.empty() && RawStr.back() != '/' && RawStr.back() != '\\')
				{
					RawStr += '/';
				}
				RawStr += InFileName;

				// Normalise so that ../ and ./ are resolved
				std::error_code ErrorCode;
				std::filesystem::path Canonical = std::filesystem::weakly_canonical(RawStr, ErrorCode);
				std::string FullPath = ErrorCode ? RawStr : Canonical.string();

				std::ifstream InputFileStream(FullPath, std::ios::binary);
				if (!InputFileStream.is_open())
				{
					continue;
				}

				std::string Content((std::istreambuf_iterator<char>(InputFileStream)), std::istreambuf_iterator<char>());
				char* Buff = new char[Content.size()];
				memcpy(Buff, Content.data(), Content.size());
				*InDataPtr = Buff;
				*InBytes = static_cast<UINT>(Content.size());

				// Remember this file's directory so its children can resolve relative paths
				DataToDir[Buff] = std::filesystem::path(FullPath).parent_path().string();
				return S_OK;
			}
			return E_FAIL;
		}

		HRESULT __stdcall Close(LPCVOID InData) override
		{
			DataToDir.erase(InData);
			delete[] (static_cast<const char*>(InData));
			return S_OK;
		}
	};

	static std::wstring ToWideString(const std::string& InStr) noexcept
	{
		std::wstring WStr(InStr.begin(), InStr.end());
		return WStr;
	}

	static std::string MakeHResultText(HRESULT InHandleResult) noexcept
	{
		std::ostringstream StrStream;
		StrStream << "0x" << std::hex << std::setw(8) << std::setfill('0') << static_cast<uint32_t>(InHandleResult);
		return StrStream.str();
	}

	// ---------------------------------------------------------------------------
	// Constant buffer layout - taken from the compiler reflection of the produced
	// container, so the cpu side mirrors the packing rules the shader was built with.
	// ---------------------------------------------------------------------------

	// Array elements inside a constant buffer are laid out with a 16 byte stride, and the
	// compiler reports the byte span from the variable start to the end of the last element
	// content (the trailing slot padding of the last element is excluded). The element
	// content size below reproduces that span, which lets the stride be derived and verified
	// instead of being guessed.
	static uint32_t RoundUp16(uint32_t InValue) noexcept
	{
		return ((InValue + 15u) & (~15u));
	}

	static uint32_t GetCbElementContentSize(D3D_SHADER_VARIABLE_CLASS InClass, uint32_t InRows, uint32_t InColumns) noexcept
	{
		if ((InRows == 0u) || (InColumns == 0u))
		{
			return 0u;
		}
		switch (InClass)
		{
			case D3D_SVC_SCALAR:
			{
				return 4u;
			}
			case D3D_SVC_VECTOR:
			{
				return 4u * InColumns;
			}
			case D3D_SVC_MATRIX_COLUMNS:
			{
				return (16u * (InColumns - 1u)) + (4u * InRows);
			}
			case D3D_SVC_MATRIX_ROWS:
			{
				return (16u * (InRows - 1u)) + (4u * InColumns);
			}
		}
		return 0u;
	}

	static bool FillCbArrayStride(const std::string& InVariableName, D3D_SHADER_VARIABLE_CLASS InClass, uint32_t InRows, uint32_t InColumns, uint32_t InElements, uint32_t InContentBytes, uint32_t& OutArrayStride, std::string& OutError) noexcept
	{
		OutArrayStride = 0;
		if (InElements <= 1u)
		{
			return true;
		}
		const uint32_t ElementContentSize = GetCbElementContentSize(InClass, InRows, InColumns);
		const uint32_t ElementStride = RoundUp16(ElementContentSize);
		if ((ElementContentSize == 0u) || (InContentBytes != ((InElements - 1u) * ElementStride + ElementContentSize)))
		{
			OutError = "cannot derive the array stride of constant buffer variable '" + InVariableName + "' (elements = " + std::to_string(InElements) + ", content bytes = " + std::to_string(InContentBytes) + ")";
			return false;
		}
		OutArrayStride = ElementStride;
		return true;
	}

	// Bound resources - what the compiled shader actually references. Unused declarations are
	// removed by the compiler, so this list is how "declared but culled" is detected.
	static bool ToBindingKind(D3D_SHADER_INPUT_TYPE InType, MCDxcBindingKind& OutKind) noexcept
	{
		switch (InType)
		{
			case D3D_SIT_CBUFFER:
			{
				OutKind = MCDxcBindingKind::Binding_CBuffer;
				return true;
			}
			case D3D_SIT_SAMPLER:
			{
				OutKind = MCDxcBindingKind::Binding_Sampler;
				return true;
			}
			case D3D_SIT_TEXTURE:
			case D3D_SIT_TBUFFER:
			case D3D_SIT_STRUCTURED:
			case D3D_SIT_BYTEADDRESS:
			{
				OutKind = MCDxcBindingKind::Binding_Srv;
				return true;
			}
			case D3D_SIT_UAV_RWTYPED:
			case D3D_SIT_UAV_RWSTRUCTURED:
			case D3D_SIT_UAV_RWBYTEADDRESS:
			case D3D_SIT_UAV_APPEND_STRUCTURED:
			case D3D_SIT_UAV_CONSUME_STRUCTURED:
			case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
			{
				OutKind = MCDxcBindingKind::Binding_Uav;
				return true;
			}
		}
		return false;
	}

	static bool CollectDxilBindings(ID3D12ShaderReflection* InReflection, const D3D12_SHADER_DESC& InShaderDesc, std::vector<MCDxcBinding>& OutBindings, std::string& OutError) noexcept
	{
		for (UINT BindingIdx = 0; BindingIdx < InShaderDesc.BoundResources; ++BindingIdx)
		{
			D3D12_SHADER_INPUT_BIND_DESC BindingDesc{};
			HRESULT HandleResult = InReflection->GetResourceBindingDesc(BindingIdx, &BindingDesc);
			if (FAILED(HandleResult))
			{
				OutError = "shader reflection failed (ID3D12ShaderReflection::GetResourceBindingDesc), HRESULT = " + MakeHResultText(HandleResult);
				return false;
			}
			MCDxcBindingKind Kind = MCDxcBindingKind::Binding_Srv;
			if (!ToBindingKind(BindingDesc.Type, Kind))
			{
				continue;
			}
			MCDxcBinding Binding;
			Binding.Name = (BindingDesc.Name != nullptr) ? BindingDesc.Name : "";
			Binding.BindPoint = BindingDesc.BindPoint;
			Binding.Kind = Kind;
			OutBindings.emplace_back(std::move(Binding));
		}
		return true;
	}

	static bool CollectDxbcBindings(ID3D11ShaderReflection* InReflection, const D3D11_SHADER_DESC& InShaderDesc, std::vector<MCDxcBinding>& OutBindings, std::string& OutError) noexcept
	{
		for (UINT BindingIdx = 0; BindingIdx < InShaderDesc.BoundResources; ++BindingIdx)
		{
			D3D11_SHADER_INPUT_BIND_DESC BindingDesc{};
			HRESULT HandleResult = InReflection->GetResourceBindingDesc(BindingIdx, &BindingDesc);
			if (FAILED(HandleResult))
			{
				OutError = "shader reflection failed (ID3D11ShaderReflection::GetResourceBindingDesc), HRESULT = " + MakeHResultText(HandleResult);
				return false;
			}
			MCDxcBindingKind Kind = MCDxcBindingKind::Binding_Srv;
			if (!ToBindingKind(BindingDesc.Type, Kind))
			{
				continue;
			}
			MCDxcBinding Binding;
			Binding.Name = (BindingDesc.Name != nullptr) ? BindingDesc.Name : "";
			Binding.BindPoint = BindingDesc.BindPoint;
			Binding.Kind = Kind;
			OutBindings.emplace_back(std::move(Binding));
		}
		return true;
	}

	static bool CollectDxilCbVariables(ID3D12ShaderReflectionConstantBuffer* InCbReflection, const D3D12_SHADER_BUFFER_DESC& InCbDesc, MCDxcCbLayout& OutLayout, std::string& OutError) noexcept
	{
		for (UINT VarIdx = 0; VarIdx < InCbDesc.Variables; ++VarIdx)
		{
			ID3D12ShaderReflectionVariable* VarReflection = InCbReflection->GetVariableByIndex(VarIdx);
			if (VarReflection == nullptr)
			{
				continue;
			}
			D3D12_SHADER_VARIABLE_DESC VarDesc{};
			if (FAILED(VarReflection->GetDesc(&VarDesc)))
			{
				continue;
			}
			MCDxcCbVariable Variable;
			Variable.Name = (VarDesc.Name != nullptr) ? VarDesc.Name : "";
			Variable.ByteOffset = VarDesc.StartOffset;
			Variable.ContentBytes = VarDesc.Size;

			ID3D12ShaderReflectionType* TypeReflection = VarReflection->GetType();
			if (TypeReflection != nullptr)
			{
				D3D12_SHADER_TYPE_DESC TypeDesc{};
				if (SUCCEEDED(TypeReflection->GetDesc(&TypeDesc)))
				{
					if (!FillCbArrayStride(Variable.Name, TypeDesc.Class, TypeDesc.Rows, TypeDesc.Columns, TypeDesc.Elements, VarDesc.Size, Variable.ArrayStride, OutError))
					{
						return false;
					}
				}
			}
			OutLayout.Variables.emplace_back(std::move(Variable));
		}
		return true;
	}

	static bool CollectDxbcCbVariables(ID3D11ShaderReflectionConstantBuffer* InCbReflection, const D3D11_SHADER_BUFFER_DESC& InCbDesc, MCDxcCbLayout& OutLayout, std::string& OutError) noexcept
	{
		for (UINT VarIdx = 0; VarIdx < InCbDesc.Variables; ++VarIdx)
		{
			ID3D11ShaderReflectionVariable* VarReflection = InCbReflection->GetVariableByIndex(VarIdx);
			if (VarReflection == nullptr)
			{
				continue;
			}
			D3D11_SHADER_VARIABLE_DESC VarDesc{};
			if (FAILED(VarReflection->GetDesc(&VarDesc)))
			{
				continue;
			}
			MCDxcCbVariable Variable;
			Variable.Name = (VarDesc.Name != nullptr) ? VarDesc.Name : "";
			Variable.ByteOffset = VarDesc.StartOffset;
			Variable.ContentBytes = VarDesc.Size;

			ID3D11ShaderReflectionType* TypeReflection = VarReflection->GetType();
			if (TypeReflection != nullptr)
			{
				D3D11_SHADER_TYPE_DESC TypeDesc{};
				if (SUCCEEDED(TypeReflection->GetDesc(&TypeDesc)))
				{
					if (!FillCbArrayStride(Variable.Name, TypeDesc.Class, TypeDesc.Rows, TypeDesc.Columns, TypeDesc.Elements, VarDesc.Size, Variable.ArrayStride, OutError))
					{
						return false;
					}
				}
			}
			OutLayout.Variables.emplace_back(std::move(Variable));
		}
		return true;
	}

	static bool ExtractReflectionFromDxil(IDxcUtils* InDxcUtils, const void* InBlobData, size_t InBlobSize, MCDxcResult& OutResult) noexcept
	{
		DxcBuffer BlobBuffer{};
		BlobBuffer.Ptr = InBlobData;
		BlobBuffer.Size = InBlobSize;

		Microsoft::WRL::ComPtr<ID3D12ShaderReflection> DxilReflection;
		HRESULT HandleResult = InDxcUtils->CreateReflection(&BlobBuffer, IID_PPV_ARGS(&DxilReflection));
		if (FAILED(HandleResult) || (!DxilReflection))
		{
			OutResult.Errors = "shader reflection failed (DXIL CreateReflection), HRESULT = " + MakeHResultText(HandleResult);
			return false;
		}

		D3D12_SHADER_DESC ShaderDesc{};
		HandleResult = DxilReflection->GetDesc(&ShaderDesc);
		if (FAILED(HandleResult))
		{
			OutResult.Errors = "shader reflection failed (ID3D12ShaderReflection::GetDesc), HRESULT = " + MakeHResultText(HandleResult);
			return false;
		}

		for (UINT CbIdx = 0; CbIdx < ShaderDesc.ConstantBuffers; ++CbIdx)
		{
			ID3D12ShaderReflectionConstantBuffer* CbReflection = DxilReflection->GetConstantBufferByIndex(CbIdx);
			if (CbReflection == nullptr)
			{
				continue;
			}
			D3D12_SHADER_BUFFER_DESC CbDesc{};
			if (FAILED(CbReflection->GetDesc(&CbDesc)))
			{
				continue;
			}
			MCDxcCbLayout Layout;
			Layout.Name = (CbDesc.Name != nullptr) ? CbDesc.Name : "";
			Layout.ByteSize = CbDesc.Size;
			if (!CollectDxilCbVariables(CbReflection, CbDesc, Layout, OutResult.Errors))
			{
				return false;
			}
			OutResult.CBuffers.emplace_back(std::move(Layout));
		}

		if (!CollectDxilBindings(DxilReflection.Get(), ShaderDesc, OutResult.Bindings, OutResult.Errors))
		{
			return false;
		}
		return true;
	}

	static bool ExtractReflectionFromDxbc(const void* InBlobData, size_t InBlobSize, MCDxcResult& OutResult) noexcept
	{
		Microsoft::WRL::ComPtr<ID3D11ShaderReflection> DxbcReflection;
		HRESULT HandleResult = D3DReflect(InBlobData, InBlobSize, IID_PPV_ARGS(&DxbcReflection));
		if (FAILED(HandleResult) || (!DxbcReflection))
		{
			OutResult.Errors = "shader reflection failed (DXBC D3DReflect), HRESULT = " + MakeHResultText(HandleResult);
			return false;
		}

		D3D11_SHADER_DESC ShaderDesc{};
		HandleResult = DxbcReflection->GetDesc(&ShaderDesc);
		if (FAILED(HandleResult))
		{
			OutResult.Errors = "shader reflection failed (ID3D11ShaderReflection::GetDesc), HRESULT = " + MakeHResultText(HandleResult);
			return false;
		}

		for (UINT CbIdx = 0; CbIdx < ShaderDesc.ConstantBuffers; ++CbIdx)
		{
			ID3D11ShaderReflectionConstantBuffer* CbReflection = DxbcReflection->GetConstantBufferByIndex(CbIdx);
			if (CbReflection == nullptr)
			{
				continue;
			}
			D3D11_SHADER_BUFFER_DESC CbDesc{};
			if (FAILED(CbReflection->GetDesc(&CbDesc)))
			{
				continue;
			}
			MCDxcCbLayout Layout;
			Layout.Name = (CbDesc.Name != nullptr) ? CbDesc.Name : "";
			Layout.ByteSize = CbDesc.Size;
			if (!CollectDxbcCbVariables(CbReflection, CbDesc, Layout, OutResult.Errors))
			{
				return false;
			}
			OutResult.CBuffers.emplace_back(std::move(Layout));
		}

		if (!CollectDxbcBindings(DxbcReflection.Get(), ShaderDesc, OutResult.Bindings, OutResult.Errors))
		{
			return false;
		}
		return true;
	}

	// ---------------------------------------------------------------------------

	MCDxcCompiler::MCDxcCompiler() noexcept
	{

	}

	MCDxcCompiler::~MCDxcCompiler() noexcept
	{
		Shutdown();
	}

	bool MCDxcCompiler::Init(std::string& OutError) noexcept
	{
		HRESULT HandleResult = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&DXCUtils));
		if (FAILED(HandleResult))
		{
			OutError = "DxcCreateInstance(DxcUtils) failed";
			return false;
		}

		HandleResult = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&DXCCompiler));
		if (FAILED(HandleResult))
		{
			OutError = "DxcCreateInstance(DxcCompiler) failed";
			return false;
		}

		HandleResult = DXCUtils->CreateDefaultIncludeHandler(&DXCIncludeHandler);
		if (FAILED(HandleResult))
		{
			OutError = "CreateDefaultIncludeHandler failed";
			return false;
		}

		return true;
	}

	void MCDxcCompiler::Shutdown() noexcept
	{
		if (DXCIncludeHandler)
		{
			DXCIncludeHandler->Release();
			DXCIncludeHandler = nullptr;
		}
		if (DXCCompiler)
		{
			DXCCompiler->Release();
			DXCCompiler = nullptr;
		}
		if (DXCUtils)
		{
			DXCUtils->Release();
			DXCUtils = nullptr;
		}
	}

	bool MCDxcCompiler::Compile(const MCCompileParams& InParams, MCDxcResult& OutResult) noexcept
	{
		if (InParams.bDx11)
		{
			return CompileD3D(InParams, OutResult);
		}
		else
		{
			return CompileDxc(InParams, OutResult);
		}
	}

	bool MCDxcCompiler::CompileDxc(const MCCompileParams& InParams, MCDxcResult& OutResult) noexcept
	{
		OutResult = {};

		Microsoft::WRL::ComPtr<IDxcBlobEncoding> SourceBlob;
		HRESULT HandleResult = DXCUtils->CreateBlob(InParams.HlslSource.data(), static_cast<UINT32>(InParams.HlslSource.size()), DXC_CP_UTF8, &SourceBlob);
		if (FAILED(HandleResult))
		{
			OutResult.Errors = "CreateBlob failed";
			return false;
		}

		DxcBuffer SrcBuf{};
		SrcBuf.Ptr = SourceBlob->GetBufferPointer();
		SrcBuf.Size = SourceBlob->GetBufferSize();
		SrcBuf.Encoding = DXC_CP_UTF8;

		std::wstring SourceName = ToWideString(InParams.SourceName);
		std::wstring Profile = ToWideString(InParams.Profile);
		std::wstring EntryName = ToWideString(InParams.EntryName);

		std::vector<std::wstring> IncludeDirWides;
		for (auto& Dir : InParams.IncludeDirs)
		{
			IncludeDirWides.push_back(ToWideString(Dir));
		}

		std::vector<LPCWSTR> Args;
		Args.push_back(SourceName.c_str());
		Args.push_back(L"-E"); Args.push_back(EntryName.c_str());
		Args.push_back(L"-T"); Args.push_back(Profile.c_str());
		for (auto& Dir : IncludeDirWides)
		{
			Args.push_back(L"-I"); Args.push_back(Dir.c_str());
		}
		Args.push_back(L"-Zpc");

		if (InParams.bDebug)
		{
			Args.push_back(L"-Zi");
			Args.push_back(L"-Qembed_debug");
		}
		else
		{
			Args.push_back(L"-O3");
		}

		std::vector<std::wstring> WideDefines;
		for (const auto& Def : InParams.Defines)
		{
			WideDefines.push_back(ToWideString(Def.first + "=" + Def.second));
			Args.push_back(L"-D");
			Args.push_back(WideDefines.back().c_str());
		}

		Microsoft::WRL::ComPtr<IDxcResult> DXCResult;
		HandleResult = DXCCompiler->Compile(
			&SrcBuf,
			Args.data(),
			static_cast<UINT32>(Args.size()),
			DXCIncludeHandler,
			IID_PPV_ARGS(&DXCResult));

		Microsoft::WRL::ComPtr<IDxcBlobUtf8> ErrorBlob;
		if ((SUCCEEDED(DXCResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&ErrorBlob), nullptr))) && ErrorBlob && (ErrorBlob->GetStringLength() > 0))
		{
			OutResult.Errors = std::string(ErrorBlob->GetStringPointer(), ErrorBlob->GetStringLength());
		}

		HandleResult = S_OK;
		DXCResult->GetStatus(&HandleResult);
		if (FAILED(HandleResult))
		{
			return false;
		}

		Microsoft::WRL::ComPtr<IDxcBlob> DxilBlob;
		if ((SUCCEEDED(DXCResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&DxilBlob), nullptr))) && DxilBlob)
		{
			auto* BufPtr = static_cast<const uint8_t*>(DxilBlob->GetBufferPointer());
			OutResult.DxilBytes.assign(BufPtr, BufPtr + DxilBlob->GetBufferSize());
		}

		if (!DxilBlob)
		{
			OutResult.Errors = "shader compiler produced no object blob";
			return false;
		}
		if (!ExtractReflectionFromDxil(DXCUtils, DxilBlob->GetBufferPointer(), DxilBlob->GetBufferSize(), OutResult))
		{
			return false;
		}

		if (InParams.bDebug)
		{
			Microsoft::WRL::ComPtr<IDxcBlob> PdbBlob;
			if ((SUCCEEDED(DXCResult->GetOutput(DXC_OUT_PDB, IID_PPV_ARGS(&PdbBlob), nullptr))) && PdbBlob)
			{
				auto* BufPtr = static_cast<const uint8_t*>(PdbBlob->GetBufferPointer());
				OutResult.PdbBytes.assign(BufPtr, BufPtr + PdbBlob->GetBufferSize());
			}
		}

		OutResult.Success = true;
		return true;
	}

	bool MCDxcCompiler::CompileD3D(const MCCompileParams& InParams, MCDxcResult& OutResult) noexcept
	{
		OutResult = {};

		D3DIncludeHandler IncludeHandler;
		IncludeHandler.SearchDirs = InParams.IncludeDirs;

		UINT Flags = D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR | D3DCOMPILE_ENABLE_STRICTNESS;
		if (InParams.bDebug)
		{
			Flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
		}
		else
		{
			Flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
		}

		std::vector<D3D_SHADER_MACRO> Macros;
		for (const auto& Def : InParams.Defines)
		{
			Macros.push_back({ Def.first.c_str(), Def.second.c_str() });
		}
		Macros.push_back({ nullptr, nullptr });

		Microsoft::WRL::ComPtr<ID3DBlob> CodeBlob;
		Microsoft::WRL::ComPtr<ID3DBlob> ErrorBlob;
		HRESULT HandleResult = D3DCompile(
			InParams.HlslSource.data(),
			InParams.HlslSource.size(),
			InParams.SourceName.c_str(),
			Macros.data(),
			&IncludeHandler,
			InParams.EntryName.c_str(),
			InParams.Profile.c_str(),
			Flags,
			0,
			&CodeBlob,
			&ErrorBlob);

		if (ErrorBlob && (ErrorBlob->GetBufferSize() > 0))
		{
			OutResult.Errors = std::string(static_cast<const char*>(ErrorBlob->GetBufferPointer()), ErrorBlob->GetBufferSize());
		}

		if (FAILED(HandleResult))
		{
			return false;
		}

		auto* BufPtr = static_cast<const uint8_t*>(CodeBlob->GetBufferPointer());
		OutResult.DxilBytes.assign(BufPtr, BufPtr + CodeBlob->GetBufferSize());

		if (!ExtractReflectionFromDxbc(CodeBlob->GetBufferPointer(), CodeBlob->GetBufferSize(), OutResult))
		{
			return false;
		}

		OutResult.Success = true;
		return true;
	}

}	// namespace MC
