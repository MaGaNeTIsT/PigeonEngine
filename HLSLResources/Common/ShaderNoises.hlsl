#ifndef _SHADER_NOISES_HLSL
#define _SHADER_NOISES_HLSL

#define CUSTOM_SHADER_PI (3.141592653)

float random2(in float2 vec)
{
	return frac(sin(dot(vec.xy, float2(12.9898, 78.233))) * 43758.5453123);
}

float2 random22(in float2 vec)
{
	vec = float2(dot(vec, float2(127.1, 311.7)), dot(vec, float2(269.5, 183.3)));
	return frac(sin(vec) * 43758.5453123);
}

float voronoi2(in float2 vec)
{
	float2 ivec = floor(vec);
	float2 fvec = frac(vec);

	float value = 1.0;

	for (int y = -1; y <= 1; y++)
	{
		for (int x = -1; x <= 1; x++)
		{
			float2 offset = float2(x, y);

			float2 position;
			position = random22(ivec + offset);

			float dist = distance(position + offset, fvec);

			value = min(value, dist);
		}
	}
	return value;
}

float valueNoise2(in float2 vec)
{
	float2 ivec = floor(vec);
	float2 fvec = frac(vec);

	float a = random2(ivec + float2(0.0, 0.0));
	float b = random2(ivec + float2(1.0, 0.0));
	float c = random2(ivec + float2(0.0, 1.0));
	float d = random2(ivec + float2(1.0, 1.0));

	fvec = smoothstep(0.0, 1.0, fvec);

	return lerp(lerp(a, b, fvec.x), lerp(c, d, fvec.x), fvec.y);
}

float perlinNoise2(in float2 vec)
{
	float2 ivec = floor(vec);
	float2 fvec = frac(vec);

	float a = dot(random22(ivec + float2(0.0, 0.0)) * 2.0 - 1.0, fvec - float2(0.0, 0.0));
	float b = dot(random22(ivec + float2(1.0, 0.0)) * 2.0 - 1.0, fvec - float2(1.0, 0.0));
	float c = dot(random22(ivec + float2(0.0, 1.0)) * 2.0 - 1.0, fvec - float2(0.0, 1.0));
	float d = dot(random22(ivec + float2(1.0, 1.0)) * 2.0 - 1.0, fvec - float2(1.0, 1.0));

	fvec = smoothstep(0.0, 1.0, fvec);

	return lerp(lerp(a, b, fvec.x), lerp(c, d, fvec.x), fvec.y);
}

float fbm2(in float2 vec, int octave, float offset = 0.0)
{
	float value = 0.0;
	float amplitude = 1.0;
	//float frequency = 0.0;

	for (int i = 0; i < octave; i++)
	{
		value += amplitude * perlinNoise2(vec + offset);
		vec *= 2.0;
		amplitude *= 0.5;
	}

	return value;
}

float random3(in float3 vec)
{
	return frac(sin(dot(vec.xyz, float3(12.9898, 78.233, 47.2311))) * 43758.5453123);
}

float valueNoise3(in float3 vec)
{
	float3 ivec = floor(vec);
	float3 fvec = frac(vec);

	float a = random3(ivec + float3(0.0, 0.0, 0.0));
	float b = random3(ivec + float3(1.0, 0.0, 0.0));
	float c = random3(ivec + float3(0.0, 1.0, 0.0));
	float d = random3(ivec + float3(1.0, 1.0, 0.0));
	float e = random3(ivec + float3(0.0, 0.0, 1.0));
	float f = random3(ivec + float3(1.0, 0.0, 1.0));
	float g = random3(ivec + float3(0.0, 1.0, 1.0));
	float h = random3(ivec + float3(1.0, 1.0, 1.0));

	fvec = smoothstep(0.0, 1.0, fvec);
	float v1 = lerp(lerp(a, b, fvec.x), lerp(c, d, fvec.x), fvec.y);
	float v2 = lerp(lerp(e, f, fvec.x), lerp(g, h, fvec.x), fvec.y);
	float v3 = lerp(v1, v2, fvec.z);

	return v3;
}

float3 random33(in float3 vec)
{
	vec = float3(dot(vec, float3(127.1, 311.7, 245.4)), dot(vec, float3(269.5, 183.3, 131.2)), dot(vec, float3(522.3, 243.1, 532.4)));
	return frac(sin(vec) * 43758.5453123);
}

float perlinNoise3(in float3 vec)
{
	float3 ivec = floor(vec);
	float3 fvec = frac(vec);

	float a = dot(random3(ivec + float3(0.0, 0.0, 0.0)) * 2.0 - 1.0, fvec - float3(0.0, 0.0, 0.0));
	float b = dot(random3(ivec + float3(1.0, 0.0, 0.0)) * 2.0 - 1.0, fvec - float3(1.0, 0.0, 0.0));
	float c = dot(random3(ivec + float3(0.0, 1.0, 0.0)) * 2.0 - 1.0, fvec - float3(0.0, 1.0, 0.0));
	float d = dot(random3(ivec + float3(1.0, 1.0, 0.0)) * 2.0 - 1.0, fvec - float3(1.0, 1.0, 0.0));
	float e = dot(random3(ivec + float3(0.0, 0.0, 1.0)) * 2.0 - 1.0, fvec - float3(0.0, 0.0, 1.0));
	float f = dot(random3(ivec + float3(1.0, 0.0, 1.0)) * 2.0 - 1.0, fvec - float3(1.0, 0.0, 1.0));
	float g = dot(random3(ivec + float3(0.0, 1.0, 1.0)) * 2.0 - 1.0, fvec - float3(0.0, 1.0, 1.0));
	float h = dot(random3(ivec + float3(1.0, 1.0, 1.0)) * 2.0 - 1.0, fvec - float3(1.0, 1.0, 1.0));

	fvec = smoothstep(0.0, 1.0, fvec);
	float v1 = lerp(lerp(a, b, fvec.x), lerp(c, d, fvec.x), fvec.y);
	float v2 = lerp(lerp(e, f, fvec.x), lerp(g, h, fvec.x), fvec.y);
	float v3 = lerp(v1, v2, fvec.z);

	return v3;
}

float fbm3(in float3 vec, int octave, float offset = 0.0)
{
	float value = 0.0;
	float amplitude = 1.0;
	//float frequency = 0.0;

	for (int i = 0; i < octave; i++)
	{
		value += amplitude * perlinNoise3(vec + offset);
		vec *= 2.0;
		amplitude *= 0.5;
	}

	return value;
}

float stripes(float x, float f)
{
	float t = 0.5 + 0.5 * sin(f * 2 * CUSTOM_SHADER_PI * x);
	return t * t - 0.5;
}

float turbulence2(float2 xy, float f, float w)//w = Image width in pixels
{
	float t = -0.5;
	for (; f <= w / 12; f *= 2)
		t += abs(fbm2(xy, f) / f);
	return t;
}

float turbulence3(float3 xyz, float f, float w)//w = Image width in pixels
{
	float t = -0.5;
	for (; f <= w / 12; f *= 2)
		t += abs(fbm3(xyz, f) / f);
	return t;
}


// fbm(p + fbm(p + fbm(p)))


float noise(in float3 x)
{
	float3 p = floor(x);
	float3 f = frac(x);
	f = f * f * (3.0 - 2.0 * f);

	float2 uv = (p.xy + float2(37.0, 239.0) * p.z) + f.xy;

	float2 rg = random22(uv);

	return -1.0 + 2.0 * lerp(rg.x, rg.y, f.z);
}

float cloud5(in float3 p, float time = 0.0)
{
	return clamp(fbm3(p, 5, time), 0.0, 1.0);
}
float cloud4(in float3 p, float time = 0.0)
{
	return clamp(fbm3(p, 4, time), 0.0, 1.0);
}
float cloud3(in float3 p, float time = 0.0)
{
	return clamp(fbm3(p, 3, time), 0.0, 1.0);
}
float cloud2(in float3 p, float time = 0.0)
{
	return clamp(fbm3(p, 2, time), 0.0, 1.0);
}

#endif