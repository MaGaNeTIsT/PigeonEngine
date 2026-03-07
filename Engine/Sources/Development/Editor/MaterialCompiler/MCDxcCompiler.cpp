#include "MCDxcCompiler.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <Unknwn.h>
#include <dxcapi.h>
#include <wrl/client.h>
#include <string>
#include <vector>

using Microsoft::WRL::ComPtr;

namespace MC
{

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
    MCDxcResult&       Out)
{
    Out = {};

    // Wrap source in a DxcBuffer
    ComPtr<IDxcBlobEncoding> sourceBlob;
    HRESULT hr = m_Utils->CreateBlob(
        HlslSource.data(), static_cast<UINT32>(HlslSource.size()),
        DXC_CP_UTF8, &sourceBlob);
    if (FAILED(hr)) { Out.Errors = "CreateBlob failed"; return false; }

    DxcBuffer srcBuf{};
    srcBuf.Ptr      = sourceBlob->GetBufferPointer();
    srcBuf.Size     = sourceBlob->GetBufferSize();
    srcBuf.Encoding = DXC_CP_UTF8;

    std::wstring wSourceName = ToWide(SourceName);
    std::wstring wProfile    = ToWide(Profile);
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
    args.push_back(L"-Zpc"); // column-major matrices

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

    // Collect errors regardless of hr
    ComPtr<IDxcBlobUtf8> errBlob;
    if (SUCCEEDED(result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errBlob), nullptr)) && errBlob && errBlob->GetStringLength() > 0)
        Out.Errors = std::string(errBlob->GetStringPointer(), errBlob->GetStringLength());

    HRESULT compileStatus = S_OK;
    result->GetStatus(&compileStatus);
    if (FAILED(compileStatus)) return false;

    // Extract DXIL
    ComPtr<IDxcBlob> dxilBlob;
    if (SUCCEEDED(result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&dxilBlob), nullptr)) && dxilBlob)
    {
        auto* ptr = static_cast<const uint8_t*>(dxilBlob->GetBufferPointer());
        Out.DxilBytes.assign(ptr, ptr + dxilBlob->GetBufferSize());
    }

    // Extract PDB (debug only)
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

} // namespace MC
