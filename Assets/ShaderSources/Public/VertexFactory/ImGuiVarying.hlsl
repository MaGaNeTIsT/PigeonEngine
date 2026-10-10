#ifndef IMGUI_VARYING_HLSL
#define IMGUI_VARYING_HLSL

// ---------------------------------------------------------------------------
// Interpolants of the ImGui vertex factory.
//
// A fragment, not a file: this is #included INSIDE the body of
// struct VFVarying, which the vertex factory component assembles in injected
// text. That is deliberate and it is forced by the injector: a #include'd file
// is not scanned for placeholders, so the material's own member block has to be
// spliced in at a place that is scanned text, which means the struct braces have
// to be part of the injected text as well.
//
// So this file carries members and nothing else. No struct, no pragma once, no
// includes of its own. It is included exactly once per compilation.
//
// The member set is identical to the other factories' and that is not a
// coincidence: the pixel stage reads these members by name through
// BuildMaterialPixelInput, so every factory owes the pixel stage the same
// members and only differs in how it fills them. The user varying block is
// appended after this one, which is why the texture coordinate indices below are
// the system's and a material's own members start after them.
//
// Semantics: SV_Position is the clip space position. The world space members
// carry the interpolated world position and the tangent frame that the pixel
// stage needs to bring a tangent space normal into world space. They are filled
// by the factory's VFTransformVertex, which runs after the material's vertex
// entry point, so a material cannot change them.
// ---------------------------------------------------------------------------

	float4	Position		: SV_Position;
	float3	WorldPosition	: TEXCOORD0;
	float3	WorldNormal		: TEXCOORD1;
	float3	WorldTangent	: TEXCOORD2;
	float3	WorldBitangent	: TEXCOORD3;
	float2	UV0				: TEXCOORD4;
	float2	UV1				: TEXCOORD5;
	float4	Color			: TEXCOORD6;
#endif	//IMGUI_VARYING_HLSL
