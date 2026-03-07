#ifndef _SHADER_DEFERRED_COMMON_HLSL
#define _SHADER_DEFERRED_COMMON_HLSL

#include "./ShaderStructCommon.hlsl"
#include "./ShaderFunctions.hlsl"

#ifndef SHADER_USE_GBUFFER_INPUT
#define SHADER_USE_GBUFFER_INPUT					0
#endif	//SHADER_USE_GBUFFER_INPUT

#ifndef SHADER_GBUFFER_A_INPUT_SLOT
#define SHADER_GBUFFER_A_INPUT_SLOT					t0
#endif	//SHADER_GBUFFER_A_INPUT_SLOT

#ifndef SHADER_GBUFFER_B_INPUT_SLOT
#define SHADER_GBUFFER_B_INPUT_SLOT					t1
#endif	//SHADER_GBUFFER_B_INPUT_SLOT

#ifndef SHADER_GBUFFER_C_INPUT_SLOT
#define SHADER_GBUFFER_C_INPUT_SLOT					t2
#endif	//SHADER_GBUFFER_C_INPUT_SLOT

#if (SHADER_USE_GBUFFER_INPUT)
Texture2D<float4>	_ShaderGBufferAInput : register(SHADER_GBUFFER_A_INPUT_SLOT);
Texture2D<float4>	_ShaderGBufferBInput : register(SHADER_GBUFFER_B_INPUT_SLOT);
Texture2D<float4>	_ShaderGBufferCInput : register(SHADER_GBUFFER_C_INPUT_SLOT);
#endif

void SampleGBuffer(in float2 InUV, in SamplerState InUsedSampler, out float4 OutGBufferA, out float4 OutGBufferB, out float4 OutGBufferC)
{
#if (SHADER_USE_GBUFFER_INPUT)
	OutGBufferA.xyzw = _ShaderGBufferAInput.Sample(InUsedSampler, InUV.xy).xyzw;
	OutGBufferB.xyzw = _ShaderGBufferBInput.Sample(InUsedSampler, InUV.xy).xyzw;
	OutGBufferC.xyzw = _ShaderGBufferCInput.Sample(InUsedSampler, InUV.xy).xyzw;
#else
	OutGBufferA.xyzw = float4(0.0, 0.0, 0.0, 0.0);
	OutGBufferB.xyzw = float4(0.0, 0.0, 0.0, 0.0);
	OutGBufferC.xyzw = float4(0.0, 0.0, 0.0, 0.0);
#endif
}

void LoadGBuffer(in uint2 InTexCoord, out float4 OutGBufferA, out float4 OutGBufferB, out float4 OutGBufferC)
{
#if (SHADER_USE_GBUFFER_INPUT)
	OutGBufferA.xyzw = _ShaderGBufferAInput.Load(uint3(InTexCoord.xy, 0u)).xyzw;
	OutGBufferB.xyzw = _ShaderGBufferBInput.Load(uint3(InTexCoord.xy, 0u)).xyzw;
	OutGBufferC.xyzw = _ShaderGBufferCInput.Load(uint3(InTexCoord.xy, 0u)).xyzw;
#else
	OutGBufferA.xyzw = float4(0.0, 0.0, 0.0, 0.0);
	OutGBufferB.xyzw = float4(0.0, 0.0, 0.0, 0.0);
	OutGBufferC.xyzw = float4(0.0, 0.0, 0.0, 0.0);
#endif
}

void EncodeGBuffer(in GBufferTerm InGBuffer, out float4 OutGBufferA, out float4 OutGBufferB, out float4 OutGBufferC)
{
	float3	BaseColor				= InGBuffer.BaseColor;
	float	PrecomputedShadowFactor	= InGBuffer.PrecomputedShadowFactor;
	OutGBufferA.rgb	= BaseColor.rgb;
	OutGBufferA.a	= PrecomputedShadowFactor;

	float2 OctNormal = UnitVectorToOctahedron(InGBuffer.Normal.xyz).xy;
	OutGBufferB.rg	= OctNormal.xy;
	OutGBufferB.b	= 0.0;
	OutGBufferB.a	= 0.0;

	float	Roughness		= InGBuffer.Roughness;
	float	Reflectance		= InGBuffer.Reflectance;
	float	Metallicness	= InGBuffer.Metallicness;
	uint	ShadingModelID	= InGBuffer.ShadingModelID;
	OutGBufferC.r = Roughness;
	OutGBufferC.g = Reflectance;
	OutGBufferC.b = Metallicness;
	OutGBufferC.a = asfloat(((ShadingModelID & 0xfu) << 4u) | 0x0u);	// 0x0u is not used(4 bits).
}

void DecodeGBuffer(in float4 InGBufferA, in float4 InGBufferB, in float4 InGBufferC, out GBufferTerm OutGBuffer)
{
	float3	BaseColor				= InGBufferA.rgb;
	float	PrecomputedShadowFactor = InGBufferA.a;
	OutGBuffer.BaseColor.rgb			= BaseColor.rgb;
	OutGBuffer.PrecomputedShadowFactor	= PrecomputedShadowFactor;

	float3 Normal = OctahedronToUnitVector(InGBufferB.rg).xyz;
	OutGBuffer.Normal.xyz = Normal.xyz;

	float	Roughness		= InGBufferC.r;
	float	Reflectance		= InGBufferC.g;
	float	Metallicness	= InGBufferC.b;
	uint	ShadingModelID	= ((asuint(InGBufferC.a)) >> 4u) & 0xfu;
	OutGBuffer.Roughness		= Roughness;
	OutGBuffer.Reflectance		= Reflectance;
	OutGBuffer.Metallicness		= Metallicness;
	OutGBuffer.ShadingModelID	= ShadingModelID;
}

#endif	//_SHADER_DEFERRED_COMMON_HLSL