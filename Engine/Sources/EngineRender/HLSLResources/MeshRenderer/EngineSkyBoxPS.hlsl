#ifndef _ENGINE_SKY_BOX_PS_HLSL
#define _ENGINE_SKY_BOX_PS_HLSL

#include "../Common/ShaderCommon.hlsl"

cbuffer ConstantBufferSkyBox : register(b3)
{
	float4 _SkyBoxParameter;	//x y z is origin position of sky sphere. w is radius of sky sphere.
};
TextureCube _SkyBoxCubeMap : register(t5);

float3 HitPositionInSphere(float3 rayOrigin, float3 rayDirection, float3 sphereOrigin, float radius)
{
	float3 rayToSphere = sphereOrigin - rayOrigin;
	float rayLength = length(rayToSphere);
	{
		rayToSphere = rayToSphere / rayLength;
		bool isNaN = any(isinf(rayToSphere));
		rayToSphere = isNaN ? float3(0.0, 0.0, 0.0) : rayToSphere;
	}
	float sinAngle = dot(rayDirection, rayToSphere);
	float d0 = rayLength * sinAngle;
	float vSq = rayLength * rayLength - d0 * d0;
	float d1Sq = radius * radius - vSq;
	return (rayOrigin + rayDirection * (sqrt(d1Sq) + d0));
}

float4 main(VaryingSkyBox input) :SV_Target
{
	const float3 skyOrigin = _SkyBoxParameter.xyz;
	const float skyRadius = _SkyBoxParameter.w;
	float2 hdc = (input.uv0 - 0.5) * 2.0;
	hdc.y = -hdc.y;
	float3 posWS = TransformClipToWorld(float4(hdc, 1.0, 1.0));
	float3 viewDirWS = SafeNormalize(posWS - GetCameraWorldPosition());
	float3 skyPos = HitPositionInSphere(GetCameraWorldPosition(), viewDirWS, skyOrigin, skyRadius);
	float3 sampleVec = SafeNormalize(skyPos - skyOrigin);
	float3 color = _SkyBoxCubeMap.Sample(_LinearClampSampler, sampleVec).rgb;
	return float4(color, 1.0);
}

#endif