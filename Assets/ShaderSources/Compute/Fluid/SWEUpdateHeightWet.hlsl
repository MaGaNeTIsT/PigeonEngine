#ifndef SWE_UPDATE_HEIGHT_WET_HLSL
#define SWE_UPDATE_HEIGHT_WET_HLSL

// ---------------------------------------------------------------------------
// Shallow water equation, update height wet.
//
// Provenance: SWE_UpdateHeightWet.usf, the pass that turns the simulation state
// into the two channel texture the water reads: .r the outline height the surface
// is drawn at and .g the wetness that decides whether there is any water at all.
// It averages the previous frame's height and wetness over a five tap cross,
// takes the outline state of the current frame, and blends the two: the border
// height comes from the outline or from the previous frame, the water height is
// the smoothed surface, the foam and the fluid thickness on top of it, the
// wetness dries at the configured speed, and the result is the height blended
// between the border and the water by how much volume the cell holds. Every
// expression below is the legacy one, character for character; what changed is
// the parameter path, the entry point name and the notes below.
//
// What the port changed:
//
//   old Texture2D PreviousHeightWetTex : register(t0), VelocityHeightFoamTex,
//   GroundTex, plus the #define aliases that named them
//       new the three names declared in Compute/Fluid/SWEUpdateHeightWet.json.
//       Note which texture this pass's VelocityHeightFoamTex is: the second
//       input, because the previous height wet state comes first here. The
//       shared FluxSample reads it and GroundTex by name, so their names and the
//       sampler are what the library needs, not their slots
//   old PreviousHeightWetTex.SampleLevel( UCGSampler, UV, 0 ).rg and the four
//   clamped taps around it
//       new PE_SAMPLE_TEX2D_LEVEL( PreviousHeightWetTex, SamplerLinearClamp, UV, 0 ).rg
//       with the same five texel centre UVs and the same clamp of the coordinate
//       to the resolution minus one, so the border still repeats the edge texel
//   old RWTexture2D<float2> HeightWetOut0, declared in the legacy pass itself
//   because the shared UCGOutputN were RGBA16f for the integrate passes
//       new the RWTextures entry OutHeightWetTex with Format float2, which is the
//       name the legacy body already used for it through its own #define. The two
//       channel target is the point: the C++ side bound this slot with its own
//       parameter struct, and the component JSON is where that is said now
//   old the SWE_ parameters and the fourteen Outline* #defines of
//   UCGVectorParams
//       new named SWEParams fields, declared in the component JSON: the same
//       twenty fields the two already migrated SWE passes declare, then the
//       pass's own fields in the legacy order. The legacy's OutlineDryingBlur has
//       no define and no reader (only the disabled formula in the comment below
//       mentions it), so it is not declared here either
//   old FluxOutline of the legacy shared header, switched on for this pass by
//   SWE_USE_FLUX_OUTLINE_FUNCTION
//       new FluxOutlineState in Public/Fluid/ShallowWaterEquationOutline.hlsl,
//       the same nine output function under the name it can have here: the
//       already migrated Public/Fluid/ShallowWaterEquationCommon.hlsl carries a
//       different FluxOutline, this pass includes both files, and two functions
//       of one name would not compile. The call site passes the same
//       float2( 1, 1 ) tap spacing and the same true, false flags
//   old the FluxSample call of the current texel and of the outline
//       new unchanged, through the shared library
//   old MainCS
//       new CSMain, the Entry the program description names
//   this pass has no early exit, no water mask, no pixel branch and no bounds
//   anchor in the legacy beyond the resolution test it keeps below, and none
//   here either
// ---------------------------------------------------------------------------

#include "Public/Fluid/ShallowWaterEquationCommon.hlsl"
#include "Public/Fluid/ShallowWaterEquationOutline.hlsl"

[numthreads(SHADER_THREAD_GROUP_X, SHADER_THREAD_GROUP_Y, 1)]
void CSMain(uint3 InDispatchThreadID : SV_DispatchThreadID)
{
	float2 Resolution;
	{
		float2 Location; float2 Scale; float2 InvScale; float Height; float2 TexelSize;
		FluxSimulationUV(Location, Scale, InvScale, Height, TexelSize, Resolution);
	}

	int2 Coord = int2(InDispatchThreadID.xy);
	if (any(Coord >= int2(Resolution.x, Resolution.y)))
	{
		return;
	}

	float2 UV = (float2(Coord) + 0.5) / Resolution;
	float2 PrevCenterHeightWet = PE_SAMPLE_TEX2D_LEVEL( PreviousHeightWetTex, SamplerLinearClamp, UV, 0 ).rg;
	float2 PrevHeightWetSum = PrevCenterHeightWet;
	{
		int2 OutputResMinus1 = int2(Resolution.x, Resolution.y) - int2(1, 1);
		float2 PrevUV = (float2(clamp(Coord + int2(1, 1), int2(0, 0), OutputResMinus1)) + 0.5) / Resolution;
		PrevHeightWetSum += PE_SAMPLE_TEX2D_LEVEL( PreviousHeightWetTex, SamplerLinearClamp, PrevUV, 0 ).rg;
		PrevUV = (float2(clamp(Coord + int2(-1, -1), int2(0, 0), OutputResMinus1)) + 0.5) / Resolution;
		PrevHeightWetSum += PE_SAMPLE_TEX2D_LEVEL( PreviousHeightWetTex, SamplerLinearClamp, PrevUV, 0 ).rg;
		PrevUV = (float2(clamp(Coord + int2(-1, 1), int2(0, 0), OutputResMinus1)) + 0.5) / Resolution;
		PrevHeightWetSum += PE_SAMPLE_TEX2D_LEVEL( PreviousHeightWetTex, SamplerLinearClamp, PrevUV, 0 ).rg;
		PrevUV = (float2(clamp(Coord + int2(1, -1), int2(0, 0), OutputResMinus1)) + 0.5) / Resolution;
		PrevHeightWetSum += PE_SAMPLE_TEX2D_LEVEL( PreviousHeightWetTex, SamplerLinearClamp, PrevUV, 0 ).rg;
	}
	float2 PrevHeightWetAvg = (PrevHeightWetSum * 0.2) + (OutlineStateWetHeightOffset);
	PrevCenterHeightWet = PrevCenterHeightWet + (OutlineStateWetHeightOffset);

	float2 OutlineSlope; float OutlineHeightAvg, OutlineWaterHeight, OutlineBorderWater, OutlineBorderHeight, OutlineWaterMin, OutlineWaterMax, OutlineGroundMin, OutlineGroundMax;
	FluxOutlineState(UV, float2(1, 1),
		(OutlineMinWater),			// 0.007
		(OutlineFallingSpeed),		// 0.7
		(OutlineMinOutline),		// -1300.0
		(OutlineAverage),			// 0.0
		true, false,
		OutlineHeightAvg, OutlineSlope, OutlineWaterHeight, OutlineBorderWater, OutlineBorderHeight, OutlineWaterMin, OutlineWaterMax, OutlineGroundMin, OutlineGroundMax
	);

	float2 CurrentVel; float CurrentV; float CurrentF; float CurrentGH; float CurrentGHLocal; float CurrentSH;
	FluxSample( UV, float2( 0, 0 ), true, CurrentVel, CurrentV, CurrentF, CurrentGH, CurrentGHLocal, CurrentSH );

	float Border = (OutlineWaterMax > (OutlineMinWater)) ? OutlineBorderHeight : min(PrevHeightWetAvg.x, min(OutlineGroundMin, CurrentGH) - 10.0);
	Border = lerp(Border, PrevHeightWetAvg.x, (OutlineNoWaterBlend));

	float Water = max(lerp(CurrentSH, OutlineHeightAvg, (OutlineSurfaceSmoothing)), lerp(OutlineGroundMax, PrevCenterHeightWet.x - (OutlineThickness), 0.9));
	Water = lerp(CurrentSH, Water, saturate(OutlineWaterMin / 0.03)) + (CurrentF * (OutlineFoamThickness) + (OutlineFluidThickness));

	// max(PrevCenterHeightWet.y, lerp(PrevCenterHeightWet.y, PrevHeightWetAvg.y, OutlineDryingBlur))
	float Wet = PrevCenterHeightWet.y - max((OutlineDryingSpeed), saturate((OutlineWaterMin - (OutlineDryingOffset)) * 1000.0));

	float Height = lerp(Border, Water, saturate(CurrentV / (OutlineBorderBlend)));
	float2 HeightWet = float2(Height, max(min(OutlineGroundMax + (OutlineWetnessLimit), Wet), Height));
	HeightWet = HeightWet - (OutlineStateWetHeightOffset);

	OutHeightWetTex[Coord] = HeightWet.xy;
}

#endif	//SWE_UPDATE_HEIGHT_WET_HLSL
