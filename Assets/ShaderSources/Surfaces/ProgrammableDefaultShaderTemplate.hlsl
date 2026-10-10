#ifndef PROGRAMMABLE_DEFAULT_SHADER_TEMPLATE_HLSL
#define PROGRAMMABLE_DEFAULT_SHADER_TEMPLATE_HLSL

// ---------------------------------------------------------------------------
// Default material implementation, and the worked example of the material
// contract.
//
// This component occupies the material slot for programs that have no material
// of their own, and it is what makes "there is no material" a defined state
// rather than a missing one. A material and this component are mutually
// exclusive: both would provide the MATERIAL_FUNCTIONS key, and two providers
// for one key is a hard compiler error. A program points at this file by naming
// Surfaces/ProgrammableDefaultShaderTemplate.json as its material component.
//
// It is also the file to read first when writing a material, because everything
// a material does is here and nothing else is required:
//
//   CalcMaterialSurface   the pixel entry point. It is given the interpolants and
//                         the domain's MaterialSurface, and writes the fields it
//                         has an opinion about.
//   CalcMaterialOpacity   optional. Splits the opacity answer out of the surface
//                         evaluation for the depth only passes, which need that
//                         one channel and nothing else.
//   CalcMaterialVertex    optional, and not provided here. A material that
//                         displaces or animates geometry writes it and declares
//                         VertexOps true in its json; every other material is
//                         given Materials/M_EngineNeutralVertex.json for its
//                         vertex stage, which is what lets all of them share one
//                         compiled vertex shader. Materials/Templates/T_DefaultLit.hlsl
//                         is the fuller template, with the vertex half in it.
//
// OutSurface is the selected domain's structure, so which fields a material may
// write is decided by the domain the material was created from rather than by
// this file. The body starts from InitMaterialSurface, which fills every field
// with its default, so a material writes only what it means to change.
//
// This component declares no constant buffer of its own, which is why the body
// below is written with literals: a program that wants parameters in its material
// slot should use a material json instead - Materials/Templates/T_DefaultLit.* is
// the starting point for one - rather than this file.
//
// What it draws is the same look as M_EngineFallback: neutral grey with a
// checkerboard whose lighter square is glossy, laid out triplanar in world space
// with the cell size taken from PE_EDITOR_CHECKER_GRID_CM, so a mesh is marked
// without needing a UV channel. The two are the same look on purpose: one marks a
// mesh nobody assigned a material to, this one stands in for a program that has
// no material slot at all.
// ---------------------------------------------------------------------------

#if SHADER_STAGE_PIXEL

// The engine publishes the cell size through its common settings component. This
// file keeps a default of its own so that a pass which does not declare that
// component can still compile it.
#ifndef PE_EDITOR_CHECKER_GRID_CM
#define PE_EDITOR_CHECKER_GRID_CM	(50)
#endif	//PE_EDITOR_CHECKER_GRID_CM

// Which of the two squares this pixel lands on: 0.0 for one, 1.0 for its
// neighbour. The two axes of the dominant normal direction are the projection
// plane, which is what lets the same pattern land on a floor and on a wall
// without either of them owning a UV channel.
float GetDefaultMaterialCheckerParity(in float3 InWorldPosition, in float3 InWorldNormal)
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

	// Every field left untouched above keeps the default InitMaterialSurface gave it, which is
	// the whole point of starting from it: a material states its differences, not its baseline.
	const float CheckerParity = GetDefaultMaterialCheckerParity(InInput.WorldPosition, InInput.WorldNormal);
	OutSurface.BaseColor = float4(0.5, 0.5, 0.5, 1.0) * lerp(0.6, 1.0, CheckerParity);
	OutSurface.Roughness = lerp(0.6, 0.05, CheckerParity);
	OutSurface.Metallic = 0.0;
	OutSurface.Reflectance = 0.5;
	OutSurface.AmbientOcclusion = 1.0;
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

#endif	//PROGRAMMABLE_DEFAULT_SHADER_TEMPLATE_HLSL
