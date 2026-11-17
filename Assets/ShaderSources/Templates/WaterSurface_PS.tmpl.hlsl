#ifndef WATER_SURFACE_PS_TMPL_HLSL
#define WATER_SURFACE_PS_TMPL_HLSL

// ---------------------------------------------------------------------------
// Water surface pass, pixel stage.
//
// The forward translucent shading path plus the water simulation declarations.
// The water material samples WaterStateTex and WaterHeightWetTex; the pass blends
// the result into the scene colour with the surface's own opacity.
// ---------------------------------------------------------------------------

#include "Public/Lighting/ShaderDirectLighting.hlsl"
#include "Public/Lighting/ShaderIndirectLighting.hlsl"

{SYSTEM_MACROS}
{WATER_SYSTEM}
{VF_VARYING_STRUCT}
{DRAW_PARAMS}
{VF_VERTEX_FACTORY}
{PROGRAMMABLE}
{LIGHTING}

void PSMain(in VFVarying InVarying,
	in uint InPrimitiveID : SV_PrimitiveID,
	in bool InIsFrontFace : SV_IsFrontFace,
	out float4 OutSceneColor : SV_Target0)
{
	MaterialPixelInput MatInput = BuildMaterialPixelInput(InVarying, ViewPosition.xyz, InPrimitiveID, InIsFrontFace);

	SurfaceData Surface;
	ShadingResult Result;
	EvaluateSurface(MatInput, Surface, Result);

	float3 Color = Result.Diffuse + Result.Specular + Surface.EmissiveColor;
	OutSceneColor = float4(Color, Surface.Opacity);
}

#endif	//WATER_SURFACE_PS_TMPL_HLSL
