void CalcSurface(in Varying v, out SurfaceOutput s)
{
    float4 albedo = BaseColorMap.Sample(sLinearWrap, v.UV) * BaseColorTint;
    float3 orm    = ORMMap.Sample(sLinearWrap, v.UV).rgb;

    float3 worldNormal = ApplyTBNNormal(NormalMap.Sample(sLinearWrap, v.UV).xyz, v.Normal, v.Tangent, NormalScale);

    s.BaseColor = albedo.rgb;
    s.Normal    = normalize(worldNormal);
    s.Roughness = orm.g * RoughnessScale;
    s.Metallic  = orm.b * MetallicScale;
}
