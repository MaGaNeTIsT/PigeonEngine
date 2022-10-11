#ifndef _ENGINE_SKY_BOX_PS_HLSL
#define _ENGINE_SKY_BOX_PS_HLSL

#include "../Common/ShaderCommon.hlsl"

TextureCube _SkyBoxCubeMap : register(t3);

float3 HitPositionInSphere(float3 rayOrigin, float3 rayDirection, float3 sphereOrigin, float radius)
{
	float3 rayToSphere = sphereOrigin - rayOrigin;
	float rayLength = length(rayToSphere);
	rayToSphere = rayToSphere / rayLength;
	bool isNaN = any(isinf(rayToSphere));
	rayToSphere = isNaN ? float3(0.0, 0.0, 0.0) : rayToSphere;
}

float4 main(VaryingSkyBox input) :SV_Target
{
	float2 hdc = (input.uv - 0.5) * 2.0;
	hdc.y = -hdc.y;
	float3 posWS = TransformClipToWorld(float4(hdc, 1.0, 1.0));
	float3 viewDirWS = SafeNormalize(posWS - GetCameraWorldPosition());

	return float4(1.0, 1.0, 1.0, 1.0);
}

#endif