#pragma once
#include <string>
#include <vector>

namespace MC
{

struct MCDefine
{
    std::string Name;
    int         Value = 1;
};

struct MCVFInput
{
    std::string Semantic;   // e.g. "POSITION", "NORMAL"
    int         Index = 0;  // semantic index
    std::string Format;     // e.g. "FLOAT4", "FLOAT2", "UINT4"
    int         Slot  = 0;  // vertex buffer input slot
};

struct MCVertexFactory
{
    std::string                Name;
    std::vector<MCVFInput>     Inputs;
    std::vector<MCDefine>      Defines;
    std::string                HlslVS;
    std::string                HlslVSFunctions;
    std::string                VaryingStruct;
};

struct MCCBField
{
    std::string       Name;
    std::string       Type;   // "float4", "float", "float2", etc.
    std::vector<float> Default;
};

struct MCConstantBuffer
{
    std::string          Name;
    std::vector<MCCBField> Fields;
};

struct MCTexture
{
    std::string Name;
    std::string Type;    // "Texture2D"
    std::string Sampler; // sampler name reference
};

struct MCSampler
{
    std::string Name;
    std::string Filter;  // "LINEAR", "POINT"
    std::string Address; // "WRAP", "CLAMP"
};

struct MCMaterial
{
    std::string          Name;
    MCConstantBuffer     CB;
    std::vector<MCTexture>  Textures;
    std::vector<MCSampler>  Samplers;
    std::vector<MCDefine>   Defines;
    std::string          HlslSurface;
    std::string          HlslPSFunctions;
    std::string          HlslCSFunctions;
};

struct MCVariant
{
    std::string           Name;
    std::vector<MCDefine> Defines;
};

struct MCShaderPass
{
    std::string            Name;
    std::string            Stage;         // "VS_PS", "CS"
    std::string            VertexFactory;
    std::string            Material;
    std::string            TemplateVS;
    std::string            TemplatePS;
    std::string            TemplateCS;
    std::vector<MCDefine>  Defines;
    std::vector<MCVariant> Variants;
};

// Returns false and sets OutError on failure.
bool ParseVertexFactory(const std::string& JsonPath, MCVertexFactory& Out, std::string& OutError);
bool ParseMaterial     (const std::string& JsonPath, MCMaterial&      Out, std::string& OutError);
bool ParseShaderPass   (const std::string& JsonPath, MCShaderPass&    Out, std::string& OutError);

} // namespace MC
