#ifndef _ENGINE_TEST_BEZIER_GRASS_DEBUG_SCREEN_CS_HLSL
#define _ENGINE_TEST_BEZIER_GRASS_DEBUG_SCREEN_CS_HLSL

#include "./BezierGrassCommon.hlsl"

Texture2D<uint>		_InputDebugResult	: register(t5);
RWTexture2D<float4>	_OutputDebugResult	: register(u0);

[numthreads(8, 8, 1)]
void main(uint3 DispatchThreadID : SV_DispatchThreadID)
{
	uint2 OutputResultSize;
	_OutputDebugResult.GetDimensions(OutputResultSize.x, OutputResultSize.y);

	if (any(DispatchThreadID.xy >= OutputResultSize))
	{
		return;
	}

	_OutputDebugResult[DispatchThreadID.xy] = _InputDebugResult.Load(int3(DispatchThreadID.x, OutputResultSize.y - 1u - DispatchThreadID.y, 0)).r > 0u ? float4(1.0, 1.0, 1.0, 1.0) : float4(0.0, 0.0, 0.0, 1.0);
}

#endif