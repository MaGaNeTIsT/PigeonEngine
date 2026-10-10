#ifndef M_ENGINE_FALLBACK_HLSL
#define M_ENGINE_FALLBACK_HLSL

// ---------------------------------------------------------------------------
// The engine's fallback material: what a mesh is drawn with when nobody assigned
// it a material.
//
// This is the quiet of the two engine stand-ins. A mesh arrives here with an
// empty material slot, which is a normal state rather than a fault, so the
// material only has to be recognisable, not alarming: a mesh whose material could
// not be assembled is a fault, and it wears Materials/M_EngineError.hlsl instead,
// which is loud on purpose.
//
// Neutral grey with the lighter square glossy: the tight highlight on those
// squares moves as the camera orbits, which is what separates a placeholder
// from a flat test pattern in a still screenshot. The grid is laid out
// triplanar in world space, so a mesh is marked without needing a UV channel,
// and the cell size comes from the engine's common settings rather than from a
// constant of this file.
//
// Only the pixel half lives here. VertexOps is false in the JSON, so the vertex
// stage is Materials/M_EngineNeutralVertex.json and every mesh wearing the
// placeholder shares one compiled vertex shader.
// ---------------------------------------------------------------------------

#if SHADER_STAGE_PIXEL

// The engine publishes the cell size through its common settings component. The
// fallback keeps a default of its own so that a pass which does not declare that
// component can still compile the placeholder: its look may not depend on which
// blocks a given program happened to include.
#ifndef PE_EDITOR_CHECKER_GRID_CM
#define PE_EDITOR_CHECKER_GRID_CM	(50)
#endif	//PE_EDITOR_CHECKER_GRID_CM

// Which of the two squares this pixel lands on: 0.0 for one, 1.0 for its
// neighbour. The two axes of the dominant normal direction are the projection
// plane, which is what lets the same pattern land on a floor and on a wall
// without either of them owning a UV channel.
float GetFallbackCheckerParity(in float3 InWorldPosition, in float3 InWorldNormal)
{
	const float3 AbsNormal = abs(InWorldNormal);
	float2 PlanarPosition;
	if ((AbsNormal.x >= AbsNormal.y) && (AbsNormal.x >= AbsNormal.z))
	{
		PlanarPosition = InWorldPosition.zy;
	}
	else if (AbsNormal.y >= AbsNormal.z)
	{
		PlanarPosition = InWorldPosition.xz;
	}
	else
	{
		PlanarPosition = InWorldPosition.xy;
	}
	const float CellSizeMeters = ((float)PE_EDITOR_CHECKER_GRID_CM) * 0.01;
	const float2 Cell = floor(PlanarPosition / CellSizeMeters);
	return frac((Cell.x + Cell.y) * 0.5) * 2.0;
}

void CalcMaterialSurface(in MaterialPixelInput InInput, out MaterialSurface OutSurface)
{
	InitMaterialSurface(OutSurface);

	const float CheckerParity = GetFallbackCheckerParity(InInput.WorldPosition, InInput.WorldNormal);
	// Two grey levels rather than two colours, so the placeholder cannot be mistaken for a
	// material decision, and the lighter square is the glossy one for the moving highlight.
	OutSurface.BaseColor = BaseColor * lerp(0.6, 1.0, CheckerParity);
	OutSurface.Roughness = lerp(Roughness, 0.05, CheckerParity);
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

#endif	//M_ENGINE_FALLBACK_HLSL
