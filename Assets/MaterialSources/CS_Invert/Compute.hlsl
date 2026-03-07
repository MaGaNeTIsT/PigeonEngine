void PostProcessMain(uint3 DTid)
{
    float4 color = InputTex.Load(int3(DTid.xy, 0));
    OutputTex[DTid.xy] = float4(InvertColor(color.rgb), color.a);
}
