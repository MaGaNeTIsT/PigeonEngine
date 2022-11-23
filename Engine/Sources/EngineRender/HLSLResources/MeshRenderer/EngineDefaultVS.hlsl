#ifndef _ENGINE_DEFAULT_VS_HLSL
#define _ENGINE_DEFAULT_VS_HLSL

#include "../Common/ShaderCommon.hlsl"

Varying main(Attribute input)
{
	Varying output;

	output.positionWS	= float4(TransformObjectToWorld(input.position.xyz), 1.0);
	output.positionCS	= TransformWorldToClip(output.positionWS.xyz);
	output.normal		= TransformObjectToWorldNormal(input.normal.xyz).xyzz;
	output.tangent		= TransformObjectToWorldDir(input.tangent.xyz).xyzz;
	output.uv0			= input.uv0;

	return output;
}

#endif