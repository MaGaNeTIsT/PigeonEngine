#pragma once
#include <string>
#include <vector>
#include <cstdint>

struct IDxcUtils;
struct IDxcCompiler3;
struct IDxcIncludeHandler;

namespace MC
{

struct MCDxcResult
{
    std::vector<uint8_t> DxilBytes;
    std::vector<uint8_t> PdbBytes;
    std::string          Errors;
    bool                 Success = false;
};

class MCDxcCompiler
{
public:
    MCDxcCompiler();
    ~MCDxcCompiler();

    bool Init(std::string& OutError);
    void Shutdown();

    // profile: e.g. "vs_6_0"/"ps_6_0" for DX12, "vs_5_0"/"ps_5_0" for DX11
    // Dx11: true → compile via D3DCompile (DXBC); false → compile via DXC (DXIL)
    bool Compile(
        const std::string& HlslSource,
        const std::string& SourceName,
        const std::string& Profile,
        const std::string& ShaderIncludeDir,
        const std::string& TemplateDir,
        const std::string& MatSourceDir,
        bool               EmitDebug,
        bool               Dx11,
        MCDxcResult&       Out);

private:
    IDxcUtils*          m_Utils    = nullptr;
    IDxcCompiler3*      m_Compiler = nullptr;
    IDxcIncludeHandler* m_Include  = nullptr;

    bool CompileDxc(
        const std::string& HlslSource,
        const std::string& SourceName,
        const std::string& Profile,
        const std::string& ShaderIncludeDir,
        const std::string& TemplateDir,
        const std::string& MatSourceDir,
        bool               EmitDebug,
        MCDxcResult&       Out);

    bool CompileD3D(
        const std::string& HlslSource,
        const std::string& SourceName,
        const std::string& Profile,
        const std::string& ShaderIncludeDir,
        const std::string& TemplateDir,
        const std::string& MatSourceDir,
        bool               EmitDebug,
        MCDxcResult&       Out);
};

} // namespace MC
