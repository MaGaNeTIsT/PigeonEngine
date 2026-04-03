{MATERIAL_CB_DECL}

{MATERIAL_TEXTURE_DECL}

{MATERIAL_CS_FUNCTIONS}

{MATERIAL_SURFACE_CODE}

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    PostProcessMain(DTid);
}
