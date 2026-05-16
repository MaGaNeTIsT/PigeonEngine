// Color grading helper functions

float3 ApplyBrightness(float3 color, float brightness)
{
    return color * brightness;
}

float3 ApplyContrast(float3 color, float contrast)
{
    return (color - 0.5) * contrast + 0.5;
}

float3 ApplySaturation(float3 color, float saturation)
{
    float luminance = dot(color, float3(0.299, 0.587, 0.114));
    return lerp(float3(luminance, luminance, luminance), color, saturation);
}

float3 ApplyGamma(float3 color, float gamma)
{
    return pow(max(color, 0.0), 1.0 / gamma);
}
