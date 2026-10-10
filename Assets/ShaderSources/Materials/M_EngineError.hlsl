#ifndef M_ENGINE_ERROR_HLSL
#define M_ENGINE_ERROR_HLSL

// ---------------------------------------------------------------------------
// The material a mesh is drawn with when the material it asked for could not be
// assembled - a compile failure behind that material, or any other reason the
// instance could not be created.
//
// It is deliberately not the same look as M_EngineFallback. That one stands in
// for a mesh nobody assigned a material to, which is a normal state and stays
// quiet; this one means something is broken, so it is magenta grid lines over
// near black cells, with the lines glossy enough to throw a highlight that moves
// as the camera orbits. Whoever sees it in the frame knows which object has the
// bad material, and the log line that named the failed compile says which
// material that is.
//
// Triplanar in world space like the fallback's, for the same reason: a mesh is
// marked without owning a UV channel, and the cell size comes from the engine's
// common settings rather than from a constant of this file.
//
// Only the pixel half lives here. VertexOps is false in the JSON, so the vertex
// stage is Materials/M_EngineNeutralVertex.json and every mesh wearing this
// shares one compiled vertex shader.
// ---------------------------------------------------------------------------

#if SHADER_STAGE_PIXEL

// The engine publishes the cell size through its common settings component. The
// error material keeps a default of its own so that a pass which does not declare
// that component can still compile it: a marker that disappears depending on
// which blocks a program includes would be worse than a marker with a fixed cell.
#ifndef PE_EDITOR_CHECKER_GRID_CM
#define PE_EDITOR_CHECKER_GRID_CM	(50)
#endif	//PE_EDITOR_CHECKER_GRID_CM

// Where this pixel sits inside its grid cell, on the plane the dominant normal
// direction faces. The cell size is handed back because the caller measures the
// line width in meters rather than in cell-relative units, which is what keeps a
// line the same thickness when the settings value changes.
float2 GetErrorGridCellLocal(in float3 InWorldPosition, in float3 InWorldNormal, out float OutCellSizeMeters)
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
	OutCellSizeMeters = ((float)PE_EDITOR_CHECKER_GRID_CM) * 0.01;
	return frac(PlanarPosition / OutCellSizeMeters);
}

void CalcMaterialSurface(in MaterialPixelInput InInput, out MaterialSurface OutSurface)
{
	InitMaterialSurface(OutSurface);

	float CellSizeMeters = 0.0;
	const float2 CellLocal = GetErrorGridCellLocal(InInput.WorldPosition, InInput.WorldNormal, CellSizeMeters);
	const float2 DistanceToBorder = min(CellLocal, 1.0 - CellLocal) * CellSizeMeters;
	const float LineWidthMeters = CellSizeMeters * 0.08;
	const float LineMask = (min(DistanceToBorder.x, DistanceToBorder.y) <= LineWidthMeters) ? 1.0 : 0.0;

	// Both ends of the marker are literals: nothing in the scene may talk the error material into
	// looking like an authored one. The two fields read below are the ones that cannot weaken it,
	// and they keep the block live, so this material declares the same buffer as every other.
	OutSurface.BaseColor = lerp(float4(0.02, 0.0, 0.02, 1.0), float4(1.0, 0.0, 1.0, 1.0), LineMask);
	OutSurface.Roughness = lerp(0.85, 0.05, LineMask);
	OutSurface.Metallic = 0.0;
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

#endif	//M_ENGINE_ERROR_HLSL
