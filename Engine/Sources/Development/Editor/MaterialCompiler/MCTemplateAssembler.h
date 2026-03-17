#pragma once
#include "MCJsonParser.h"
#include <string>
#include <vector>

namespace MC
{

// Slot assignments (engine-fixed)
static constexpr int kMaterialCBStartSlot      = 2;  // b2+
static constexpr int kMaterialSRVStartSlot     = 1;  // t1+
static constexpr int kMaterialSamplerStartSlot = 4;  // s4+
static constexpr int kMaterialUAVStartSlot     = 0;  // u0+

struct MCSlotMap
{
    int CBSlot     = kMaterialCBStartSlot;
    // SRV slots: index i -> t(kMaterialSRVStartSlot + i)
    // Sampler slots: index i -> s(kMaterialSamplerStartSlot + i)
};

struct MCAssembledShader
{
    std::string HlslSource;  // final HLSL string ready for DXC
    MCSlotMap   Slots;
};

class MCTemplateAssembler
{
public:
    // Load a template file from disk.
    bool LoadTemplate(const std::string& TemplatePath, std::string& OutError);

    // Assemble the final HLSL for one variant.
    // DefineOrder: VF defines -> Material defines -> Pass defines -> Variant defines
    bool Assemble(
        const MCVertexFactory& VF,
        const MCMaterial&      Mat,
        const MCShaderPass&    Pass,
        const MCVariant&       Variant,
        MCAssembledShader&     Out,
        std::string&           OutError);

private:
    std::string m_TemplateSource;

    static std::string BuildDefineBlock(
        const MCVertexFactory& VF,
        const MCMaterial&      Mat,
        const MCShaderPass&    Pass,
        const MCVariant&       Variant);

    static std::string BuildAttributeStruct(const MCVertexFactory& VF);
    static std::string BuildCBDecl(const MCMaterial& Mat, int Slot);
    static std::string BuildTextureDecl(const MCMaterial& Mat, int SRVStart, int SamplerStart);

    static void ReplaceToken(std::string& Src, const std::string& Token, const std::string& Value);
};

} // namespace MC
