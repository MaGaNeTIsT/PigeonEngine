#ifndef M_ENGINE_UNLIT_HLSL
#define M_ENGINE_UNLIT_HLSL

// ---------------------------------------------------------------------------
// Engine unlit material.
//
// A flat colour with no lighting, for guides, gizmos and debug geometry. The
// Unlit domain carries it: the material writes a colour, the domain claims no
// light channel, and the lighting pass skips those pixels without shading them.
//
// VertexOps is false in its JSON, so it shares the neutral vertex shader with
// every other material that does not touch vertices.
// ---------------------------------------------------------------------------

#if SHADER_STAGE_PIXEL
void CalcMaterialSurface(in MaterialPixelInput InInput, out MaterialSurface OutSurface)
{
	InitMaterialSurface(OutSurface);

	OutSurface.BaseColor = BaseColor;
}

void CalcMaterialOpacity(in MaterialPixelInput InInput, out float OutOpacity, out float OutAlphaCutoff)
{
	MaterialSurface TempSurface;
	InitMaterialSurface(TempSurface);
	CalcMaterialSurface(InInput, TempSurface);
	OutOpacity = TempSurface.Opacity;
	OutAlphaCutoff = TempSurface.AlphaCutoff;
}
#endif	//SHADER_STAGE_PIXEL

#endif	//M_ENGINE_UNLIT_HLSL
