#ifndef _DYNAMIC_WIND_DIFFUSION_CS_HLSL
#define _DYNAMIC_WIND_DIFFUSION_CS_HLSL

#include "./DynamicWindCommon.hlsl"

cbuffer	ConstantComputeBuffer : register(b3)
{
	int4	_DynamicWindParams0;
	float4	_DynamicWindParams1; // x is (dt * diff * w * h * l).
};
RWTexture3D<int> _VelocityBuffer0 : register(u0);
RWTexture3D<int> _VelocityBuffer1 : register(u1);

[numthreads(GROUP_THREAD_SIZE_X, GROUP_THREAD_SIZE_Y, GROUP_THREAD_SIZE_Z)]
void main(uint3 dispatchThreadID :SV_DispatchThreadID)
{
	if (any(dispatchThreadID.xyz >= uint3(_DynamicWindParams0.xyz)))
	{
		return;
	}

	int3 coord = int3(dispatchThreadID.xyz);
	float newValue = 0.0;

	CalculateDiffusion(_VelocityBuffer0, _DynamicWindParams0.xyz, coord, _DynamicWindParams1.x, newValue);
	GroupMemoryBarrierWithGroupSync();
	AtomicAdd(_VelocityBuffer1, _DynamicWindParams0.xyz, coord, newValue);

	GroupMemoryBarrierWithGroupSync();

	CalculateDiffusion(_VelocityBuffer1, _DynamicWindParams0.xyz, coord, _DynamicWindParams1.x, newValue);
	GroupMemoryBarrierWithGroupSync();
	AtomicAdd(_VelocityBuffer0, _DynamicWindParams0.xyz, coord, newValue);

	GroupMemoryBarrierWithGroupSync();

	CalculateDiffusion(_VelocityBuffer0, _DynamicWindParams0.xyz, coord, _DynamicWindParams1.x, newValue);
	GroupMemoryBarrierWithGroupSync();
	AtomicAdd(_VelocityBuffer1, _DynamicWindParams0.xyz, coord, newValue);

	GroupMemoryBarrierWithGroupSync();

	CalculateDiffusion(_VelocityBuffer1, _DynamicWindParams0.xyz, coord, _DynamicWindParams1.x, newValue);
	GroupMemoryBarrierWithGroupSync();
	AtomicAdd(_VelocityBuffer0, _DynamicWindParams0.xyz, coord, newValue);

	GroupMemoryBarrierWithGroupSync();

	CalculateDiffusion(_VelocityBuffer0, _DynamicWindParams0.xyz, coord, _DynamicWindParams1.x, newValue);
	GroupMemoryBarrierWithGroupSync();
	AtomicAdd(_VelocityBuffer1, _DynamicWindParams0.xyz, coord, newValue);
}

#endif