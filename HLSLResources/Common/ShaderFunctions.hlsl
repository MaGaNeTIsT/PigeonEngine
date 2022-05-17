#ifndef _SHADER_FUNCTIONS_HLSL
#define _SHADER_FUNCTIONS_HLSL

#include "./ShaderNoises.hlsl"

struct FieldvInput {
	float4 Position			: POSITION0;
	float4 Normal			: NORMAL0;
	float4 Diffuse			: COLOR0;
	float2 TexCoord			: TEXCOORD0;
};
struct FieldpInput {
	float4 Position			: SV_POSITION;
	float4 WorldPosition	: POSITION0;
	float4 Diffuse			: COLOR0;
	float2 TexCoord			: TEXCOORD0;
};
struct FieldFrontpOutput {
	float4 Diffuse			: SV_Target;
};
struct FieldDeferredpOutput {
	float4 WorldPosition	: SV_Target0;
	float4 WorldNormal		: SV_Target1;
	float4 Diffuse			: SV_Target2;
	float4 Specular			: SV_Target3;
};

void ComputeDirectionalLight(
	float4 Ambient, float4 Diffuse, float4 Specular,
	float4 LAmbient, float4 LDiffuse, float4 LSpecular, float3 LDirection,
	float3 normal, float3 toEye,
	out float4 ambient, out float4 diffuse, out float4 spec)
{

	ambient = float4(0.0, 0.0, 0.0, 0.0);
	diffuse = float4(0.0, 0.0, 0.0, 0.0);
	spec = float4(0.0, 0.0, 0.0, 0.0);

	LDirection = normalize(LDirection);
	float3 lightVec = -LDirection;

	ambient = Ambient * LAmbient;

	float diffuseFactor = dot(lightVec, normal);

	[flatten]
	if (diffuseFactor > 0)
	{
		float3 v = reflect(-lightVec, normal);
		float specFactor = pow(max(dot(v, toEye), 0.0), Specular.w);
		diffuse = diffuseFactor * Diffuse * LDiffuse;
		spec = specFactor * Specular * LSpecular;
	}

}

void ComputeBlinnPhongLight(	in float	Ka,
								in float3	Kd,
								in float3	Ks,
								in float	Shininess,
								in float3	Normal,
								in float3	LightDir,
								in float3	LightColor,
								in float3	GlobalAmbient,
								in float3	EyeDir,

								out float3	outAmbient,
								out float3	outDiffuse,
								out float3	outSpecular)
{

	float3 H = normalize((-LightDir) + (-EyeDir));
	outAmbient = Ka * GlobalAmbient;
	float diffuseLight = max(dot(Normal, -LightDir), 0.0);
	outDiffuse = Kd * LightColor * diffuseLight;
	float specularLight = pow(max(dot(H, Normal), 0.0), Shininess);
	[flatten]
	if (diffuseLight < 0.000001)
		specularLight = 0.0;
	outSpecular = Ks * LightColor * specularLight;

}

int ComputeShadow(		in matrix			matView,
						in matrix			matProj,
						in float3			worldPos,
						in Texture2D		shadowMap,
						in SamplerState		samplerS)
{

	float4 worldP = float4(worldPos, 1.0);
	worldP = mul(worldP, matView);
	worldP = mul(worldP, matProj);

	float3 shadowPos = worldP.xyz / worldP.w;
	shadowPos.x = shadowPos.x * 0.5 + 0.5;
	shadowPos.y = shadowPos.y * (-0.5) + 0.5;
	[flatten]
	if ((saturate(shadowPos.x) == shadowPos.x) && (saturate(shadowPos.y) == shadowPos.y) && (shadowPos.z > 0))
	{
		float ShadowDepth = shadowMap.Sample(samplerS, shadowPos.xy).r + 0.001;
		[flatten]
		if (ShadowDepth < shadowPos.z)
			return 1;
	}
	return 0;

}

struct GerstnerWaveParam
{
	float  Amplitude;
	float  Steepness;
	float  WaveSpeed;
	float  WaveLength;
	float2 Direction;
};

float3 GerstnerWaveCalculatePos(float3 Pos, float Time)
{
	GerstnerWaveParam gWaveData[3];
	gWaveData[0].Amplitude = 0.25;
	gWaveData[0].Steepness = 0.8;
	gWaveData[0].WaveSpeed = 4.5;
	gWaveData[0].WaveLength = 5.0;
	gWaveData[0].Direction = float2(0.707106, 0.707106);
	gWaveData[1].Amplitude = 0.125;
	gWaveData[1].Steepness = 0.4;
	gWaveData[1].WaveSpeed = 1.0;
	gWaveData[1].WaveLength = 2.5;
	gWaveData[1].Direction = float2(1.0, 0.0);
	gWaveData[2].Amplitude = 0.0625;
	gWaveData[2].Steepness = 0.2;
	gWaveData[2].WaveSpeed = 6.0;
	gWaveData[2].WaveLength = 1.25;
	gWaveData[2].Direction = float2(0.866025, 0.5);
	float Height = 0.0;
	float PosXOffset = 0.0;
	float PosZOffset = 0.0;
	float x = Pos.x;
	float z = Pos.z;
	for (int i = 0; i < 3; ++i)
	{
		float fDdotXZ = gWaveData[i].Direction.x * x + gWaveData[i].Direction.y * z;
		float w = 2.0 * CUSTOM_SHADER_PI / gWaveData[i].WaveLength;
		float phaseConstant = gWaveData[i].WaveSpeed * w;
		float alpha = fDdotXZ * w + Time * phaseConstant;
		Height += gWaveData[i].Amplitude * sin(alpha);
		PosXOffset += gWaveData[i].Amplitude * gWaveData[i].Steepness * gWaveData[i].Direction.x * cos(alpha);
		PosZOffset += gWaveData[i].Amplitude * gWaveData[i].Steepness * gWaveData[i].Direction.y * cos(alpha);
	}
	return float3(PosXOffset, Height, PosZOffset);
}

void GerstnerWaveCalculateTBN(
	float3 Pos, float Time,
	out float3 T, out float3 B, out float3 N)
{
	GerstnerWaveParam gWaveData[3];
	gWaveData[0].Amplitude = 0.25;
	gWaveData[0].Steepness = 0.8;
	gWaveData[0].WaveSpeed = 4.5;
	gWaveData[0].WaveLength = 10.0;
	gWaveData[0].Direction = float2(0.707106, 0.707106);
	gWaveData[1].Amplitude = 0.125;
	gWaveData[1].Steepness = 0.4;
	gWaveData[1].WaveSpeed = 1.0;
	gWaveData[1].WaveLength = 5.0;
	gWaveData[1].Direction = float2(1.0, 0.0);
	gWaveData[2].Amplitude = 0.0625;
	gWaveData[2].Steepness = 0.2;
	gWaveData[2].WaveSpeed = 6.0;
	gWaveData[2].WaveLength = 2.5;
	gWaveData[2].Direction = float2(0.866025, 0.5);
	T = float3(0.0, 0.0, 0.0);
	B = float3(0.0, 0.0, 0.0);
	N = float3(0.0, 0.0, 0.0);
	float x = Pos.x;
	float z = Pos.z;
	float Tx = 0.0;
	float Ty = 0.0;
	float Tz = 0.0;
	float Bx = 0.0;
	float By = 0.0;
	float Bz = 0.0;
	for (int i = 0; i < 3; ++i)
	{
		float fDdotXZ = gWaveData[i].Direction.x * x + gWaveData[i].Direction.y * z;
		float w = 2.0 * CUSTOM_SHADER_PI / gWaveData[i].WaveLength;
		float phaseConstant = gWaveData[i].WaveSpeed * w;
		float alpha = fDdotXZ * w + Time * phaseConstant;
		Tx += gWaveData[i].Amplitude * gWaveData[i].Steepness * w *
			gWaveData[i].Direction.x * gWaveData[i].Direction.x * sin(alpha);
		Ty += gWaveData[i].Amplitude * w * gWaveData[i].Direction.x * cos(alpha);
		Tz += gWaveData[i].Amplitude * gWaveData[i].Steepness * w *
			gWaveData[i].Direction.x * gWaveData[i].Direction.y * sin(alpha);
		By += gWaveData[i].Amplitude * w * gWaveData[i].Direction.y * cos(alpha);
		Bz += gWaveData[i].Amplitude * gWaveData[i].Steepness * w *
			gWaveData[i].Direction.y * gWaveData[i].Direction.y * sin(alpha);
	}
	Bx = Tz;
	T = float3(1.0 - Tx, Ty, 0.0 - Tz);
	B = float3(0.0 - Bx, By, 1.0 - Bz);
	N = cross(B, T);
}

#endif