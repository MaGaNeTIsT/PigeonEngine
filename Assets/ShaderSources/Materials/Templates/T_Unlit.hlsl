#ifndef T_UNLIT_HLSL
#define T_UNLIT_HLSL

// ---------------------------------------------------------------------------
// Unlit material template.
//
// The starting point for a material in the Unlit domain. It is short because the
// domain is: colour and coverage, no lighting.
//
// The field set here is the one D_Unlit declares, which is the same nine core
// fields every domain has. A domain that extended them would show the extension
// in its own template, so what an artist can write follows the domain they chose.
// ---------------------------------------------------------------------------

#if SHADER_STAGE_VERTEX
void CalcMaterialVertex(in MaterialVertexInput InInput, out float3 OutWorldPositionOffset, inout VFVarying InOutVarying)
{
	OutWorldPositionOffset = float3(0.0, 0.0, 0.0);
}
#endif	//SHADER_STAGE_VERTEX

#if SHADER_STAGE_PIXEL
void CalcMaterialSurface(in MaterialPixelInput InInput, out MaterialSurface OutSurface)
{
	InitMaterialSurface(OutSurface);

	OutSurface.BaseColor = BaseColor;
	OutSurface.EmissiveColor = EmissiveColor.rgb;
}

// Coverage only, for the depth only passes. Same default as the DefaultLit
// template: correct but not cheap, and a material that cares overrides it.
void CalcMaterialOpacity(in MaterialPixelInput InInput, out float OutOpacity, out float OutAlphaCutoff)
{
	MaterialSurface TempSurface;
	InitMaterialSurface(TempSurface);
	CalcMaterialSurface(InInput, TempSurface);
	OutOpacity = TempSurface.Opacity;
	OutAlphaCutoff = TempSurface.AlphaCutoff;
}
#endif	//SHADER_STAGE_PIXEL

#endif	//T_UNLIT_HLSL
