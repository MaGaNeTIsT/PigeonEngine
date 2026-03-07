{VARYING_STRUCT}

{MATERIAL_CB_DECL}

#define SHADER_USE_VIEW_INPUT       1
#define SHADER_USE_PERDRAW_INPUT    1
#define SHADER_USE_TRANSFORM_INPUT  1
#define SHADER_USE_TRANSFORM        1

#include "../Common/ShaderCommon.hlsl"

{VF_VS_FUNCTIONS}

void main(in Attribute Input, out Varying Output)
{
    {VF_VS_CODE}
}
