#ifndef _SHADER_DEFINE_COMMON_HLSL
#define _SHADER_DEFINE_COMMON_HLSL

#define CUSTOM_SHADER_PI		(3.141592653589793)
#define CUSTOM_SHADER_PI_HALF	(1.570796326794897)

struct DirectionalLightData
{
	float4 forward;
	float4 color;
};
struct Attribute
{
	float4 position		: POSITION0;
	float4 normal		: NORMAL0;
	float4 tangent		: TANGENT0;
	float4 color		: COLOR0;
	float2 uv0			: TEXCOORD0;
};
struct Varying
{
	float4 positionCS	: SV_POSITION;
	float4 normal		: NORMAL0;
	float4 tangent		: TANGENT0;
	float2 uv0			: TEXCOORD0;
};
struct DeferredOutput
{
	float4 normalWS		: SV_Target0;
	float4 albedo		: SV_Target1;
	float4 property		: SV_Target2;
	float4 id			: SV_Target3;
};
struct VaryingScreenPolygon2D
{
	float4 positionCS	: SV_POSITION;
	float2 uv0			: TEXCOORD0;
};

#endif