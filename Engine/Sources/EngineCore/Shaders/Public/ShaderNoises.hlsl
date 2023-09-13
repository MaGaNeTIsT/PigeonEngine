#ifndef _SHADER_NOISES_HLSL
#define _SHADER_NOISES_HLSL

float SimpleRandom2(in float2 InValue2)
{
	return frac(sin(dot(InValue2.xy, float2(12.9898, 78.233))) * 43758.5453123);
}

float2 SimpleRandom22(in float2 InValue2)
{
	InValue2 = float2(dot(InValue2, float2(127.1, 311.7)), dot(InValue2, float2(269.5, 183.3)));
	return frac(sin(InValue2) * 43758.5453123);
}

float Voronoi2(in float2 InValue2)
{
	float2 i = floor(InValue2);
	float2 f = frac(InValue2);

	float Result = 1.0;

	[unroll]
	for (int y = -1; y <= 1; y++)
	{
		[unroll]
		for (int x = -1; x <= 1; x++)
		{
			float2 Offset = float2(x, y);

			float2 Position;
			Position = SimpleRandom22(i + Offset);

			float Dist = distance(Position + Offset, f);

			Result = min(Result, Dist);
		}
	}
	return Result;
}

float ValueNoise2(in float2 InValue2)
{
	float2 i = floor(InValue2);
	float2 f = frac(InValue2);

	float a = SimpleRandom2(i + float2(0.0, 0.0));
	float b = SimpleRandom2(i + float2(1.0, 0.0));
	float c = SimpleRandom2(i + float2(0.0, 1.0));
	float d = SimpleRandom2(i + float2(1.0, 1.0));

	f = smoothstep(0.0, 1.0, f);

	return lerp(lerp(a, b, f.x), lerp(c, d, f.x), f.y);
}

float PerlinNoise2(in float2 InValue2)
{
	float2 i = floor(InValue2);
	float2 f = frac(InValue2);

	float a = dot(SimpleRandom22(i + float2(0.0, 0.0)) * 2.0 - 1.0, f - float2(0.0, 0.0));
	float b = dot(SimpleRandom22(i + float2(1.0, 0.0)) * 2.0 - 1.0, f - float2(1.0, 0.0));
	float c = dot(SimpleRandom22(i + float2(0.0, 1.0)) * 2.0 - 1.0, f - float2(0.0, 1.0));
	float d = dot(SimpleRandom22(i + float2(1.0, 1.0)) * 2.0 - 1.0, f - float2(1.0, 1.0));

	f = smoothstep(0.0, 1.0, f);

	return lerp(lerp(a, b, f.x), lerp(c, d, f.x), f.y);
}

float FractionalBrownianMotion_PerlinNoise2(in float2 InValue2, float InOffset, uniform uint InOctave)
{
	const float Frequency = 2.0;
	float Result = 0.0;
	float Amplitude = 1.0;

	[unroll]
	for (uint i = 0; i < InOctave; i++)
	{
		Result += Amplitude * PerlinNoise2(InValue2 + InOffset);
		InValue2 *= Frequency;
		Amplitude *= 0.5;
	}

	return Result;
}

float SimpleRandom3(in float3 InValue3)
{
	return frac(sin(dot(InValue3.xyz, float3(12.9898, 78.233, 47.2311))) * 43758.5453123);
}

float3 SimpleRandom33(in float3 InValue3)
{
	InValue3 = float3(dot(InValue3, float3(127.1, 311.7, 245.4)), dot(InValue3, float3(269.5, 183.3, 131.2)), dot(InValue3, float3(522.3, 243.1, 532.4)));
	return frac(sin(InValue3) * 43758.5453123);
}

float ValueNoise3(in float3 InValue3)
{
	float3 i = floor(InValue3);
	float3 f = frac(InValue3);

	float a = SimpleRandom3(i + float3(0.0, 0.0, 0.0));
	float b = SimpleRandom3(i + float3(1.0, 0.0, 0.0));
	float c = SimpleRandom3(i + float3(0.0, 1.0, 0.0));
	float d = SimpleRandom3(i + float3(1.0, 1.0, 0.0));
	float e = SimpleRandom3(i + float3(0.0, 0.0, 1.0));
	float f = SimpleRandom3(i + float3(1.0, 0.0, 1.0));
	float g = SimpleRandom3(i + float3(0.0, 1.0, 1.0));
	float h = SimpleRandom3(i + float3(1.0, 1.0, 1.0));

	f = smoothstep(0.0, 1.0, f);
	float v1 = lerp(lerp(a, b, f.x), lerp(c, d, f.x), f.y);
	float v2 = lerp(lerp(e, f, f.x), lerp(g, h, f.x), f.y);
	float v3 = lerp(v1, v2, f.z);

	return v3;
}

float PerlinNoise3(in float3 InValue3)
{
	float3 i = floor(InValue3);
	float3 f = frac(InValue3);

	float a = dot(SimpleRandom3(i + float3(0.0, 0.0, 0.0)) * 2.0 - 1.0, f - float3(0.0, 0.0, 0.0));
	float b = dot(SimpleRandom3(i + float3(1.0, 0.0, 0.0)) * 2.0 - 1.0, f - float3(1.0, 0.0, 0.0));
	float c = dot(SimpleRandom3(i + float3(0.0, 1.0, 0.0)) * 2.0 - 1.0, f - float3(0.0, 1.0, 0.0));
	float d = dot(SimpleRandom3(i + float3(1.0, 1.0, 0.0)) * 2.0 - 1.0, f - float3(1.0, 1.0, 0.0));
	float e = dot(SimpleRandom3(i + float3(0.0, 0.0, 1.0)) * 2.0 - 1.0, f - float3(0.0, 0.0, 1.0));
	float f = dot(SimpleRandom3(i + float3(1.0, 0.0, 1.0)) * 2.0 - 1.0, f - float3(1.0, 0.0, 1.0));
	float g = dot(SimpleRandom3(i + float3(0.0, 1.0, 1.0)) * 2.0 - 1.0, f - float3(0.0, 1.0, 1.0));
	float h = dot(SimpleRandom3(i + float3(1.0, 1.0, 1.0)) * 2.0 - 1.0, f - float3(1.0, 1.0, 1.0));

	f = smoothstep(0.0, 1.0, f);
	float v1 = lerp(lerp(a, b, f.x), lerp(c, d, f.x), f.y);
	float v2 = lerp(lerp(e, f, f.x), lerp(g, h, f.x), f.y);
	float v3 = lerp(v1, v2, f.z);

	return v3;
}

float FractionalBrownianMotion_PerlinNoise3(in float3 InValue3, float InOffset, uniform uint InOctave)
{
	const float Frequency = 2.0;
	float Result = 0.0;
	float Amplitude = 1.0;

	[unroll]
	for (uint i = 0u; i < InOctave; i++)
	{
		Result += Amplitude * PerlinNoise3(InValue3 + InOffset);
		InValue3 *= Frequency;
		Amplitude *= 0.5;
	}

	return Result;
}

float StripesNoise(float x, float f)
{
	float Result = 0.5 + 0.5 * sin(f * x * (2.0 * (CUSTOM_SHADER_PI)));
	return (Result * Result - 0.5);
}

float TurbulenceNoise2(float2 InValue2, float InStartValue, float InTargetWidth, float InSourceWidth)
{
	const float TestValue = InSourceWidth / InTargetWidth;

	float Result = -0.5;

	[loop]
	for (; InStartValue <= TestValue; InStartValue *= 2.0)
	{
		Result += abs(FractionalBrownianMotion_PerlinNoise2(InValue2, 0.0, (uint)InStartValue) / InStartValue);
	}

	return Result;
}

float TurbulenceNoise3(float3 InValue3, float InStartValue, float InTargetWidth, float InSourceWidth)
{
	const float TestValue = InSourceWidth / InTargetWidth;

	float Result = -0.5;

	[loop]
	for (; InStartValue <= TestValue; InStartValue *= 2.0)
	{
		Result += abs(FractionalBrownianMotion_PerlinNoise3(InValue3, 0.0, (uint)InStartValue) / InStartValue);
	}

	return Result;
}


// fbm(p + fbm(p + fbm(p)))


float UnnamedNoise3(in float3 InValue3)
{
	float3 i = floor(InValue3);
	float3 f = frac(InValue3);

	f = f * f * (3.0 - 2.0 * f);

	float2 uv = (i.xy + float2(37.0, 239.0) * i.z) + f.xy;

	float2 r = SimpleRandom22(uv);

	return (-1.0 + 2.0 * lerp(r.x, r.y, f.z));
}

float InterleavedGradientNoise(float2 InValue2)
{
	return frac(52.9829189 * frac((InValue2.x * 0.06711056) + (InValue2.y * 0.00583715)));
}

float3 GetRandomVectorFromCoord(int2 InCoord)
{
	InCoord.y = 16384 - InCoord.y;

	float3 RandomVec = 0.0, RandomTexVec = 0.0;
	float ScaleOffset = 0.0;

	const float TemporalCos = 0.8660253882f;
	const float TemporalSin = 0.5;

	float GradientNoise = InterleavedGradientNoise((float2)InCoord);

	RandomTexVec.x = cos((GradientNoise * (CUSTOM_SHADER_PI)));
	RandomTexVec.y = sin((GradientNoise * (CUSTOM_SHADER_PI)));

	ScaleOffset = (1.0 / 4.0) * ((InCoord.y - InCoord.x) & 3);
	//ScaleOffset = (1.0 / 5.0) * ((InCoord.y - InCoord.x) % 5);

	RandomVec.x = dot(RandomTexVec.xy, float2(TemporalCos, -TemporalSin));
	RandomVec.y = dot(RandomTexVec.xy, float2(TemporalSin, TemporalCos));
	RandomVec.z = frac(ScaleOffset + 0.025);

	return RandomVec;
}

#endif