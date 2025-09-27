struct VSInput
{
    float3 position : POSITION;
    float3 color;
}

struct VSOutput
{
    float4 position : SV_Position;
    float4 color;
};

struct PushData
{
    float4x4 modelMatrix;
    float4x4 viewProjMatrix;
};
[[vk::push_constant]] PushData pc;

[shader("vertex")]
VSOutput main(VSInput input)
{
    VSOutput output;

    float4 worldPos = mul(float4(input.position, 1.0), pc.modelMatrix);
    output.position = mul(worldPos, pc.viewProjMatrix);
    output.color = float4(input.color, 1.0);
    return output;
}

[shader("fragment")]
float4 main(VSOutput input) : SV_Target
{
    return input.color;
}