Texture2D myTexture : register(t0);
SamplerState mySampler : register(s0);

struct VS_OUTPUT
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};

float4 main(VS_OUTPUT input) : SV_TARGET
{
    //return myTexture.Sample(mySampler, input.uv);
    return float4(0.0f, 0.7f, 0.7f, 1.0f);
}