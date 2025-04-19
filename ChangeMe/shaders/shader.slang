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

[shader("vertex")]
VSOutput main(VSInput input)
{
    VSOutput output;
    output.position = float4(input.position, 1.0);
    output.color = float4(input.color, 1.0);
    return output;
}

[shader("fragment")]
float4 main(VSOutput input) : SV_Target
{
    return input.color;
}