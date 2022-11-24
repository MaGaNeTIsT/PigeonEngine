#ifndef _SHADER_DEFINE_COMMON_HLSL
#define _SHADER_DEFINE_COMMON_HLSL

#define ENABLE_CONSOLE_SHADING

#define MEDIUM_PRECISION_FLT_MAX		(65504.0)

#ifdef ENABLE_CONSOLE_SHADING
#define SaturateMediumPrecision(x)		(x)
#else
#define SaturateMediumPrecision(x)		(min(x, MEDIUM_PRECISION_FLT_MAX))
#endif

#define CUSTOM_SHADER_PI				(3.1415926535897932)
#define CUSTOM_SHADER_PI_HALF			(1.5707963267948966)
#define CUSTOM_SHADER_PI_DERIVATIVE		(0.3183098861837907)

#define GLOBAL_LIGHT_COUNT				(16)

struct LightParams
{
	float4	Color;
	float4	Params0;
	float4	Params1;
};
struct GBufferTerm
{
	float3	NormalWS;
	float3	BaseColor;
	float3	Emissive;
	float	Roughness;
	float	Metallicness;
	float	AmbientOcclusion;
};
struct PixelParams
{
	float3	DiffuseColor;
	float3	SpecularColor;
	float3	F0;
	float	PerceptualRoughness;
	float	PerceptualRoughnessUnclamped;
	float	Roughness;
	float3	SheenColor;
#ifdef CLOTH_HAS_SUBSURFACE_COLOR
	float3	SubsurfaceColor;
#endif
};
struct ShadingLightParams
{
	float4	ColorIntensity;
	float3	Direction;
	float	Attenuation;
};
struct NormalViewLightDotParams
{
	float	LdotH;
	float	NdotV;
	float	NdotL;
	float	NdotH;
};
struct NormalViewLightDotParams_Anisotropic
{
	float	LdotH;
	float	NdotV;
	float	NdotL;
	float	NdotH;
	float	TdotV;
	float	TdotL;
	float	TdotH;
	float	BdotV;
	float	BdotL;
	float	BdotH;
};
struct AnisotropicRoughness
{
	float	at;
	float	ab;
};
struct BRDFTerm
{
	float3	Fd;
	float3	Fr;
};
struct Attribute
{
	float4	position	: POSITION0;
	float4	normal		: NORMAL0;
	float4	tangent		: TANGENT0;
	float2	uv0			: TEXCOORD0;
};
struct Varying
{
	float4	positionCS	: SV_POSITION;
	float4	normal		: NORMAL0;
	float4	tangent		: TANGENT0;
	float2	uv0			: TEXCOORD0;
};
struct DeferredOutput
{
	float4	color		: SV_Target0;
	float4	normal		: SV_Target1;
	float4	albedo		: SV_Target2;
	float4	property	: SV_Target3;
};
struct AttributeTerrain
{
	float4	position	: POSITION0;
	float4	normal		: NORMAL0;
	float4	tangent		: TANGENT0;
	float2	uv0			: TEXCOORD0;
};
struct VaryingTerrain
{
	float4	positionCS	: SV_POSITION;
	float4	normal		: NORMAL0;
	float4	tangent		: TANGENT0;
	float2	uv0			: TEXCOORD0;
};
struct AttributeCloth
{
	float4	position	: POSITION0;
	float4	normal		: NORMAL0;
	float4	tangent		: TANGENT0;
	float2	uv0			: TEXCOORD0;
};
struct VaryingCloth
{
	float4	positionCS	: SV_POSITION;
	float4	normal		: NORMAL0;
	float4	tangent		: TANGENT0;
	float2	uv0			: TEXCOORD0;
	float4	positionWS	: TEXCOORD1;
	float4	positionSS	: TEXCOORD2;
};
struct AttributeScreenPolygon2D
{
	float4	position	: POSITION0;
	float2	uv0			: TEXCOORD0;
};
struct VaryingScreenPolygon2D
{
	float4	positionCS	: SV_POSITION;
	float2	uv0			: TEXCOORD0;
};
struct AttributeSkyBox
{
	float4	position	: POSITION0;
	float2	uv0			: TEXCOORD0;
};
struct VaryingSkyBox
{
	float4	positionCS	: SV_POSITION;
	float2	uv0			: TEXCOORD0;
};

#endif