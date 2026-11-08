#ifndef M_WATER_DEBUG_HLSL
#define M_WATER_DEBUG_HLSL

// ---------------------------------------------------------------------------
// Water surface sample material.
//
// This is a deliberately small water shading model on top of the simulation, not the final
// artist facing water material. It still shows the things a water surface needs to read as
// water:
//
//   - the mesh is displaced along world Y from the simulation height
//   - the pixel normal comes from the height field's gradient, with a procedural ripple
//     field advected by the simulation velocity (the state texture's .xy)
//   - shallow/deep scattering colour and opacity come from the water column depth
//   - the DefaultLit domain supplies sun and environment specular from that normal
//   - the scene colour copy taken before this draw supplies screen space refraction
//
// The simulation plane is world XZ and the height axis is world Y. WaterAreaLoc and
// WaterAreaSize map a world XZ position to the simulation texture. A final water material
// can keep the same inputs and replace the look.
// ---------------------------------------------------------------------------

float2 WaterSimulationUV(in float3 InWorldPosition)
{
	return ((InWorldPosition.xz - WaterAreaLoc) / max(WaterAreaSize, float2(0.0001f, 0.0001f)));
}

float WaterSampleHeight(in float2 InUV)
{
	return SAMPLE_TEX2D_LEVEL(WaterHeightWetTex, SamplerLinearClamp, InUV, 0).r;
}

#if SHADER_STAGE_VERTEX
void CalcMaterialVertex(in MaterialVertexInput InInput, out float3 OutWorldPositionOffset, inout VFVarying InOutVarying)
{
	float2 SimulationUV = WaterSimulationUV(InInput.WorldPosition);
	// A vertex shader has no screen space derivatives, so the sample needs an explicit
	// level: SAMPLE_TEX2D (Texture2D.Sample) has no vs_5_0 mapping.
	float2 HeightWet = SAMPLE_TEX2D_LEVEL(WaterHeightWetTex, SamplerLinearClamp, SimulationUV, 0).rg;
	float SurfaceOffset = (HeightWet.r - WaterReferenceY) * WaterHeightScale * saturate(HeightWet.g) * saturate(WaterEnable);
	OutWorldPositionOffset = float3(0.0f, SurfaceOffset, 0.0f);
}
#endif	//SHADER_STAGE_VERTEX

#if SHADER_STAGE_PIXEL
void CalcMaterialSurface(in MaterialPixelInput InInput, out MaterialSurface OutSurface)
{
	InitMaterialSurface(OutSurface);

	float2 SimulationUV = WaterSimulationUV(InInput.WorldPosition);
	float4 State = SAMPLE_TEX2D(WaterStateTex, SamplerLinearClamp, SimulationUV);
	float2 HeightWet = SAMPLE_TEX2D(WaterHeightWetTex, SamplerLinearClamp, SimulationUV).rg;
	float Wetness = saturate(HeightWet.g) * saturate(WaterEnable) * saturate(WaterSimulationValid);

	// -----------------------------------------------------------------------
	// Normal. The height texture is world Y, so the surface normal is the height
	// gradient in world XZ. The taps are one simulation texel apart; the area
	// converts that UV step back to world units.
	// -----------------------------------------------------------------------
	float2 Texel = max(WaterTexelSize, float2(1e-6f, 1e-6f));
	float HeightL = WaterSampleHeight(SimulationUV - float2(Texel.x, 0.0f));
	float HeightR = WaterSampleHeight(SimulationUV + float2(Texel.x, 0.0f));
	float HeightD = WaterSampleHeight(SimulationUV - float2(0.0f, Texel.y));
	float HeightU = WaterSampleHeight(SimulationUV + float2(0.0f, Texel.y));
	float DhDx = (HeightR - HeightL) / max(2.0f * WaterAreaSize.x * Texel.x, 1e-4f);
	float DhDz = (HeightU - HeightD) / max(2.0f * WaterAreaSize.y * Texel.y, 1e-4f);

	// The simulation velocity is a flow map: it advects the fine ripple field, so the
	// detail visibly travels the way the water is moving.
	float2 Flow = State.xy;
	float2 FlowUV = SimulationUV * 64.0f - Flow * WaterTime * 0.0005f;
	float2 FlowGradient;
	FlowGradient.x = cos(FlowUV.x * 6.2831853f + sin(FlowUV.y * 2.3999632f));
	FlowGradient.y = cos(FlowUV.y * 6.2831853f + sin(FlowUV.x * 1.6180340f));
	float FlowScale = WaterFlowStrength * 0.05f;
	DhDx += FlowGradient.x * FlowScale;
	DhDz += FlowGradient.y * FlowScale;

	float3 WorldNormal = normalize(float3(-DhDx * WaterNormalStrength, 1.0f, -DhDz * WaterNormalStrength));
	OutSurface.Normal = normalize(float3(
		dot(WorldNormal, InInput.WorldTangent),
		dot(WorldNormal, InInput.WorldBitangent),
		dot(WorldNormal, InInput.WorldNormal)));

	// -----------------------------------------------------------------------
	// Scattering. The water column depth decides the shallow/deep colour and how
	// much of the background survives the blend.
	// -----------------------------------------------------------------------
	float Depth = max(State.z, 0.0f);
	float DepthFactor = saturate(Depth / max(WaterScatterDepth, 0.0001f));
	float3 ShallowColor = float3(0.03f, 0.34f, 0.42f);
	float3 DeepColor = float3(0.004f, 0.045f, 0.13f);
	float3 ScatterColor = lerp(ShallowColor, DeepColor, DepthFactor);
	// A little more body where the surface turns away from the eye: the classic darker rim.
	float NdotV = saturate(dot(WorldNormal, InInput.ViewDirection));
	ScatterColor += DeepColor * (1.0f - NdotV) * 0.5f;
	// Foam is a hint from the simulation; keep it subtle so the depth colour stays readable.
	ScatterColor = lerp(ScatterColor, float3(0.75f, 0.85f, 0.9f), saturate(State.w) * 0.5f);

	OutSurface.BaseColor = float4(ScatterColor, 1.0f);
	OutSurface.Roughness = 0.08f;
	OutSurface.Metallic = 0.0f;
	OutSurface.Reflectance = 0.35f;
	OutSurface.AmbientOcclusion = 1.0f;
	OutSurface.Opacity = saturate(0.35f + 0.65f * DepthFactor) * Wetness;

	// -----------------------------------------------------------------------
	// Refraction. The renderer copied the scene colour before this draw. The
	// alpha blend already supplies the unrefracted background, so the emitted
	// correction replaces its share with the refracted sample:
	//   final = lit * Opacity + Refracted * (1 - Opacity)
	// The offset is the surface normal projected onto the screen, scaled by the
	// water column depth.
	// -----------------------------------------------------------------------
	float3 RefractionCorrection = float3(0.0f, 0.0f, 0.0f);
	if (WaterRefractionValid > 0.5f)
	{
		float2 RefractionOffset = WorldNormal.xz * WaterRefractionStrength * (0.25f + DepthFactor);
		float3 Background = SAMPLE_TEX2D(WaterRefractionColor, SamplerLinearClamp, InInput.ScreenUV).rgb;
		float3 Refracted = SAMPLE_TEX2D(WaterRefractionColor, SamplerLinearClamp, InInput.ScreenUV + RefractionOffset).rgb;
		// A missing copy reads black; do not let that subtract the whole background.
		float CopyLuma = dot(Refracted, float3(0.2126f, 0.7152f, 0.0722f));
		float CorrectionWeight = min((1.0f - OutSurface.Opacity) / max(OutSurface.Opacity, 0.001f), 4.0f);
		RefractionCorrection = (Refracted - Background) * CorrectionWeight * step(0.0001f, CopyLuma);
	}
	OutSurface.EmissiveColor = RefractionCorrection;
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

#endif	//M_WATER_DEBUG_HLSL
