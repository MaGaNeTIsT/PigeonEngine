{VF_ATTRIBUTE_STRUCT}

{VARYING_STRUCT}

{MATERIAL_CB_DECL}

{MATERIAL_TEXTURE_DECL}

#define SHADER_USE_VIEW_INPUT       1
#define SHADER_USE_PERDRAW_INPUT    1
#define SHADER_USE_TRANSFORM_INPUT  1
#define SHADER_USE_TRANSFORM        1

#include "Common/ShaderCommon.hlsl"

{VF_COMMON}

{VF_VS_FUNCTIONS}

{MATERIAL_COMMON}

{PASS_COMMON}

{VF_VS_CODE}

void main(in Attribute Input, uint InstanceID : SV_InstanceID, out Varying Output)
{
    Output = VertexFactoryMain(Input, InstanceID);
}
