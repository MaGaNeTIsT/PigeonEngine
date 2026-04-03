#pragma once
#include "MCJsonParser.h"
#include <string>
#include <vector>

namespace MC
{

// Slot assignments (engine-fixed)
static constexpr int kMaterialCBStartSlot      = 3;  // b3+ (b0-b2 reserved for engine)
static constexpr int kMaterialSRVStartSlot     = 8;  // t8+ (t0-t7 reserved for engine)
static constexpr int kMaterialSamplerStartSlot = 4;  // s4+ (s0-s3 reserved for engine)
static constexpr int kMaterialUAVStartSlot     = 0;  // u0+

// Info about a sampler that was mapped to an engine global slot (s0-s3).
struct MCEngineSamplerMapping
{
    std::string SamplerName;       // material sampler name
    int         EngineSlot = -1;   // engine slot (0-3)
    std::string EngineHlslName;    // e.g. "_LinearWrapSampler"
    std::string Filter;            // "POINT" / "LINEAR"
    std::string Address;           // "WRAP" / "CLAMP"
};

// Info about a custom sampler assigned to s4+.
struct MCCustomSamplerMapping
{
    std::string SamplerName;
    int         Slot = -1;         // s4, s5, ...
    std::string Filter;
    std::string Address;
};

struct MCSlotMap
{
    int CBSlot     = kMaterialCBStartSlot;
    // SRV slots: index i -> t(kMaterialSRVStartSlot + i)
    // Sampler slots: engine-global ones map to s0-s3, custom ones start at s4+
    std::vector<MCEngineSamplerMapping> EngineSamplers;
    std::vector<MCCustomSamplerMapping> CustomSamplers;
};

struct MCAssembledShader
{
    std::string HlslSource;  // final HLSL string ready for DXC
    MCSlotMap   Slots;
    std::string DumpPath;    // Optional: path to dump HLSL
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
        std::string&           OutError,
        bool                   DumpHlsl = false);

private:
    std::string m_TemplateSource;

    static std::string BuildDefineBlock(
        const MCVertexFactory& VF,
        const MCMaterial&      Mat,
        const MCShaderPass&    Pass,
        const MCVariant&       Variant);

    static std::string BuildAttributeStruct(const MCVertexFactory& VF);
    static std::string BuildCBDecl(const MCMaterial& Mat, int Slot);
    static std::string BuildTextureDecl(const MCMaterial& Mat, int SRVStart, int SamplerStart, MCSlotMap& OutSlots);
    static std::string BuildBufferDecl(const MCMaterial& Mat, int SRVStart, int UAVStart);

    static void ReplaceToken(std::string& Src, const std::string& Token, const std::string& Value);
};

} // namespace MC
