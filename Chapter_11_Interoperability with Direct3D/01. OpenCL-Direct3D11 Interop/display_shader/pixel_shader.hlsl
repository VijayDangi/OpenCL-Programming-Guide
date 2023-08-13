cbuffer ConstantBuffer
{
    float4 u_color;
    int u_use_texture;
}

struct VS_OUT
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

Texture2D u_texture;
SamplerState u_sampler;

float4 main( VS_OUT input) : SV_TARGET
{
    // code
    float4 color = float4( 0.0, 0.0, 0.0, 0.0);

    if( u_use_texture == 1)
    {
        //color = float4( input.texcoord, 0.0, 1.0);
        color = u_texture.Sample( u_sampler, input.texcoord);
    }
    else
    {
        color = u_color;
    }

    return color;
}
