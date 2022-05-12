#ifndef _SHADER_SKY_PS_HLSL
#define _SHADER_SKY_PS_HLSL

#include "../Common/ShaderFunctions.hlsl"

cbuffer ConstatntBuffer : register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;

    float4 CameraPosition;
    float4 Parameter;

}

static float SKY_RANGE_XZ = 0.019635; // xz / 80.0 * PI / 2
static float SKY_RANGE_Y_LOW = 3.162278; // sqrt(10)
static float SKY_RANGE_Y_HIGH = 4.472136; // sqrt(20)

bool MarchSkyHeight(in float3 Position, in float3 RayDirection)
{
	bool beMarched = false;
	float Height0 = 0.0, Height1 = 0.0;
	float3 Normal0 = 0.0, Normal1 = 0.0;

	float x = Position.x * SKY_RANGE_XZ;
	float z = Position.z * SKY_RANGE_XZ;
	float derivative = 0.0;

	Height0 = SKY_RANGE_Y_LOW * cos(x);
	Height0 *= SKY_RANGE_Y_LOW * cos(z);

	derivative = -SKY_RANGE_Y_LOW * SKY_RANGE_XZ * sin(x);
	if (abs(derivative) < 0.0001)
		Normal0 = float3(0.0, 1.0, 0.0);
	else
		Normal0 = normalize(float3(Position.x, -Position.x / derivative, 0.0));

	derivative = -SKY_RANGE_Y_LOW * SKY_RANGE_XZ * sin(z);
	if (abs(derivative) < 0.0001)
		Normal0 += float3(0.0, 1.0, 0.0);
	else
		Normal0 += normalize(float3(Position.z, -Position.z / derivative, 0.0));

	Normal0 = normalize(Normal0);

	Height1 = SKY_RANGE_Y_HIGH * cos(x);
	Height1 *= SKY_RANGE_Y_HIGH * cos(z);

	derivative = -SKY_RANGE_Y_HIGH * SKY_RANGE_XZ * sin(x);
	if (abs(derivative) < 0.0001)
		Normal1 = float3(0.0, 1.0, 0.0);
	else
		Normal1 = normalize(float3(Position.x, -Position.x / derivative, 0.0));

	derivative = -SKY_RANGE_Y_HIGH * SKY_RANGE_XZ * sin(z);
	if (abs(derivative) < 0.0001)
		Normal1 += float3(0.0, 1.0, 0.0);
	else
		Normal1 += normalize(float3(Position.z, -Position.z / derivative, 0.0));

	Normal1 = normalize(Normal1);

	[flatten]
	if ((Position.y < Height0 && dot(Normal0, RayDirection) < -0.2) ||
		(Position.y > Height1 && dot(Normal1, RayDirection) > -0.2))
	{
		beMarched = true;
		return beMarched;
	}
	beMarched = false;
	return beMarched;
}

void March0(in float3 RayOrigin, in float3 RayDirection, in float3 SunDirection, in float3 BackGroundColor,
	in out float Step, in out float4 Result, float Time = 0.0)
{
	for (int i = 0; i < 40; i++)
	{
		float3 pos = RayOrigin + Step * RayDirection;

		//if (pos.y < 10.0 || pos.y > 20.0 || Result.a > 0.99)
		//	break;
		if (MarchSkyHeight(pos, RayDirection) || Result.a > 0.99)
			break;

		float density = cloud5(pos * 0.25, Time);
		if (density > 0.01)
		{
			float r = clamp((density - cloud5((pos + 0.3 * -SunDirection) * 0.25, Time)) / 0.6, 0.0, 1.0);
			float3  l = float3(0.65, 0.7, 0.75) * 1.4 + float3(1.0, 0.6, 0.3) * r;
			float4  color = float4(lerp(float3(1.0, 0.95, 0.8), float3(0.25, 0.3, 0.35), density), density);
			color.xyz *= l;
			color.xyz = lerp(color.xyz, BackGroundColor, 1.0 - exp(-0.003 * Step * Step));
			color.w *= 0.4;

			color.rgb *= color.a;
			Result += color * (1.0 - Result.a);
		}
		Step += max(0.05, 0.02 * Step);
	}
}
void March1(in float3 RayOrigin, in float3 RayDirection, in float3 SunDirection, in float3 BackGroundColor,
	in out float Step, in out float4 Result, float Time = 0.0)
{
	for (int i = 0; i < 40; i++)
	{
		float3 pos = RayOrigin + Step * RayDirection;

		//if (pos.y < 10.0 || pos.y > 20.0 || Result.a > 0.99)
		//	break;
		if (MarchSkyHeight(pos, RayDirection) || Result.a > 0.99)
			break;

		float density = cloud4(pos * 0.25, Time);
		if (density > 0.01)
		{
			float r = clamp((density - cloud4((pos + 0.3 * -SunDirection) * 0.25, Time)) / 0.6, 0.0, 1.0);
			float3  l = float3(0.65, 0.7, 0.75) * 1.4 + float3(1.0, 0.6, 0.3) * r;
			float4  color = float4(lerp(float3(1.0, 0.95, 0.8), float3(0.25, 0.3, 0.35), density), density);
			color.xyz *= l;
			color.xyz = lerp(color.xyz, BackGroundColor, 1.0 - exp(-0.003 * Step * Step));
			color.w *= 0.4;

			color.rgb *= color.a;
			Result += color * (1.0 - Result.a);
		}
		Step += max(0.05, 0.02 * Step);
	}
}
void March2(in float3 RayOrigin, in float3 RayDirection, in float3 SunDirection, in float3 BackGroundColor,
	in out float Step, in out float4 Result, float Time = 0.0)
{
	for (int i = 0; i < 30; i++)
	{
		float3 pos = RayOrigin + Step * RayDirection;

		//if (pos.y < 10.0 || pos.y > 20.0 || Result.a > 0.99)
		//	break;
		if (MarchSkyHeight(pos, RayDirection) || Result.a > 0.99)
			break;

		float density = cloud3(pos * 0.25, Time);
		if (density > 0.01)
		{
			float r = clamp((density - cloud3((pos + 0.3 * -SunDirection) * 0.25, Time)) / 0.6, 0.0, 1.0);
			float3  l = float3(0.65, 0.7, 0.75) * 1.4 + float3(1.0, 0.6, 0.3) * r;
			float4  color = float4(lerp(float3(1.0, 0.95, 0.8), float3(0.25, 0.3, 0.35), density), density);
			color.xyz *= l;
			color.xyz = lerp(color.xyz, BackGroundColor, 1.0 - exp(-0.003 * Step * Step));
			color.w *= 0.4;

			color.rgb *= color.a;
			Result += color * (1.0 - Result.a);
		}
		Step += max(0.05, 0.02 * Step);
	}
}
void March3(in float3 RayOrigin, in float3 RayDirection, in float3 SunDirection, in float3 BackGroundColor,
	in out float Step, in out float4 Result, float Time = 0.0)
{
	for (int i = 0; i < 30; i++)
	{
		float3 pos = RayOrigin + Step * RayDirection;

		//if (pos.y < 10.0 || pos.y > 20.0 || Result.a > 0.99)
		//	break;
		if (MarchSkyHeight(pos, RayDirection) || Result.a > 0.99)
			break;

		float density = cloud2(pos * 0.25, Time);
		if (density > 0.01)
		{
			float r = clamp((density - cloud2((pos + 0.3 * -SunDirection) * 0.25, Time)) / 0.6, 0.0, 1.0);
			float3  l = float3(0.65, 0.7, 0.75) * 1.4 + float3(1.0, 0.6, 0.3) * r;
			float4  color = float4(lerp(float3(1.0, 0.95, 0.8), float3(0.25, 0.3, 0.35), density), density);
			color.xyz *= l;
			color.xyz = lerp(color.xyz, BackGroundColor, 1.0 - exp(-0.003 * Step * Step));
			color.w *= 0.4;

			color.rgb *= color.a;
			Result += color * (1.0 - Result.a);
		}
		Step += max(0.05, 0.02 * Step);
	}
}

void RayMarch(in float3 RayDirection, in float3 RayOrigin, in float3 SunDirection,
	in float3 BackGroundColor, out float4 Result, float Time = 0.0)
{
	Result = 0.0;

	float step = 0.0;

	March0(RayOrigin, RayDirection, SunDirection, BackGroundColor, step, Result, Time);
	March1(RayOrigin, RayDirection, SunDirection, BackGroundColor, step, Result, Time);
	March2(RayOrigin, RayDirection, SunDirection, BackGroundColor, step, Result, Time);
	March3(RayOrigin, RayDirection, SunDirection, BackGroundColor, step, Result, Time);

	Result = clamp(Result, 0.0, 1.0);
}

void TestCloud(in float3 RayDirection, in float3 RayOrigin, in float3 SunDirection, out float4 Color, float Time = 0.0)
{
	float sun = clamp(dot(SunDirection, RayDirection), 0.0, 1.0);
	Color.xyz = float3(0.6, 0.71, 0.75) - RayDirection.y * 0.2 * float3(1.0, 0.5, 1.0) + 0.15 * 0.5;
	Color.xyz += 0.2 * float3(1.0, .6, 0.1) * pow(sun, 8.0);

	float4 result;
	RayMarch(RayDirection, RayOrigin, SunDirection, Color.xyz, result, Time);
	Color.xyz = Color.xyz * (1.0 - result.w) + result.xyz;

	Color.xyz += 0.2 * float3(1.0, 0.4, 0.2) * pow(sun, 3.0);
	Color.w = 1.0;
}

void main(	in float4 inPosition		: SV_POSITION,
            in float4 inWorldPosition	: POSITION0,
			in float4 inNormal			: NORMAL0,
			in float4 inDiffuse			: COLOR0,
			in float2 inTexCoord		: TEXCOORD0,

			out float4 outDiffuse		: SV_Target)
{
	float3 eye = normalize(inWorldPosition.xyz - CameraPosition.xyz);
	TestCloud(eye, inWorldPosition.xyz, normalize(Parameter.yzw), outDiffuse, Parameter.x * 0.25);
    outDiffuse.a = 1.0;
}

#endif