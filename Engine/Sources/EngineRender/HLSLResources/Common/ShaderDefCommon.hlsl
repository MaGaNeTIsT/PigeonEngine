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
	float4 color		: SV_Target0;
	float4 normal		: SV_Target1;
	float4 albedo		: SV_Target2;
	float4 property		: SV_Target3;
};
struct AttributeTerrain
{
	float4 position		: POSITION0;
	float4 normal		: NORMAL0;
	float4 tangent		: TANGENT0;
	float2 uv0			: TEXCOORD0;
};
struct VaryingTerrain
{
	float4 positionCS	: SV_POSITION;
	float4 normal		: NORMAL0;
	float4 tangent		: TANGENT0;
	float2 uv0			: TEXCOORD0;
};
struct AttributeScreenPolygon2D
{
	float4 position		: POSITION0;
	float2 uv0			: TEXCOORD0;
};
struct VaryingScreenPolygon2D
{
	float4 positionCS	: SV_POSITION;
	float2 uv0			: TEXCOORD0;
};
struct AttributeSkyBox
{
	float4 position		: POSITION0;
	float2 uv0			: TEXCOORD0;
};
struct VaryingSkyBox
{
	float4 positionCS	: SV_POSITION;
	float2 uv0			: TEXCOORD0;
};

#endif