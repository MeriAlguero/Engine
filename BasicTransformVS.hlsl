cbuffer ConstantData : register(b0)
{
    float4x4 mvp;
};

struct VS_INPUT
{
    float3 pos : MY_POSITION;
};

struct VS_OUTPUT
{
    float4 pos : SV_POSITION;
};

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;
    // Multiplicamos la posición del vértice por la matriz MVP
    output.pos = mul(float4(input.pos, 1.0f), mvp);
    return output;
}