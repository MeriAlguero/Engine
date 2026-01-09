cbuffer MaterialBuffer : register(b1)
{
    float4 baseColor;
    bool hasTexture;
};

struct PixelInput
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float2 texcoord : TEXCOORD;
};

float4 main(PixelInput input) : SV_TARGET
{
    // Simple flat color with basic lighting
    float3 lightDir = normalize(float3(1.0f, 1.0f, 1.0f));
    float3 normal = normalize(input.normal);
    float diff = max(dot(normal, lightDir), 0.0f);
    float ambient = 0.3f;
    
    return float4(baseColor.rgb * (ambient + diff), baseColor.a);
}