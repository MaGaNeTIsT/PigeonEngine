#ifndef T_DEFAULT_LIT_HLSL
#define T_DEFAULT_LIT_HLSL

// ---------------------------------------------------------------------------
// DefaultLit material template.
//
// This is the file an artist copies to make a material. Everything below is a
// starting point, not a framework: a material is free to delete the vertex entry
// point, to compute BaseColor from anything it likes, and to add its own
// interpolants.
//
// What a material may write, in this domain: the fields of MaterialSurface as
// this domain declares them. They are the same nine the documentation in
// Public/Surface/MaterialSurface.hlsl lists, and a domain that extends the set
// would extend them here. That is why the fields a material can write follow the
// domain it was created from.
//
// Sampling always goes through SAMPLE_TEX2D and friends; see
// Public/Material/ShaderMaterialCommon.hlsl.
// ---------------------------------------------------------------------------

// --- vertex entry point ---------------------------------------------------
//
// Optional. A material that does not displace or animate anything can delete
// this whole function and set VertexOps to false in its JSON, which routes the
// vertex stage to Materials/M_EngineNeutralVertex.json instead and lets every
// such material share one compiled vertex shader.
//
// Writing to InOutVarying is only for the material's own interpolants. The
// engine writes the system members after this returns, so a write to one of
// those is overwritten rather than rejected.
#if SHADER_STAGE_VERTEX
void CalcMaterialVertex(in MaterialVertexInput InInput, out float3 OutWorldPositionOffset, inout VFVarying InOutVarying)
{
	OutWorldPositionOffset = float3(0.0, 0.0, 0.0);
}
#endif	//SHADER_STAGE_VERTEX

// --- pixel entry point ---------------------------------------------------

#if SHADER_STAGE_PIXEL
void CalcMaterialSurface(in MaterialPixelInput InInput, out MaterialSurface OutSurface)
{
	InitMaterialSurface(OutSurface);

	OutSurface.BaseColor = BaseColor;
	OutSurface.Roughness = Roughness;
	OutSurface.Metallic = Metallic;
	OutSurface.Reflectance = Reflectance;
	OutSurface.AmbientOcclusion = AmbientOcclusion;
	OutSurface.EmissiveColor = EmissiveColor.rgb;
}

// --- opacity only -------------------------------------------------------
//
// The depth only passes need opacity but nothing else, and evaluating a whole
// material just to read one channel is wasteful. This entry point exists so a
// material can answer that question cheaply; the default below is correct but
// not cheap, and a material that cares overrides it.
//
// It lives here rather than behind an engine switch because it is the material's
// own optimisation: the artist can see it, replace it, or leave it alone.
void CalcMaterialOpacity(in MaterialPixelInput InInput, out float OutOpacity, out float OutAlphaCutoff)
{
	MaterialSurface TempSurface;
	InitMaterialSurface(TempSurface);
	CalcMaterialSurface(InInput, TempSurface);
	OutOpacity = TempSurface.Opacity;
	OutAlphaCutoff = TempSurface.AlphaCutoff;
}
#endif	//SHADER_STAGE_PIXEL
#endif	//T_DEFAULT_LIT_HLSL
