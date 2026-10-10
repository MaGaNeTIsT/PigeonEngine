#ifndef SHADER_LIGHT_DATA_HLSL
#define SHADER_LIGHT_DATA_HLSL

// ---------------------------------------------------------------------------
// Light data: the layout the renderer uploads, the readable form the shading
// code uses, and the packet the BRDF consumes.
//
// ---------------------------------------------------------------------------
// Storage: every member is one 16 byte slot
//
// The element layouts are declared in Components/C_Lighting.json, not here. That
// is deliberate twice over:
//
//   1. A structured buffer's element layout is part of the binding contract, so
//      it belongs next to the binding.
//   2. The member types are all float4 or uint4, so every field occupies exactly
//      one 16 byte slot and the C++ side can size its own mirror from the
//      reflected field count alone. A float3 followed by a float would also fill
//      one register today, but the padding rule that makes it work is a detail of
//      one compiler's packing; a slot per member is not.
//
// The compiler generates the storage types from those Struct blocks and the
// generated names carry a _Struct suffix, so the aliases below are what names
// them, and Decode turns one into the readable form the shading code wants. The
// point of the split is that only Decode knows which component of which slot
// means what - everything downstream reads named fields.
//
// ---------------------------------------------------------------------------
// The shading packet is a different thing again, and it lives here
//
// A shading model should not have to know how the light list is stored. A domain
// loops over the counts (DirectionalLightCount and LocalLightCount, both in
// GlobalParams), builds a LightData per entry, and hands that to the BRDF.
//
// ---------------------------------------------------------------------------
// Units
//
// The whole lighting pipeline is photometric, which is what keeps a luminous
// efficacy constant out of it:
//
//   DirectionalLights   IlluminanceLux              [lux]      perpendicular irradiance
//   LocalLights         LuminousIntensityCandela    [candela]  luminous intensity
//
// A local light's I / d^2 is [cd/m2], which is [lux], so both light types arrive
// at the same quantity and LightData.Illuminance is illuminance for both. The
// BRDF then returns luminance: albedo/pi * E and f_spec * E are both [cd/m2],
// which is what the scene colour target holds and what exposure maps to display.
// ---------------------------------------------------------------------------

#include "Public/Core/ShaderConstants.hlsl"
#include "Public/Core/ShaderMath.hlsl"

// The light channel mask bits are defined in exactly one place, and that place is
// the GBuffer IO header, because the mask is stored in GBufferD. Lighting
// depending on that header is the price of having a single definition point.
#include "Public/GBuffer/ShaderGBufferIO.hlsl"

// Must match ELightType in the engine's Core/EngineCommon.h. The value travels in
// SpotAndTypeParams.x, so the two sides agreeing is a contract rather than a
// convenience.
#define LIGHT_TYPE_DIRECTIONAL	(0u)
#define LIGHT_TYPE_POINT		(1u)
#define LIGHT_TYPE_SPOT			(2u)

typedef DirectionalLights_Struct	DirectionalLightsStorage;
typedef LocalLights_Struct			LocalLightsStorage;

// ---------------------------------------------------------------------------
// Readable form
// ---------------------------------------------------------------------------

struct DirectionalLightData
{
	float3	WorldDirection;			// the direction the light travels, unit length
	float	IlluminanceLux;			// perpendicular illuminance [lux]
	float3	Color;					// normalized chromaticity
	float	SourceAngleDegrees;		// angular diameter of the disc, 0 = a delta light
	uint	ShadowMapIndex;
	uint	CascadeCount;
};

struct LocalLightData
{
	float3	WorldPosition;
	float	AttenuationRadiusMeters;	// the cutoff that bounds the shading cost [m]
	float3	Color;						// normalized chromaticity
	float	LuminousIntensityCandela;	// [candela]
	float3	WorldDirection;				// the spot axis, unit length
	float	SourceRadiusMeters;			// emitter radius, 0 = an ideal point [m]
	uint	LightType;					// LIGHT_TYPE_POINT or LIGHT_TYPE_SPOT
	float	SpotCosInnerAngle;
	float	SpotCosOuterAngle;
	uint	ShadowMapIndex;
};

// Storage to readable. The only place that knows how a slot is split.
DirectionalLightData DecodeDirectionalLight(in DirectionalLightsStorage InStored)
{
	DirectionalLightData Result;
	Result.WorldDirection		= InStored.DirectionAndIlluminanceLux.xyz;
	Result.IlluminanceLux		= InStored.DirectionAndIlluminanceLux.w;
	Result.Color				= InStored.ColorAndSourceAngleDegrees.xyz;
	Result.SourceAngleDegrees	= InStored.ColorAndSourceAngleDegrees.w;
	Result.ShadowMapIndex		= InStored.ShadowAddressing.x;
	Result.CascadeCount			= InStored.ShadowAddressing.y;
	return Result;
}

LocalLightData DecodeLocalLight(in LocalLightsStorage InStored)
{
	LocalLightData Result;
	Result.WorldPosition				= InStored.WorldPositionAndAttenuationRadiusMeters.xyz;
	Result.AttenuationRadiusMeters		= InStored.WorldPositionAndAttenuationRadiusMeters.w;
	Result.Color						= InStored.ColorAndLuminousIntensityCandela.xyz;
	Result.LuminousIntensityCandela		= InStored.ColorAndLuminousIntensityCandela.w;
	Result.WorldDirection				= InStored.WorldDirectionAndSourceRadiusMeters.xyz;
	Result.SourceRadiusMeters			= InStored.WorldDirectionAndSourceRadiusMeters.w;
	Result.LightType					= InStored.SpotAndTypeParams.x;
	Result.SpotCosInnerAngle			= asfloat(InStored.SpotAndTypeParams.y);
	Result.SpotCosOuterAngle			= asfloat(InStored.SpotAndTypeParams.z);
	Result.ShadowMapIndex				= InStored.SpotAndTypeParams.w;
	return Result;
}

// Readable to storage. The shading path never needs this - it only decodes - but a
// producer that runs on the gpu does, and keeping it here means the split is
// described once rather than once per direction.
DirectionalLightsStorage EncodeDirectionalLight(in DirectionalLightData InLight)
{
	DirectionalLightsStorage Result;
	Result.DirectionAndIlluminanceLux		= float4(InLight.WorldDirection, InLight.IlluminanceLux);
	Result.ColorAndSourceAngleDegrees		= float4(InLight.Color, InLight.SourceAngleDegrees);
	Result.ShadowAddressing					= uint4(InLight.ShadowMapIndex, InLight.CascadeCount, 0u, 0u);
	return Result;
}

LocalLightsStorage EncodeLocalLight(in LocalLightData InLight)
{
	LocalLightsStorage Result;
	Result.WorldPositionAndAttenuationRadiusMeters	= float4(InLight.WorldPosition, InLight.AttenuationRadiusMeters);
	Result.ColorAndLuminousIntensityCandela			= float4(InLight.Color, InLight.LuminousIntensityCandela);
	Result.WorldDirectionAndSourceRadiusMeters		= float4(InLight.WorldDirection, InLight.SourceRadiusMeters);
	Result.SpotAndTypeParams						= uint4(InLight.LightType
		, asuint(InLight.SpotCosInnerAngle), asuint(InLight.SpotCosOuterAngle), InLight.ShadowMapIndex);
	return Result;
}

// ---------------------------------------------------------------------------
// Shading side
// ---------------------------------------------------------------------------

struct LightData
{
	float3	Direction;				// surface towards the light, unit length
	float3	Illuminance;			// illuminance at the surface [lux]
	float	Shadow;					// 1 = fully lit, 0 = fully shadowed
	float	SpecularAlphaFloor;		// the light's own angular size, as a GGX alpha
	uint	ChannelMask;			// which bit of the light channel mask this light belongs to
};

// The GGX alpha below which the specular lobe would be sharper than the light that
// made it. A mirror reflecting the sun returns a disc and not a point, and the lobe
// cannot be narrower than that disc: tan of the light's angular radius is exactly
// the alpha whose lobe width matches it.
//
// This also keeps the distribution finite. The GGX peak is inversely proportional
// to alpha squared, so an alpha of zero is an infinite highlight in the exact mirror
// direction - which a mirror-and-sun pair reaches, and which is a NaN rather than a
// bright pixel once it meets a zero weight. A light with no angular size gets
// MIN_ROUGHNESS instead, the same floor the material path uses for the sharpest
// roughness it allows.
float ComputeLightSpecularAlphaFloor(in float InSourceAngleDegrees)
{
	const float HalfAngleRadians = 0.5 * radians(max(InSourceAngleDegrees, 0.0));
	return max(tan(HalfAngleRadians), MIN_ROUGHNESS);
}

// The same floor for a light with a physical radius, seen from a distance. asin is
// the honest angular radius of a sphere; saturating the ratio keeps a surface inside
// the emitter from producing a NaN.
float ComputeSphereLightSpecularAlphaFloor(in float InSourceRadiusMeters, in float InDistance)
{
	if (InSourceRadiusMeters <= 0.0)
	{
		return MIN_ROUGHNESS;
	}
	const float AngularRadius = asin(saturate(InSourceRadiusMeters / max(InDistance, InSourceRadiusMeters)));
	return max(tan(AngularRadius), MIN_ROUGHNESS);
}

// Builds the shading packet for a directional light. The stored direction is the
// direction the light travels, so the surface towards the light vector is its
// negation.
LightData BuildDirectionalLightData(in DirectionalLightData InStored, in float InShadow)
{
	LightData Result;
	Result.Direction = -InStored.WorldDirection;
	Result.Illuminance = InStored.Color * InStored.IlluminanceLux;
	Result.Shadow = InShadow;
	Result.SpecularAlphaFloor = ComputeLightSpecularAlphaFloor(InStored.SourceAngleDegrees);
	Result.ChannelMask = LIGHT_CHANNEL_DIRECT;
	return Result;
}

// Builds the shading packet for a local light, including attenuation at the
// surface point. The result is zero outside the light's reach, which lets the
// caller skip the BRDF rather than shade a zero contribution.
LightData BuildLocalLightData(in LocalLightData InStored, in float3 InWorldPosition, in float InShadow)
{
	LightData Result;

	float3 ToLight = InStored.WorldPosition - InWorldPosition;
	float DistanceSqr = max(dot(ToLight, ToLight), 1e-8);
	float Distance = sqrt(DistanceSqr);
	Result.Direction = ToLight / Distance;

	// Inverse square falloff, windowed by the reach so the light arrives at
	// exactly zero at its radius instead of stepping the shading rate there.
	float RadiusSqr = max(InStored.AttenuationRadiusMeters * InStored.AttenuationRadiusMeters, 1e-6);
	float Window = saturate(1.0 - (DistanceSqr / RadiusSqr));
	Window = Window * Window;

	// I / d^2 is [cd/m2], which is [lux]: the falloff is what turns an intensity into
	// an illuminance, and it is why the two light types reach the BRDF in one unit.
	float Attenuation = Window / DistanceSqr;

	// Spot cone: one inside the inner cone, falling to zero at the outer one, and
	// constant for a point light, which has no cone to speak of. The branch is on the
	// stored type rather than on a sentinel cosine, so a spot whose outer angle
	// happens to be anything at all still reads as a spot.
	float SpotWindow = 1.0;
	if (InStored.LightType == LIGHT_TYPE_SPOT)
	{
		float SpotCos = dot(-Result.Direction, InStored.WorldDirection);
		float Inner = max(InStored.SpotCosInnerAngle, InStored.SpotCosOuterAngle);
		SpotWindow = saturate((SpotCos - InStored.SpotCosOuterAngle) / max(Inner - InStored.SpotCosOuterAngle, 1e-4));
	}

	Result.Illuminance = InStored.Color * InStored.LuminousIntensityCandela * Attenuation * SpotWindow;
	Result.Shadow = InShadow;
	Result.SpecularAlphaFloor = ComputeSphereLightSpecularAlphaFloor(InStored.SourceRadiusMeters, Distance);
	Result.ChannelMask = LIGHT_CHANNEL_LOCAL;
	return Result;
}
#endif	//SHADER_LIGHT_DATA_HLSL
