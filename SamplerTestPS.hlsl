Texture2D myTexture : register(t0);
SamplerState mySampler : register(s0);

struct VS_OUTPUT
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};

float4 main(VS_OUTPUT input) : SV_TARGET
{
    return myTexture.Sample(mySampler, input.uv);
}