cbuffer MVPBuffer : register(b0)
{
    float4x4 mvp;
};

struct VertexInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 texcoord : TEXCOORD;
};

struct PixelInput
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float2 texcoord : TEXCOORD;
};

PixelInput main(VertexInput input)
{
    PixelInput output;
    
    // Transform position
    output.position = mul(float4(input.position, 1.0f), mvp);
    
    // Pass through normal and texcoord
    output.normal = input.normal;
    output.texcoord = input.texcoord;
    
    return output;
}