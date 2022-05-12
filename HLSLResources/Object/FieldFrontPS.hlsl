#ifndef _SHADER_FIELD_FRONT_PS_HLSL
#define _SHADER_FIELD_FRONT_PS_HLSL

#include "../Common/ShaderFunctions.hlsl"

FieldFrontpOutput main(FieldpInput pin)
{
	FieldFrontpOutput pout;
	pout.Diffuse = 1.0;
	return pout;
}

#endif