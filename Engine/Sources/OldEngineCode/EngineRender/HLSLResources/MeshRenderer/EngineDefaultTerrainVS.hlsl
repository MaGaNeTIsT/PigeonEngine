#ifndef _ENGINE_DEFAULT_TERRAIN_VS_HLSL
#define _ENGINE_DEFAULT_TERRAIN_VS_HLSL

#include "../Common/ShaderCommon.hlsl"

VaryingTerrain main(AttributeTerrain input)
{
	VaryingTerrain output;

	output.positionCS	= TransformObjectToClip(input.position.xyz);
	output.normal		= TransformObjectToWorldNormal(input.normal.xyz).xyzz;
	output.tangent		= TransformObjectToWorldDir(input.tangent.xyz).xyzz;
	output.uv0			= input.uv0;

	return output;
}

#endif