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
    std::string                HlslCommon;  // Common variables/functions
};

struct MCCBField
{
    std::string       Name;
    std::string       Type;   // "float4", "float", "float2", "color3", "color4", etc.
    std::vector<float> Default;
    bool              IsColor = false;  // true if type is "color3" or "color4"
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

struct MCBuffer
{
    std::string Name;
    std::string Type;    // "StructuredBuffer", "RWStructuredBuffer", "ByteAddressBuffer", "RWByteAddressBuffer"
    std::string Struct;  // struct type name (e.g., "MyData")
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
    std::vector<MCBuffer>   Buffers;
    std::vector<MCSampler>  Samplers;
    std::vector<MCDefine>   Defines;
    std::string          HlslSurface;
    std::string          HlslPSFunctions;
    std::string          HlslCSFunctions;
    std::string          HlslCommon;  // Common variables/functions
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
    std::string            VertexFactoryPath;  // Path to VF json
    std::string            MaterialPath;       // Path to Material json
    std::string            TemplateVS;
    std::string            TemplatePS;
    std::string            TemplateCS;
    std::vector<MCDefine>  Defines;
    std::vector<MCVariant> Variants;
    std::string            HlslCommon;  // Common variables/functions
};

// Returns false and sets OutError on failure.
bool ParseVertexFactory(const std::string& JsonPath, MCVertexFactory& Out, std::string& OutError);
bool ParseMaterial     (const std::string& JsonPath, MCMaterial&      Out, std::string& OutError);
bool ParseShaderPass   (const std::string& JsonPath, MCShaderPass&    Out, std::string& OutError);

} // namespace MC
