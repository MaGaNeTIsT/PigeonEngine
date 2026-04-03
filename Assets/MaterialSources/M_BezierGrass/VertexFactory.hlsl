Varying VertexFactoryMain(Attribute Input, uint InstanceID : SV_InstanceID)
{
    Varying Output;
    Output.Pos = float4(0, 0, 0, 1);
    Output.Normal = float3(0, 0, 1);
    Output.Tangent = float3(1, 0, 0);
    Output.UV = float2(0, 0);
    Output.WorldPos = float3(0, 0, 0);
    return Output;
}
