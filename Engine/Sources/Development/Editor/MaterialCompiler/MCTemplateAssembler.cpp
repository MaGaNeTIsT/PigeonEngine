#include "MCTemplateAssembler.h"
#include <fstream>
#include <sstream>

namespace MC
{

bool MCTemplateAssembler::LoadTemplate(const std::string& TemplatePath, std::string& OutError)
{
    std::ifstream f(TemplatePath);
    if (!f.is_open()) { OutError = "Cannot open template: " + TemplatePath; return false; }
    std::ostringstream ss; ss << f.rdbuf();
    m_TemplateSource = ss.str();
    return true;
}

void MCTemplateAssembler::ReplaceToken(std::string& Src, const std::string& Token, const std::string& Value)
{
    size_t pos = 0;
    while ((pos = Src.find(Token, pos)) != std::string::npos)
    {
        Src.replace(pos, Token.size(), Value);
        pos += Value.size();
    }
}

std::string MCTemplateAssembler::BuildDefineBlock(
    const MCVertexFactory& VF,
    const MCMaterial&      Mat,
    const MCShaderPass&    Pass,
    const MCVariant&       Variant)
{
    std::ostringstream ss;
    // 1. VF attribute defines
    for (auto& a : VF.Attributes)
    {
        ss << "#define " << a.Define << " " << a.Num << "\n";
        ss << "#define " << a.Define << "_NUM " << a.Num << "\n";
    }
    // 2. VF defines
    for (auto& d : VF.Defines)
        ss << "#define " << d.Name << " " << d.Value << "\n";
    // 3. Material defines
    for (auto& d : Mat.Defines)
        ss << "#define " << d.Name << " " << d.Value << "\n";
    // 3. Pass defines
    for (auto& d : Pass.Defines)
        ss << "#define " << d.Name << " " << d.Value << "\n";
    // 4. Variant defines (last — can override)
    for (auto& d : Variant.Defines)
        ss << "#define " << d.Name << " " << d.Value << "\n";
    return ss.str();
}

std::string MCTemplateAssembler::BuildCBDecl(const MCMaterial& Mat, int Slot)
{
    if (Mat.CB.Fields.empty()) return "";
    std::ostringstream ss;
    ss << "cbuffer " << Mat.CB.Name << " : register(b" << Slot << ")\n{\n";
    for (auto& f : Mat.CB.Fields)
        ss << "    " << f.Type << " " << f.Name << ";\n";
    ss << "};\n";
    return ss.str();
}

std::string MCTemplateAssembler::BuildTextureDecl(const MCMaterial& Mat, int SRVStart, int SamplerStart)
{
    std::ostringstream ss;
    int srvSlot     = SRVStart;
    int uavSlot     = 0;
    int samplerSlot = SamplerStart;

    for (auto& tex : Mat.Textures)
    {
        bool isUAV = tex.Type.rfind("RW", 0) == 0;
        if (isUAV)
            ss << tex.Type << "<float4> " << tex.Name << " : register(u" << uavSlot++ << ");\n";
        else
            ss << tex.Type << " " << tex.Name << " : register(t" << srvSlot++ << ");\n";
    }

    for (auto& samp : Mat.Samplers)
        ss << "SamplerState s" << samp.Name << " : register(s" << samplerSlot++ << ");\n";

    return ss.str();
}

bool MCTemplateAssembler::Assemble(
    const MCVertexFactory& VF,
    const MCMaterial&      Mat,
    const MCShaderPass&    Pass,
    const MCVariant&       Variant,
    MCAssembledShader&     Out,
    std::string&           OutError)
{
    if (m_TemplateSource.empty()) { OutError = "Template not loaded"; return false; }

    // Pass 1 — token substitution
    std::string src = m_TemplateSource;
    ReplaceToken(src, "{VF_VS_FUNCTIONS}",       VF.HlslVSFunctions);
    ReplaceToken(src, "{VF_VS_CODE}",            VF.HlslVS);
    ReplaceToken(src, "{VARYING_STRUCT}",        VF.VaryingStruct);
    ReplaceToken(src, "{MATERIAL_CB_DECL}",      BuildCBDecl(Mat, kMaterialCBStartSlot));
    ReplaceToken(src, "{MATERIAL_TEXTURE_DECL}", BuildTextureDecl(Mat, kMaterialSRVStartSlot, kMaterialSamplerStartSlot));
    ReplaceToken(src, "{MATERIAL_CS_FUNCTIONS}",  Mat.HlslCSFunctions);
    ReplaceToken(src, "{MATERIAL_PS_FUNCTIONS}",  Mat.HlslPSFunctions);
    ReplaceToken(src, "{MATERIAL_SURFACE_CODE}", Mat.HlslSurface);

    // Pass 2 — prepend #define block
    std::string defines = BuildDefineBlock(VF, Mat, Pass, Variant);
    Out.HlslSource = defines + "\n" + src;

    Out.Slots.CBSlot = kMaterialCBStartSlot;
    OutError.clear();
    return true;
}

} // namespace MC
