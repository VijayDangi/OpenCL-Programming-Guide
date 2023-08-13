Texture2D u_texture;
SamplerState u_sampler
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Wrap;
    AddressV = Wrap;
};

cbuffer ConstantBuffer
{
    float4 u_color;
    int4 u_use_texture;
}


struct PS_INPUT
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

    // vertex shader
PS_INPUT vs_main( float2 position : POSITION)
{
    // code
    PS_INPUT output = (PS_INPUT)0;

    output.texcoord = position * 0.5 + 0.5;
    output.position = float4( position, 0.0, 1.0);

    return output;
}

    // pixel shader
float4 ps_main( PS_INPUT input) : SV_TARGET
{
    // code
    float4 color = float4( 0.0, 0.0, 0.0, 0.0);

    if( u_use_texture[0] == 1)
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

technique11 Render
{
    pass p0
    {
        SetVertexShader( CompileShader( vs_5_0, vs_main()));
        SetGeometryShader( NULL);
        SetPixelShader( CompileShader( ps_5_0, ps_main()));
    }
}
