#ifndef _SHADER_SELECT_FUNCTIONS_HLSL
#define _SHADER_SELECT_FUNCTIONS_HLSL

float	SelectInternal(bool		InCon, float	A, float	B) { return float(InCon ? A.x : B.x); }
float2	SelectInternal(bool		InCon, float	A, float2	B) { return float2(InCon ? A : B.x, InCon ? A : B.y); }
float2	SelectInternal(bool		InCon, float2	A, float	B) { return float2(InCon ? A.x : B, InCon ? A.y : B); }
float2	SelectInternal(bool		InCon, float2	A, float2	B) { return float2(InCon ? A.x : B.x, InCon ? A.y : B.y); }
float2	SelectInternal(bool2	InCon, float	A, float	B) { return float2(InCon.x ? A : B, InCon.y ? A : B); }
float2	SelectInternal(bool2	InCon, float	A, float2	B) { return float2(InCon.x ? A : B.x, InCon.y ? A : B.y); }
float2	SelectInternal(bool2	InCon, float2	A, float	B) { return float2(InCon.x ? A.x : B, InCon.y ? A.y : B); }
float2	SelectInternal(bool2	InCon, float2	A, float2	B) { return float2(InCon.x ? A.x : B.x, InCon.y ? A.y : B.y); }
float3	SelectInternal(bool		InCon, float	A, float3	B) { return float3(InCon ? A : B.x, InCon ? A : B.y, InCon ? A : B.z); }
float3	SelectInternal(bool		InCon, float3	A, float	B) { return float3(InCon ? A.x : B, InCon ? A.y : B, InCon ? A.z : B); }
float3	SelectInternal(bool		InCon, float3	A, float3	B) { return float3(InCon ? A.x : B.x, InCon ? A.y : B.y, InCon ? A.z : B.z); }
float3	SelectInternal(bool3	InCon, float	A, float	B) { return float3(InCon.x ? A : B, InCon.y ? A : B, InCon.z ? A : B); }
float3	SelectInternal(bool3	InCon, float	A, float3	B) { return float3(InCon.x ? A : B.x, InCon.y ? A : B.y, InCon.z ? A : B.z); }
float3	SelectInternal(bool3	InCon, float3	A, float	B) { return float3(InCon.x ? A.x : B, InCon.y ? A.y : B, InCon.z ? A.z : B); }
float3	SelectInternal(bool3	InCon, float3	A, float3	B) { return float3(InCon.x ? A.x : B.x, InCon.y ? A.y : B.y, InCon.z ? A.z : B.z); }
float4	SelectInternal(bool		InCon, float	A, float4	B) { return float4(InCon ? A : B.x, InCon ? A : B.y, InCon ? A : B.z, InCon ? A : B.w); }
float4	SelectInternal(bool		InCon, float4	A, float	B) { return float4(InCon ? A.x : B, InCon ? A.y : B, InCon ? A.z : B, InCon ? A.w : B); }
float4	SelectInternal(bool		InCon, float4	A, float4	B) { return float4(InCon ? A.x : B.x, InCon ? A.y : B.y, InCon ? A.z : B.z, InCon ? A.w : B.w); }
float4	SelectInternal(bool4	InCon, float	A, float	B) { return float4(InCon.x ? A : B, InCon.y ? A : B, InCon.z ? A : B, InCon.w ? A : B); }
float4	SelectInternal(bool4	InCon, float	A, float4	B) { return float4(InCon.x ? A : B.x, InCon.y ? A : B.y, InCon.z ? A : B.z, InCon.w ? A : B.w); }
float4	SelectInternal(bool4	InCon, float4	A, float	B) { return float4(InCon.x ? A.x : B, InCon.y ? A.y : B, InCon.z ? A.z : B, InCon.w ? A.w : B); }
float4	SelectInternal(bool4	InCon, float4	A, float4	B) { return float4(InCon.x ? A.x : B.x, InCon.y ? A.y : B.y, InCon.z ? A.z : B.z, InCon.w ? A.w : B.w); }

int		SelectInternal(bool		InCon, int	A, int	B) { return int(InCon ? A.x : B.x); }
int2	SelectInternal(bool		InCon, int	A, int2	B) { return int2(InCon ? A : B.x, InCon ? A : B.y); }
int2	SelectInternal(bool		InCon, int2	A, int	B) { return int2(InCon ? A.x : B, InCon ? A.y : B); }
int2	SelectInternal(bool		InCon, int2	A, int2	B) { return int2(InCon ? A.x : B.x, InCon ? A.y : B.y); }
int2	SelectInternal(bool2	InCon, int	A, int	B) { return int2(InCon.x ? A : B, InCon.y ? A : B); }
int2	SelectInternal(bool2	InCon, int	A, int2	B) { return int2(InCon.x ? A : B.x, InCon.y ? A : B.y); }
int2	SelectInternal(bool2	InCon, int2	A, int	B) { return int2(InCon.x ? A.x : B, InCon.y ? A.y : B); }
int2	SelectInternal(bool2	InCon, int2	A, int2	B) { return int2(InCon.x ? A.x : B.x, InCon.y ? A.y : B.y); }
int3	SelectInternal(bool		InCon, int	A, int3	B) { return int3(InCon ? A : B.x, InCon ? A : B.y, InCon ? A : B.z); }
int3	SelectInternal(bool		InCon, int3	A, int	B) { return int3(InCon ? A.x : B, InCon ? A.y : B, InCon ? A.z : B); }
int3	SelectInternal(bool		InCon, int3	A, int3	B) { return int3(InCon ? A.x : B.x, InCon ? A.y : B.y, InCon ? A.z : B.z); }
int3	SelectInternal(bool3	InCon, int	A, int	B) { return int3(InCon.x ? A : B, InCon.y ? A : B, InCon.z ? A : B); }
int3	SelectInternal(bool3	InCon, int	A, int3	B) { return int3(InCon.x ? A : B.x, InCon.y ? A : B.y, InCon.z ? A : B.z); }
int3	SelectInternal(bool3	InCon, int3	A, int	B) { return int3(InCon.x ? A.x : B, InCon.y ? A.y : B, InCon.z ? A.z : B); }
int3	SelectInternal(bool3	InCon, int3	A, int3	B) { return int3(InCon.x ? A.x : B.x, InCon.y ? A.y : B.y, InCon.z ? A.z : B.z); }
int4	SelectInternal(bool		InCon, int	A, int4	B) { return int4(InCon ? A : B.x, InCon ? A : B.y, InCon ? A : B.z, InCon ? A : B.w); }
int4	SelectInternal(bool		InCon, int4	A, int	B) { return int4(InCon ? A.x : B, InCon ? A.y : B, InCon ? A.z : B, InCon ? A.w : B); }
int4	SelectInternal(bool		InCon, int4	A, int4	B) { return int4(InCon ? A.x : B.x, InCon ? A.y : B.y, InCon ? A.z : B.z, InCon ? A.w : B.w); }
int4	SelectInternal(bool4	InCon, int	A, int	B) { return int4(InCon.x ? A : B, InCon.y ? A : B, InCon.z ? A : B, InCon.w ? A : B); }
int4	SelectInternal(bool4	InCon, int	A, int4	B) { return int4(InCon.x ? A : B.x, InCon.y ? A : B.y, InCon.z ? A : B.z, InCon.w ? A : B.w); }
int4	SelectInternal(bool4	InCon, int4	A, int	B) { return int4(InCon.x ? A.x : B, InCon.y ? A.y : B, InCon.z ? A.z : B, InCon.w ? A.w : B); }
int4	SelectInternal(bool4	InCon, int4	A, int4	B) { return int4(InCon.x ? A.x : B.x, InCon.y ? A.y : B.y, InCon.z ? A.z : B.z, InCon.w ? A.w : B.w); }

uint	SelectInternal(bool		InCon, uint		A, uint		B) { return uint(InCon ? A.x : B.x); }
uint2	SelectInternal(bool		InCon, uint		A, uint2	B) { return uint2(InCon ? A : B.x, InCon ? A : B.y); }
uint2	SelectInternal(bool		InCon, uint2	A, uint		B) { return uint2(InCon ? A.x : B, InCon ? A.y : B); }
uint2	SelectInternal(bool		InCon, uint2	A, uint2	B) { return uint2(InCon ? A.x : B.x, InCon ? A.y : B.y); }
uint2	SelectInternal(bool2	InCon, uint		A, uint		B) { return uint2(InCon.x ? A : B, InCon.y ? A : B); }
uint2	SelectInternal(bool2	InCon, uint		A, uint2	B) { return uint2(InCon.x ? A : B.x, InCon.y ? A : B.y); }
uint2	SelectInternal(bool2	InCon, uint2	A, uint		B) { return uint2(InCon.x ? A.x : B, InCon.y ? A.y : B); }
uint2	SelectInternal(bool2	InCon, uint2	A, uint2	B) { return uint2(InCon.x ? A.x : B.x, InCon.y ? A.y : B.y); }
uint3	SelectInternal(bool		InCon, uint		A, uint3	B) { return uint3(InCon ? A : B.x, InCon ? A : B.y, InCon ? A : B.z); }
uint3	SelectInternal(bool		InCon, uint3	A, uint		B) { return uint3(InCon ? A.x : B, InCon ? A.y : B, InCon ? A.z : B); }
uint3	SelectInternal(bool		InCon, uint3	A, uint3	B) { return uint3(InCon ? A.x : B.x, InCon ? A.y : B.y, InCon ? A.z : B.z); }
uint3	SelectInternal(bool3	InCon, uint		A, uint		B) { return uint3(InCon.x ? A : B, InCon.y ? A : B, InCon.z ? A : B); }
uint3	SelectInternal(bool3	InCon, uint		A, uint3	B) { return uint3(InCon.x ? A : B.x, InCon.y ? A : B.y, InCon.z ? A : B.z); }
uint3	SelectInternal(bool3	InCon, uint3	A, uint		B) { return uint3(InCon.x ? A.x : B, InCon.y ? A.y : B, InCon.z ? A.z : B); }
uint3	SelectInternal(bool3	InCon, uint3	A, uint3	B) { return uint3(InCon.x ? A.x : B.x, InCon.y ? A.y : B.y, InCon.z ? A.z : B.z); }
uint4	SelectInternal(bool		InCon, uint		A, uint4	B) { return uint4(InCon ? A : B.x, InCon ? A : B.y, InCon ? A : B.z, InCon ? A : B.w); }
uint4	SelectInternal(bool		InCon, uint4	A, uint		B) { return uint4(InCon ? A.x : B, InCon ? A.y : B, InCon ? A.z : B, InCon ? A.w : B); }
uint4	SelectInternal(bool		InCon, uint4	A, uint4	B) { return uint4(InCon ? A.x : B.x, InCon ? A.y : B.y, InCon ? A.z : B.z, InCon ? A.w : B.w); }
uint4	SelectInternal(bool4	InCon, uint		A, uint		B) { return uint4(InCon.x ? A : B, InCon.y ? A : B, InCon.z ? A : B, InCon.w ? A : B); }
uint4	SelectInternal(bool4	InCon, uint		A, uint4	B) { return uint4(InCon.x ? A : B.x, InCon.y ? A : B.y, InCon.z ? A : B.z, InCon.w ? A : B.w); }
uint4	SelectInternal(bool4	InCon, uint4	A, uint		B) { return uint4(InCon.x ? A.x : B, InCon.y ? A.y : B, InCon.z ? A.z : B, InCon.w ? A.w : B); }
uint4	SelectInternal(bool4	InCon, uint4	A, uint4	B) { return uint4(InCon.x ? A.x : B.x, InCon.y ? A.y : B.y, InCon.z ? A.z : B.z, InCon.w ? A.w : B.w); }

#endif	//_SHADER_SELECT_FUNCTIONS_HLSL