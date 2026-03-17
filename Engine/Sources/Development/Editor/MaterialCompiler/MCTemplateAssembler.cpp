#include "MCTemplateAssembler.h"
#include <cctype>
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

std::string MCTemplateAssembler::BuildAttributeStruct(const MCVertexFactory& VF)
{
    // Map format string to HLSL scalar/vector type.
    // "FLOAT4" -> "float4", "FLOAT2" -> "float2", "FLOAT1" -> "float",
    // "UINT4"  -> "uint4",  "INT4"   -> "int4", etc.
    auto HlslType = [](const std::string& fmt) -> std::string
    {
        std::string base;
        size_t digitPos = 0;
        if      (fmt.rfind("FLOAT", 0) == 0) { base = "float"; digitPos = 5; }
        else if (fmt.rfind("UINT",  0) == 0) { base = "uint";  digitPos = 4; }
        else if (fmt.rfind("INT",   0) == 0) { base = "int";   digitPos = 3; }
        else return "float4";

        if (digitPos < fmt.size() && fmt[digitPos] >= '2' && fmt[digitPos] <= '4')
            return base + fmt[digitPos];
        return base; // FLOAT1 or bare FLOAT -> "float"
    };

    // "POSITION" + 0 -> "Position0"  (capitalize first, lowercase rest, append index)
    auto FieldName = [](const std::string& semantic, int index) -> std::string
    {
        std::string name;
        if (!semantic.empty())
        {
            name += (char)::toupper((unsigned char)semantic[0]);
            for (size_t i = 1; i < semantic.size(); ++i)
                name += (char)::tolower((unsigned char)semantic[i]);
        }
        name += std::to_string(index);
        return name;
    };

    std::ostringstream ss;
    ss << "#define _VF_ATTRIBUTE_STRUCT_DEFINED\n";
    ss << "struct Attribute\n{\n";
    for (auto& inp : VF.Inputs)
    {
        ss << "    " << HlslType(inp.Format) << "  "
           << FieldName(inp.Semantic, inp.Index) << "\t: "
           << inp.Semantic << inp.Index << ";\n";
    }
    ss << "};\n";
    return ss.str();
}

std::string MCTemplateAssembler::BuildDefineBlock(
    const MCVertexFactory& VF,
    const MCMaterial&      Mat,
    const MCShaderPass&    Pass,
    const MCVariant&       Variant)
{
    std::ostringstream ss;
    // 1. Input defines — derived from each input entry's semantic and format
    //    Name : SHADER_USE_{SEMANTIC}_INPUT
    //    Value: component count parsed from the last char of Format ("FLOAT4"->4, "FLOAT2"->2)
    for (auto& inp : VF.Inputs)
    {
        int num = (!inp.Format.empty() && inp.Format.back() >= '1' && inp.Format.back() <= '4')
                  ? (inp.Format.back() - '0') : 4;
        std::string defineName = "SHADER_USE_" + inp.Semantic + "_INPUT";
        ss << "#define " << defineName       << " " << num << "\n";
        ss << "#define " << defineName << "_NUM " << num << "\n";
    }
    // 2. VF defines
    for (auto& d : VF.Defines)
        ss << "#define " << d.Name << " " << d.Value << "\n";
    // 3. Material defines
    for (auto& d : Mat.Defines)
        ss << "#define " << d.Name << " " << d.Value << "\n";
    // 4. Pass defines
    for (auto& d : Pass.Defines)
        ss << "#define " << d.Name << " " << d.Value << "\n";
    // 5. Variant defines (last — can override)
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
    ReplaceToken(src, "{VF_ATTRIBUTE_STRUCT}",   BuildAttributeStruct(VF));
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
