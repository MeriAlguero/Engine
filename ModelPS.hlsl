cbuffer MaterialBuffer : register(b1)
{
    float4 baseColor;
    bool hasTexture;
};

Texture2D colorTexture : register(t0);
SamplerState colorSampler : register(s0);

struct PixelInput
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float2 texcoord : TEXCOORD;
};

float4 main(PixelInput input) : SV_TARGET
{
    float4 finalColor = baseColor;
    
    // Apply texture if available
    if (hasTexture)
    {
        float4 texColor = colorTexture.Sample(colorSampler, input.texcoord);
        finalColor *= texColor;
    }
    
    // Simple lighting (ambient + directional)
    float3 lightDir = normalize(float3(1.0f, 1.0f, 1.0f));
    float3 normal = normalize(input.normal);
    float diff = max(dot(normal, lightDir), 0.0f);
    float ambient = 0.3f;
    
    finalColor.rgb *= (ambient + diff);
    
    return finalColor;
}