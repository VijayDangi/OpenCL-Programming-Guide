struct VS_OUT
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

VS_OUT main( float2 position : POSITION)
{
    // code
    VS_OUT output = (VS_OUT)0;

    output.texcoord = position * 0.5 + 0.5;
    output.position = float4( position, 0.0, 1.0);

    return output;
}

