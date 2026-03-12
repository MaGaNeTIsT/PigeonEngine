#include "MCDxcCompiler.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <Unknwn.h>
#include <dxcapi.h>
#include <wrl/client.h>
#include <d3dcompiler.h>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <unordered_map>

using Microsoft::WRL::ComPtr;
namespace fs = std::filesystem;

namespace MC
{

// ---------------------------------------------------------------------------
// ID3DInclude that handles relative paths (../ and ./) correctly by tracking
// each opened file's directory and using it to resolve child includes.
// ---------------------------------------------------------------------------
struct D3DIncludeHandler : public ID3DInclude
{
    std::vector<std::string>              SearchDirs;
    std::unordered_map<const void*, std::string> DataToDir; // raw ptr -> directory of that file

    HRESULT __stdcall Open(D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName,
        LPCVOID pParentData, LPCVOID* ppData, UINT* pBytes) override
    {
        std::vector<std::string> dirsToTry;

        // Local include: try parent file's directory first so relative paths work
        if (IncludeType == D3D_INCLUDE_LOCAL && pParentData != nullptr)
        {
            auto it = DataToDir.find(pParentData);
            if (it != DataToDir.end())
                dirsToTry.push_back(it->second);
        }
        for (auto& d : SearchDirs)
            dirsToTry.push_back(d);

        for (auto& dir : dirsToTry)
        {
            std::string raw = dir;
            if (!raw.empty() && raw.back() != '/' && raw.back() != '\\')
                raw += '/';
            raw += pFileName;

            // Normalise so that ../ and ./ are resolved
            std::error_code ec;
            fs::path canonical = fs::weakly_canonical(raw, ec);
            std::string fullPath = ec ? raw : canonical.string();

            std::ifstream f(fullPath, std::ios::binary);
            if (!f.is_open()) continue;

            std::string content((std::istreambuf_iterator<char>(f)),
                                 std::istreambuf_iterator<char>());
            char* buf = new char[content.size()];
            memcpy(buf, content.data(), content.size());
            *ppData = buf;
            *pBytes = static_cast<UINT>(content.size());

            // Remember this file's directory so its children can resolve relative paths
            DataToDir[buf] = fs::path(fullPath).parent_path().string();
            return S_OK;
        }
        return E_FAIL;
    }

    HRESULT __stdcall Close(LPCVOID pData) override
    {
        DataToDir.erase(pData);
        delete[] static_cast<const char*>(pData);
        return S_OK;
    }
};

// ---------------------------------------------------------------------------

MCDxcCompiler::MCDxcCompiler() = default;
MCDxcCompiler::~MCDxcCompiler() { Shutdown(); }

bool MCDxcCompiler::Init(std::string& OutError)
{
    HRESULT hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&m_Utils));
    if (FAILED(hr)) { OutError = "DxcCreateInstance(DxcUtils) failed"; return false; }

    hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&m_Compiler));
    if (FAILED(hr)) { OutError = "DxcCreateInstance(DxcCompiler) failed"; return false; }

    hr = m_Utils->CreateDefaultIncludeHandler(&m_Include);
    if (FAILED(hr)) { OutError = "CreateDefaultIncludeHandler failed"; return false; }

    return true;
}

void MCDxcCompiler::Shutdown()
{
    if (m_Include)  { m_Include->Release();  m_Include  = nullptr; }
    if (m_Compiler) { m_Compiler->Release(); m_Compiler = nullptr; }
    if (m_Utils)    { m_Utils->Release();    m_Utils    = nullptr; }
}

static std::wstring ToWide(const std::string& s)
{
    std::wstring w(s.begin(), s.end());
    return w;
}

bool MCDxcCompiler::Compile(
    const std::string& HlslSource,
    const std::string& SourceName,
    const std::string& Profile,
    const std::string& ShaderIncludeDir,
    const std::string& TemplateDir,
    const std::string& MatSourceDir,
    bool               EmitDebug,
    bool               Dx11,
    MCDxcResult&       Out)
{
    if (Dx11)
        return CompileD3D(HlslSource, SourceName, Profile,
            ShaderIncludeDir, TemplateDir, MatSourceDir, EmitDebug, Out);
    else
        return CompileDxc(HlslSource, SourceName, Profile,
            ShaderIncludeDir, TemplateDir, MatSourceDir, EmitDebug, Out);
}

bool MCDxcCompiler::CompileDxc(
    const std::string& HlslSource,
    const std::string& SourceName,
    const std::string& Profile,
    const std::string& ShaderIncludeDir,
    const std::string& TemplateDir,
    const std::string& MatSourceDir,
    bool               EmitDebug,
    MCDxcResult&       Out)
{
    Out = {};

    ComPtr<IDxcBlobEncoding> sourceBlob;
    HRESULT hr = m_Utils->CreateBlob(
        HlslSource.data(), static_cast<UINT32>(HlslSource.size()),
        DXC_CP_UTF8, &sourceBlob);
    if (FAILED(hr)) { Out.Errors = "CreateBlob failed"; return false; }

    DxcBuffer srcBuf{};
    srcBuf.Ptr      = sourceBlob->GetBufferPointer();
    srcBuf.Size     = sourceBlob->GetBufferSize();
    srcBuf.Encoding = DXC_CP_UTF8;

    std::wstring wSourceName   = ToWide(SourceName);
    std::wstring wProfile      = ToWide(Profile);
    std::wstring wIncludeDir   = ToWide(ShaderIncludeDir);
    std::wstring wTemplateDir  = ToWide(TemplateDir);
    std::wstring wMatSourceDir = ToWide(MatSourceDir);

    std::vector<LPCWSTR> args;
    args.push_back(wSourceName.c_str());
    args.push_back(L"-E"); args.push_back(L"main");
    args.push_back(L"-T"); args.push_back(wProfile.c_str());
    args.push_back(L"-I"); args.push_back(wIncludeDir.c_str());
    args.push_back(L"-I"); args.push_back(wTemplateDir.c_str());
    args.push_back(L"-I"); args.push_back(wMatSourceDir.c_str());
    args.push_back(L"-Zpc");

    if (EmitDebug)
    {
        args.push_back(L"-Zi");
        args.push_back(L"-Qembed_debug");
    }
    else
    {
        args.push_back(L"-O3");
    }

    ComPtr<IDxcResult> result;
    hr = m_Compiler->Compile(
        &srcBuf,
        args.data(), static_cast<UINT32>(args.size()),
        m_Include,
        IID_PPV_ARGS(&result));

    ComPtr<IDxcBlobUtf8> errBlob;
    if (SUCCEEDED(result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errBlob), nullptr))
        && errBlob && errBlob->GetStringLength() > 0)
        Out.Errors = std::string(errBlob->GetStringPointer(), errBlob->GetStringLength());

    HRESULT compileStatus = S_OK;
    result->GetStatus(&compileStatus);
    if (FAILED(compileStatus)) return false;

    ComPtr<IDxcBlob> dxilBlob;
    if (SUCCEEDED(result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&dxilBlob), nullptr)) && dxilBlob)
    {
        auto* ptr = static_cast<const uint8_t*>(dxilBlob->GetBufferPointer());
        Out.DxilBytes.assign(ptr, ptr + dxilBlob->GetBufferSize());
    }

    if (EmitDebug)
    {
        ComPtr<IDxcBlob> pdbBlob;
        if (SUCCEEDED(result->GetOutput(DXC_OUT_PDB, IID_PPV_ARGS(&pdbBlob), nullptr)) && pdbBlob)
        {
            auto* ptr = static_cast<const uint8_t*>(pdbBlob->GetBufferPointer());
            Out.PdbBytes.assign(ptr, ptr + pdbBlob->GetBufferSize());
        }
    }

    Out.Success = true;
    return true;
}

bool MCDxcCompiler::CompileD3D(
    const std::string& HlslSource,
    const std::string& SourceName,
    const std::string& Profile,
    const std::string& ShaderIncludeDir,
    const std::string& TemplateDir,
    const std::string& MatSourceDir,
    bool               EmitDebug,
    MCDxcResult&       Out)
{
    Out = {};

    D3DIncludeHandler includeHandler;
    includeHandler.SearchDirs = { ShaderIncludeDir, TemplateDir, MatSourceDir };

    UINT flags = D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR | D3DCOMPILE_ENABLE_STRICTNESS;
    if (EmitDebug)
        flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
    else
        flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;

    ComPtr<ID3DBlob> codeBlob;
    ComPtr<ID3DBlob> errBlob;
    HRESULT hr = D3DCompile(
        HlslSource.data(),
        HlslSource.size(),
        SourceName.c_str(),
        nullptr,            // no macro defines (already baked into source)
        &includeHandler,
        "main",
        Profile.c_str(),
        flags,
        0,
        &codeBlob,
        &errBlob);

    if (errBlob && errBlob->GetBufferSize() > 0)
        Out.Errors = std::string(static_cast<const char*>(errBlob->GetBufferPointer()),
                                 errBlob->GetBufferSize());

    if (FAILED(hr)) return false;

    auto* ptr = static_cast<const uint8_t*>(codeBlob->GetBufferPointer());
    Out.DxilBytes.assign(ptr, ptr + codeBlob->GetBufferSize());
    Out.Success = true;
    return true;
}

} // namespace MC
