[numthreads(THREAD_GROUP_SIZE_X, THREAD_GROUP_SIZE_Y, 1)]
void CSMain(uint3 DispatchThreadID : SV_DispatchThreadID)
{
    uint2 TexCoord = DispatchThreadID.xy;

    // Sample input texture
    float4 InputColor = InputTexture.Load(int3(TexCoord, 0));

    // Apply color grading
    float3 Color = InputColor.rgb;
    Color = ApplyBrightness(Color, Brightness);
    Color = ApplyContrast(Color, Contrast);
    Color = ApplySaturation(Color, Saturation);
    Color = Color * ColorTint;
    Color = ApplyGamma(Color, Gamma);

    // Write output
    OutputTexture[TexCoord] = float4(Color, InputColor.a);
}
