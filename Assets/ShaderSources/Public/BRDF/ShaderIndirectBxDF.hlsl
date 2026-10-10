#ifndef SHADER_INDIRECT_BX_DF_HLSL
#define SHADER_INDIRECT_BX_DF_HLSL

// ---------------------------------------------------------------------------
// BRDF terms specific to indirect lighting.
//
// The split sum approximation factors the environment integral into a
// prefiltered radiance term and an environment BRDF term. The radiance half is
// a sampling problem and lives in the lighting layer; the BRDF half is a
// function of roughness, N dot V and F0 alone, which is what this file owns.
// ---------------------------------------------------------------------------

#include "Public/Core/ShaderConstants.hlsl"
#include "Public/Core/ShaderMath.hlsl"

// Analytic fit of the split sum environment BRDF.
//
// Karis 2013, "Real Shading in Unreal Engine 4". The second stage replaces this
// with a DFG lookup table, keeping the signature: the table is the same function
// with the fit error removed.
float3 ComputeEnvironmentBRDF(in float InRoughness, in float InNdotV, in float3 InF0)
{
	float4 C0 = float4(-1.0, -0.0275, -0.572, 0.022);
	float4 C1 = float4(1.0, 0.0425, 1.04, -0.04);

	float4 R = InRoughness * C0 + C1;
	float A004 = min(R.x * R.x, exp2(-9.28 * InNdotV)) * R.x + R.y;
	float2 AB = float2(-1.04, 1.04) * A004 + R.zw;
	return InF0 * AB.x + AB.y;
}

// Diffuse counterpart of the same split: the irradiance integral is a property
// of the environment, so all this contributes is the BRDF weight, which for a
// Lambertian surface is just the albedo over pi. It is a separate function so
// that a future diffuse model with a view dependent term has a place to go.
float3 ComputeEnvironmentDiffuseBRDF(in float3 InDiffuseColor)
{
	return InDiffuseColor * PE_SHADER_PI_INV;
}
#endif	//SHADER_INDIRECT_BX_DF_HLSL
