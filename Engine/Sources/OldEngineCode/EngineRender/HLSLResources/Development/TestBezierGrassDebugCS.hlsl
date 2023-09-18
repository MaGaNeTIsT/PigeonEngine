//#ifndef _ENGINE_TEST_BEZIER_GRASS_DEBUG_CS_HLSL
//#define _ENGINE_TEST_BEZIER_GRASS_DEBUG_CS_HLSL
//
//#include "./BezierGrassCommon.hlsl"
//
//cbuffer BezierGrassDebugConstantBuffer : register(b2)
//{
//	int4	_Parameter;		// x is total threads num;
//	float4	_StartEndPoint;
//	float4	_ControlPoint;
//};
//
//RWTexture2D<uint> _DebugResult : register(u0);
//
//[numthreads(8, 1, 1)]
//void main(uint3 DispatchThreadID : SV_DispatchThreadID)
//{
//	uint ThreadSize = (uint)(_Parameter.x);
//	if (DispatchThreadID.x >= ThreadSize)
//	{
//		return;
//	}
//
//	float2 StartPoint = _StartEndPoint.xy;
//	float2 EndPoint = _StartEndPoint.zw;
//	float2 StartControl = _ControlPoint.xy;
//	float2 EndControl = _ControlPoint.zw;
//	float T = ((float)(DispatchThreadID.x)) / ((float)(ThreadSize - 1u));
//
//	//float2 TargetPos = CubicBezierCurve(StartPoint, StartControl, EndControl, EndPoint, T);
//	float2 TargetPos = QuadraticBezierCurve(StartPoint, EndControl, EndPoint, T);
//	
//	uint2 ResultSize;
//	_DebugResult.GetDimensions(ResultSize.x, ResultSize.y);
//
//	uint2 WriteCoord = min((uint2)(floor(TargetPos * ((float2)(ResultSize - uint2(1u, 1u))))), ResultSize - uint2(1u, 1u));
//	uint OutputOld;
//	InterlockedAdd(_DebugResult[WriteCoord], 1u, OutputOld);
//}
//
//#endif