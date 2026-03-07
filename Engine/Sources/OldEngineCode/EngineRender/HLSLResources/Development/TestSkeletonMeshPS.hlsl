//#ifndef _ENGINE_TEST_SKELETON_MESH_PS_HLSL
//#define _ENGINE_TEST_SKELETON_MESH_PS_HLSL
//
//float3 SafeNormalize(const float3 input)
//{
//	float3 output = normalize(input);
//	bool i = any(isinf(output));
//	output = i ? float3(0.0, 0.0, 1.0) : output;
//	return output;
//}
//
//struct LightParams
//{
//	float4	Color;
//	float4	Params0;
//	float4	Params1;
//};
//
//struct ShadingLightParams
//{
//	float4	ColorIntensity;
//	float3	Direction;
//	float	Attenuation;
//};
//
//cbuffer ConstantBufferPerFrame : register(b0)
//{
//	float4x4		_ViewMatrix;
//	float4x4		_ViewInvMatrix;
//	float4x4		_ProjectionMatrix;
//	float4x4		_ProjectionInvMatrix;
//	float4x4		_ViewProjectionMatrix;
//	float4x4		_ViewProjectionInvMatrix;
//	float4x4		_LightViewProjectionMatrix;
//	float4			_TimeParams;
//	float4			_DepthMultiAdd;
//	float4			_ScreenToViewSpaceParams;
//	float4			_CameraViewportMinSizeAndInvBufferSize;
//	float4			_CameraViewportSizeAndInvSize;
//	float4			_CameraViewportRect;
//	float4			_CameraWorldPosition;
//};
//cbuffer ConstantBufferLightData : register(b1)
//{
//	uint4			_LightCount;
//	LightParams		_LightData[16];
//}
//cbuffer ConstantBufferPerDraw : register(b2)
//{
//	float4x4		_WorldMatrix;
//	float4x4		_WorldInvMatrix;
//	float4x4		_WorldInvTransposeMatrix;
//};
//
//struct TestVarying
//{
//	float4	positionCS	: SV_POSITION;
//	float4	normal		: NORMAL0;
//};
//
//float4 main(TestVarying input) : SV_Target
//{
//	float4 color = float4(0.0.xxx, 1.0);
//	for (uint i = 0u; i < (uint)_LightCount.x; i++)
//	{
//		ShadingLightParams light;
//		light.ColorIntensity	= _LightData[i].Color;
//		light.Direction			= -_LightData[i].Params0.xyz;
//		light.Attenuation		= 1.0;
//
//		color.rgb += light.ColorIntensity.rgb * saturate(dot(light.Direction.xyz, SafeNormalize(input.normal.xyz))) * light.Attenuation;
//	}
//	return color;
//}
//
//#endif