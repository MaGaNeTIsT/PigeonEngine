#ifndef WATER_REFRACTION_COPY_PS_TMPL_HLSL
#define WATER_REFRACTION_COPY_PS_TMPL_HLSL

// ---------------------------------------------------------------------------
// Water refraction background copy, pixel stage.
//
// Unlike Copy_PS this is a plain linear copy: the water material samples the result in the
// same frame, so the values have to stay in scene referred luminance and must not have the
// display transform (exposure, tone map, transfer function) applied.
// ---------------------------------------------------------------------------

#include "Public/System/ShaderBindless.hlsl"

{SYSTEM_MACROS}
{VF_VARYING_STRUCT}

void PSMain(in VFVarying InVarying,
	out float4 OutTarget : SV_Target0)
{
	OutTarget = PE_SAMPLE_TEX2D(CopySourceTexture, SamplerLinearClamp, InVarying.UV0);
}

#endif	//WATER_REFRACTION_COPY_PS_TMPL_HLSL
