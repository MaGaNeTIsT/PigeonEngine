#ifndef M_BEZIER_GRASS_HLSL
#define M_BEZIER_GRASS_HLSL

// ---------------------------------------------------------------------------
// Bezier grass material.
//
// The port of the legacy M_BezierGrass material: a blade coloured by a root to
// tip gradient, with no textures and no alpha, drawn into the deferred targets
// as a DefaultLit surface. The legacy material declared no texture and sampled
// none, so this one declares none either and the surface mode is Opaque; a
// material that wants a clipped silhouette turns that into Mask and adds its
// own texture, which is a change to this file and to the JSON beside it.
//
// ---------------------------------------------------------------------------
// Why this material has no vertex entry point
//
// A blade is not a mesh, so all of its geometry is the vertex factory's:
// Public/VertexFactory/ProceduralGrassVF.hlsl turns the per vertex handle and the
// packed instance into the curve point, and the material has nothing left to say
// about where a blade is. VertexOps is false in the JSON beside this file, which
// is the design's own mechanism for exactly that case: the vertex stage then
// lists Materials/M_EngineNeutralVertex.json in place of this material, and every
// material that does not touch vertices shares one compiled vertex shader.
//
// That is not a corner cut. The neutral vertex material is a replacement, not a
// fallback, and it returns the same zero world position offset this file would
// return. A material that later wants to displace a blade -- the wind a material
// owns rather than the wind the construct pass bakes, a per material bend, a
// flattening under a footprint -- writes the vertex entry point back into this
// file and flips VertexOps to true, and the factory's VFTransformVertex already
// applies the offset it returns.
//
// ---------------------------------------------------------------------------
// How the blade reaches the pixel stage
//
// Through UV0, which the factory owns: y is the position along the blade from
// root to tip and x is the side coordinate. The legacy material read the same
// channel for the same blend, and it is the only channel available, because the
// pixel stage interface a material sees carries the system interpolants and not
// a material's own. The blade's geometric normal arrives the same way: the
// factory builds the blade's world frame, this material leaves the surface normal
// at its tangent space default, and the domain's derive step composes the two.
//
// ---------------------------------------------------------------------------
// What the port changed
//
//   old the material parameter block declared RootColor, TipColor, BentBezierT,
//       Roughness, Metallic and a _pad0 field the shader never read
//       new RootColor, TipColor, Roughness and Metallic, in the same types and
//       with the legacy defaults. The padding field is gone because the compiler
//       packs the block and the shader had no use for it, and BentBezierT is gone
//       because it is not a material parameter any more: it is the position along
//       the blade where the bent blade's two halves meet, the factory reads it
//       while it builds the blade, and a material runs after that. It is declared
//       in this factory's own constant buffer,
//       Components/C_VertexFactory.json, which is the only place that can reach
//       the builder.
//   old the whole parameter block was one flat cbuffer declared in
//       M_BezierGrass/Material.json in the legacy schema
//       new one MaterialParams block declared in Materials/M_BezierGrass.json in
//       the component schema, with the same artist facing fields marked exposed.
//   old the legacy material's pixel function and its helper file, neither of which
//       had any content beyond the gradient
//       new CalcMaterialSurface below, plus the CalcMaterialOpacity every
//       material carries so a depth pass can ask for coverage without evaluating
//       the whole material.
// ---------------------------------------------------------------------------

#if SHADER_STAGE_PIXEL
void CalcMaterialSurface(in MaterialPixelInput InInput, out MaterialSurface OutSurface)
{
	// Every field starts at the domain's default, which is what removes the
	// "is this input connected" question from material code.
	InitMaterialSurface(OutSurface);

	// Root to tip, driven by the factory's blade coordinate. The legacy lerped
	// the same two colours with the same channel and had no falloff or noise
	// beyond it.
	OutSurface.BaseColor = lerp(RootColor, TipColor, InInput.UV0.y);

	// The legacy material passed both of these straight through from its
	// parameters. Roughness is the perceptual value the artist writes in this
	// domain, which is the same quantity the legacy GBuffer carried, and the
	// derive step is what squares it for the BRDF.
	OutSurface.Roughness = Roughness;
	OutSurface.Metallic = Metallic;

	// Reflectance, ambient occlusion, emissive and the surface normal keep their
	// defaults: the legacy material set none of them, and the blade's normal is
	// the frame the factory produced rather than something this material writes.
}

// Coverage only, for a pass that clips but does not shade. Same shape as every
// other material's: the helper is here rather than behind an engine switch
// because it is the material's own optimisation, and a material that can answer
// it more cheaply is free to replace the body.
void CalcMaterialOpacity(in MaterialPixelInput InInput, out float OutOpacity, out float OutAlphaCutoff)
{
	MaterialSurface TempSurface;
	InitMaterialSurface(TempSurface);
	CalcMaterialSurface(InInput, TempSurface);
	OutOpacity = TempSurface.Opacity;
	OutAlphaCutoff = TempSurface.AlphaCutoff;
}
#endif	//SHADER_STAGE_PIXEL
#endif	//M_BEZIER_GRASS_HLSL
