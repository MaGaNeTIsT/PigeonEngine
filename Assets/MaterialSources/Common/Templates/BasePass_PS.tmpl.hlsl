{VARYING_STRUCT}

{MATERIAL_CB_DECL}

{MATERIAL_TEXTURE_DECL}

#define SHADER_USE_DEFERRED_SHADING 1
#define SHADER_USE_SAMPLER_INPUT    1
#define SHADER_USE_VIEW_INPUT       1

#include "Common/ShaderCommon.hlsl"

struct SurfaceOutput
{
    float3 BaseColor;
    float3 Normal;
    float  Roughness;
    float  Metallic;
};

{MATERIAL_PS_FUNCTIONS}

{MATERIAL_SURFACE_CODE}

void main(
    in  Varying Input,
    out float4  OutSceneColor : SV_Target,
    out float4  OutGBufferA   : SV_Target1,
    out float4  OutGBufferB   : SV_Target2,
    out float4  OutGBufferC   : SV_Target3)
{
    SurfaceOutput Surface;
    CalcSurface(Input, Surface);

    GBufferTerm GBuffer = InitGBuffer(
        SafeNormalize(Surface.Normal),
        Surface.BaseColor,
        Surface.Roughness,
        Surface.Metallic,
        SHADER_SHADING_MODEL_DEFAULT_LIT,
        1.0, 1.0);

    OutSceneColor = float4(0.0, 0.0, 0.0, 1.0);
    EncodeGBuffer(GBuffer, OutGBufferA, OutGBufferB, OutGBufferC);
}
