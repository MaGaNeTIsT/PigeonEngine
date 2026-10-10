#ifndef M_ENGINE_NEUTRAL_VERTEX_HLSL
#define M_ENGINE_NEUTRAL_VERTEX_HLSL

// ---------------------------------------------------------------------------
// The shared neutral vertex material.
//
// Referenced in place of a material by the vertex stage of every material that
// does not touch vertices. It is a replacement, not an addition: the program
// description lists this component as the material for the vertex stage and the
// real material for the pixel stage, so the vertex shader of every such material
// is the same compiled shader, and the number of vertex variants stops depending
// on the number of materials.
//
// It declares no parameters and no textures. A vertex stage that did nothing
// with them would cull them anyway, so the honest thing is not to ask for them.
//
// VertexOps in the material asset is what decides this: false routes the vertex
// stage here.
// ---------------------------------------------------------------------------

#if SHADER_STAGE_VERTEX
void CalcMaterialVertex(in MaterialVertexInput InInput, out float3 OutWorldPositionOffset, inout VFVarying InOutVarying)
{
	OutWorldPositionOffset = float3(0.0, 0.0, 0.0);
}
#endif	//SHADER_STAGE_VERTEX
#endif	//M_ENGINE_NEUTRAL_VERTEX_HLSL
