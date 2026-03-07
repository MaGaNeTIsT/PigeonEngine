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

    // profile: e.g. "vs_6_0", "ps_6_0", "cs_6_0"
    bool Compile(
        const std::string& HlslSource,
        const std::string& SourceName,
        const std::string& Profile,
        const std::string& ShaderIncludeDir,
        const std::string& TemplateDir,
        const std::string& MatSourceDir,
        bool               EmitDebug,
        MCDxcResult&       Out);

private:
    IDxcUtils*          m_Utils    = nullptr;
    IDxcCompiler3*      m_Compiler = nullptr;
    IDxcIncludeHandler* m_Include  = nullptr;
};

} // namespace MC
