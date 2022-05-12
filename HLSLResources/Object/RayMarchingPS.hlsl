#ifndef _SHADER_RAY_MARCHING_PS_HLSL
#define _SHADER_RAY_MARCHING_PS_HLSL

#include "../Common/ShaderFunctions.hlsl"

cbuffer ConstatntBuffer : register(b0)
{
	matrix World;
	matrix View;
	matrix Projection;

	float4 CameraPosition;
	float4 Parameter;

}

void CloudAlpha(in float3 pos, out float a, float2 limit)
{
	a = 0.0;
	if (pos.y > limit.x && pos.y < limit.y)
	{
		a = fbm3(pos * 0.05, 4, Parameter.x * 0.55);
		a = saturate(a);
	}
}

void CloudMarching(in float3 firstpos, in float3 rayvec, in float3 eyepos, out float a, 
	float2 limit = float2(10.0, 20.0), int raystepnum = 500, float raysteplen = 0.25, float raylimitdist = 100.0)
{
	a = 0.0;
	float dist = distance(firstpos, eyepos);
	if (dist < raylimitdist)
	{
		int down = firstpos.y < limit.x, mid = ((firstpos.y > limit.x) && (firstpos.y < limit.y)), up = firstpos.y > limit.y;
		if ((down && rayvec.y > 0.02) || (up && rayvec.y < -0.02) || mid)
		{
			float3 steppos = firstpos;
			float stepa = 0.0;
			for (int i = 0; i < raystepnum; i++)
			{
				CloudAlpha(steppos, stepa, limit);
				if (stepa > 0.01)
					break;
				steppos += rayvec * raysteplen;
				if ((down && steppos.y > limit.y) || (up && steppos.y < limit.x) || (mid && (steppos.y < limit.x || steppos.y > limit.y)))
					break;
				dist += raysteplen;
				if (dist > raylimitdist)
					break;
			}
			if (stepa > 0.01)
			{
				float stepaa = 0.0;
				for (int i = 0; i < 100; i++)
				{
					float sl = 0.1;
					steppos += rayvec * sl;
					if ((down && steppos.y > limit.y) || (up && steppos.y < limit.x) || (mid && (steppos.y < limit.x || steppos.y > limit.y)))
						break;
					dist += sl;
					if (dist > raylimitdist)
						break;
					CloudAlpha(steppos, stepaa, limit);
					stepa = 1.0 - ((1.0 - stepa) * (1.0 - stepaa));
					if (stepa > 0.95)
					{
						stepa = 1.0;
						break;
					}
				}
			}
			a = stepa;
		}
	}
}

void main(
	in  float4 inPosition			: SV_POSITION,
	in  float4 inWorldPosition		: POSITION0,
	in  float4 inNormal				: NORMAL0,
	in  float4 inDiffuse			: COLOR0,
	in  float2 inTexCoord			: TEXCOORD0,

	out float4 outDiffuse			: SV_Target)
{
	//float3 eye = normalize(inWorldPosition.xyz - CameraPosition.xyz);
	//float a;
	//CloudMarching(inWorldPosition, eye, CameraPosition.xyz, a);
	//outDiffuse.rgb = HenyeyGreenstein(Parameter.yzw, eye, 0.0);
	//outDiffuse.a = 1.0 - exp(-a);

	outDiffuse = 0.0;
}

#endif