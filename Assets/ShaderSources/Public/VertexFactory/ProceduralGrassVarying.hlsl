#ifndef PROCEDURAL_GRASS_VARYING_HLSL
#define PROCEDURAL_GRASS_VARYING_HLSL

// ---------------------------------------------------------------------------
// Interpolants of the procedural grass vertex factory.
//
// A fragment, not a file: this is #included INSIDE the body of struct VFVarying,
// which the vertex factory component assembles in injected text. It carries
// members and nothing else, for the reason FullScreenVarying.hlsl documents.
//
// The member set is the system member set the programmable layer reads back:
// BuildMaterialPixelInput in Surfaces/ProgrammableShaderTemplate.hlsl names
// WorldPosition, WorldNormal, WorldTangent, WorldBitangent, UV0, UV1, Color and
// Position, so every factory's varying has to declare all of them, whether or
// not the blade has a meaningful value for each. The grass has no vertex colour
// and no second UV channel, so those two are filled with the documented constants
// (white and zero) rather than dropped.
//
// The blade travels in UV0, and that is the one member whose meaning is
// factory specific: x is the side coordinate the blade curve was evaluated at,
// y is the position along the blade from root to tip. The material reads it back
// in the pixel stage to blend its root and tip colours, which is exactly what
// the legacy material did with the same channel.
//
// The world members are filled by VFTransformVertex, which runs after the
// material's vertex entry point, so a material cannot change them.
// ---------------------------------------------------------------------------

	float4	Position		: SV_Position;
	float3	WorldPosition	: TEXCOORD0;
	float3	WorldNormal		: TEXCOORD1;
	float3	WorldTangent	: TEXCOORD2;
	float3	WorldBitangent	: TEXCOORD3;
	float2	UV0				: TEXCOORD4;
	float2	UV1				: TEXCOORD5;
	float4	Color			: TEXCOORD6;
#endif	//PROCEDURAL_GRASS_VARYING_HLSL
