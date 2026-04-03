void CalcSurface(in Varying Input, out SurfaceOutput Output)
{
    Output.BaseColor = lerp(RootColor.rgb, TipColor.rgb, Input.UV.y);
    Output.Normal = normalize(Input.Normal);
    Output.Roughness = Roughness;
    Output.Metallic = Metallic;
}
