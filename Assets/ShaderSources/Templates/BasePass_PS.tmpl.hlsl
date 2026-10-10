#ifndef BASE_PASS_PS_TMPL_HLSL
#define BASE_PASS_PS_TMPL_HLSL

// ---------------------------------------------------------------------------
// Base pass, pixel stage.
//
// Derive but do not shade. The material produces a surface, the domain turns it
// into BRDF parameters, and the encode step writes both the parameters and the
// indirect light into the render targets. The direct lighting is the deferred
// lighting pass's job, which is what keeps this pass independent of the light
// count.
//
// The surface mode decides coverage here, and only here: Mask discards, Opaque
// ignores alpha. Translucent is not this pass at all, it is the forward pass, so
// the template does not carry a branch for it.
//
// GBufferD is an unsigned integer target, so its output parameter is uint4 and
// not float4; the R16G16B16A16_UINT format is what makes the identity and the
// light mask free of any float packing.
// ---------------------------------------------------------------------------

#include "Public/GBuffer/ShaderGBufferEncode.hlsl"

{SYSTEM_MACROS}
{VF_VARYING_STRUCT}
{DRAW_PARAMS}
{VF_VERTEX_FACTORY}
{PROGRAMMABLE}
{LIGHTING}

void PSMain(in VFVarying InVarying,
	in uint InPrimitiveID : SV_PrimitiveID,
	in bool InIsFrontFace : SV_IsFrontFace,
	out float4 OutSceneColor : SV_Target0,
	out float4 OutGBufferA : SV_Target1,
	out float4 OutGBufferB : SV_Target2,
	out float4 OutGBufferC : SV_Target3,
	out uint4 OutGBufferD : SV_Target4,
	out float4 OutGBufferE : SV_Target5,
	out float4 OutGBufferF : SV_Target6)
{
	MaterialPixelInput MatInput = BuildMaterialPixelInput(InVarying, ViewPosition.xyz, InPrimitiveID, InIsFrontFace);

	MaterialSurface Material;
	SurfaceData Surface;
	GBufferOutput GBuffer;

	EvaluateMaterialSurface(MatInput, Material);
	DeriveSurface(Material, MatInput, Surface);

#if MATERIAL_SURFACE_MODE == MATERIAL_SURFACE_MODE_MASK
	if (IsAlphaDiscarded(Surface.Opacity, Surface.AlphaCutoff))
	{
		discard;
	}
#endif	//MATERIAL_SURFACE_MODE == MATERIAL_SURFACE_MODE_MASK

	// The domain's write. Which channel carries what is decided in
	// Public/GBuffer/, so nothing here has to know.
	EncodeGBuffer(Surface, MATERIAL_DOMAIN, MATERIAL_SURFACE_MODE, GBuffer);

	OutSceneColor = GBuffer.SceneColor;
	OutGBufferA = GBuffer.GBufferA;
	OutGBufferB = GBuffer.GBufferB;
	OutGBufferC = GBuffer.GBufferC;
	OutGBufferD = GBuffer.GBufferD;
	OutGBufferE = GBuffer.GBufferE;
	OutGBufferF = GBuffer.GBufferF;
}
#endif	//BASE_PASS_PS_TMPL_HLSL
