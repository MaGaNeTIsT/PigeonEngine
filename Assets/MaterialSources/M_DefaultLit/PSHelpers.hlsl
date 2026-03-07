float3 ApplyTBNNormal(float3 sample, float3 N, float3 T, float scale)
{
    float3 B = cross(N, T);
    float3 n = sample * 2.0 - 1.0;
    return normalize(n.x * T + n.y * B + n.z * N) * scale + N * (1.0 - scale);
}
