//#ifndef _DYNAMIC_WIND_COMMON_HLSL
//#define _DYNAMIC_WIND_COMMON_HLSL
//
//#include "../../Common/ShaderCommon.hlsl"
//
//#define WIND_FLT_TO_INT			(1 << 16)
//
//#define GROUP_THREAD_SIZE_X		(4)
//#define GROUP_THREAD_SIZE_Y		(4)
//#define GROUP_THREAD_SIZE_Z		(4)
//
//inline int LoadRawBuffer(uniform RWTexture3D<int> rwTex, uniform int3 rwTexSize, in int3 coord)
//{
//	int result = 0;
//	if (all(coord >= 0) && all(coord < rwTexSize))
//	{
//		result = rwTex.Load(coord);
//	}
//	return result;
//}
//
//inline float LoadBuffer(uniform RWTexture3D<int> rwTex, uniform int3 rwTexSize, in int3 coord)
//{
//	float result = 0.0;
//	if (all(coord >= 0) && all(coord < rwTexSize))
//	{
//		result = ((float)(rwTex.Load(coord))) / WIND_FLT_TO_INT;
//	}
//	return result;
//}
//
//inline void AtomicAdd(uniform RWTexture3D<int> rwTex, uniform int3 rwTexSize, in int3 coord, in float value)
//{
//	if (all(coord < rwTexSize))
//	{
//		InterlockedAdd(rwTex[coord], (int)(value * WIND_FLT_TO_INT));
//	}
//}
//
//inline void CalculateDiffusion(uniform RWTexture3D<int> rwTex, uniform int3 rwTexSize, in int3 coord, in float a, out float value)
//{
//	value = LoadBuffer(rwTex, rwTexSize, coord + int3(-1, 0, 0));
//	value += LoadBuffer(rwTex, rwTexSize, coord + int3(1, 0, 0));
//	value += LoadBuffer(rwTex, rwTexSize, coord + int3(0, -1, 0));
//	value += LoadBuffer(rwTex, rwTexSize, coord + int3(0, 1, 0));
//	value += LoadBuffer(rwTex, rwTexSize, coord + int3(0, 0, -1));
//	value += LoadBuffer(rwTex, rwTexSize, coord + int3(0, 0, 1));
//	value *= a;
//	value += LoadBuffer(rwTex, rwTexSize, coord);
//	value /= 1.0 + 6.0 * a;
//}
//
//#endif