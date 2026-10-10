#ifndef M_ENGINE_DEFAULT_LIT_HLSL
#define M_ENGINE_DEFAULT_LIT_HLSL

// ---------------------------------------------------------------------------
// Engine neutral lighting material.
//
// A concrete DefaultLit material with no textures and no vertex work: the mesh's
// own shape lit by the engine's lights. It is what a mesh gets before an artist
// assigns anything, and it is deliberately dull so that a missing material
// assignment looks like one.
//
// VertexOps is false in its JSON, so its vertex stage is
// Materials/M_EngineNeutralVertex.json and every mesh using it shares one
// compiled vertex shader. Only the pixel half lives here.
// ---------------------------------------------------------------------------

#if SHADER_STAGE_PIXEL
void CalcMaterialSurface(in MaterialPixelInput InInput, out MaterialSurface OutSurface)
{
	InitMaterialSurface(OutSurface);

	OutSurface.BaseColor = BaseColor;
	OutSurface.Roughness = Roughness;
	OutSurface.Metallic = Metallic;
	OutSurface.Reflectance = Reflectance;
	OutSurface.AmbientOcclusion = AmbientOcclusion;
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

#endif	//M_ENGINE_DEFAULT_LIT_HLSL
