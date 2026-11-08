#ifndef WATER_SURFACE_VS_TMPL_HLSL
#define WATER_SURFACE_VS_TMPL_HLSL

// ---------------------------------------------------------------------------
// Water surface pass, vertex stage.
//
// The translucent pass shape plus the water simulation declarations. A water
// material's CalcMaterialVertex reads WaterHeightWetTex and displaces the mesh
// along world Y; the simulation plane is world XZ.
// ---------------------------------------------------------------------------

{SYSTEM_MACROS}
{WATER_SYSTEM}
{VF_VARYING_STRUCT}
{DRAW_PARAMS}
{VF_VERTEX_FACTORY}
{PROGRAMMABLE}

void VSMain(in VFAttributes InAttributes,
	in uint InVertexID : SV_VertexID,
	in uint InInstanceID : SV_InstanceID,
	out VFVarying OutVarying)
{
	VFContext Context;
	Context.VertexID = InVertexID;
	Context.InstanceID = InInstanceID;
	Context.PrimitiveID = 0;

	{VF_VS_BODY}
}

#endif	//WATER_SURFACE_VS_TMPL_HLSL
