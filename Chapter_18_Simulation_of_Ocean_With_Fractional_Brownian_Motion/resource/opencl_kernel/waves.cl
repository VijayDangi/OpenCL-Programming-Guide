
struct WaveProperty
{
    float2 u_wave_direction_seed_param; // = (float2)( 0, 1253.2131);  // seed, seed_iter
    float2 u_wave_frequency_param; // = (float2)( 1.0, 1.18);          // frequency, frequency_multiplier
    float2 u_wave_amplitude_param; // = (float2)( 1.0, 0.82);          // amplitude, amplitude_multiplier
    float2 u_wave_speed_param; // = (float2)( 2.0, 1.07);              // initial_speed, speed_ramp
    float u_wave_drag; // = 1.0;
    float u_wave_height; // = 1.0;
    float u_wave_max_peak; // = 1.0;
    float u_wave_peak_offset; // = 1.0;
    int u_wave_count;
};


const sampler_t sampler_ = CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;

__kernel void fbm(
    __global float4 *vertices_in_out,
    __global float4 *normal_out,
    __global struct WaveProperty *waveProperty,
    read_only image2d_t foam_read_texture,
    write_only image2d_t foam_write_texture,
    int texture_size,
    int size,
    float time_inteval
)
{
    int index = get_global_id(0);
    if( index >= size)
    {
        return;
    }

    int x = index % texture_size;
    int y = index / texture_size;

    float f = waveProperty->u_wave_frequency_param.x;
    float a = waveProperty->u_wave_amplitude_param.x;
    float speed = waveProperty->u_wave_speed_param.x;
    float seed = waveProperty->u_wave_direction_seed_param.x;

    float3 p = vertices_in_out[index].xyz;
    float amplitudeSum = 0.0f;

    float h = 0.0;
    float2 n = (float2)( 0.0, 0.0);
    float2 d;

    for( int wi = 0; wi < waveProperty->u_wave_count; ++wi)
    {
        d = normalize( (float2)( cos(seed), sin(seed)));

        float x = dot( d, p.xz) * f + time_inteval * speed;
        float wave = a * exp( waveProperty->u_wave_max_peak * sin(x) - waveProperty->u_wave_peak_offset);
        float2 dw = f * d * ( waveProperty->u_wave_max_peak * wave * cos(x));

        h += wave;
        p.xz += -dw * a * waveProperty->u_wave_drag;

        n += dw;

        amplitudeSum += a;

        f *= waveProperty->u_wave_frequency_param.y;
        a *= waveProperty->u_wave_amplitude_param.y;
        speed *= waveProperty->u_wave_speed_param.y;
        seed += waveProperty->u_wave_direction_seed_param.y;
    }

    float3 v = (float3) ( h, n);
    v = v / amplitudeSum;
    float foam_height = v.x;
    v.x = v.x * waveProperty->u_wave_height;

    float3 normal = normalize( (float3)( -v.y, 1.0f, -v.z));
    normal_out[index].xyz = normal;

    vertices_in_out[index].y = v.x;

    float foam = read_imagef( foam_read_texture, (int2)(x + 1, y + 1)).r;
    // foam += read_imagef( foam_read_texture, (int2)(x + d.x, y + d.y)).r;


    foam = foam * exp( -0.5);

    if( foam_height > 0.6)
    {
        foam = foam_height - 0.5 + foam;
    }

    write_imagef( foam_write_texture, (int2) (x, y), (float4)( foam, 0.0, 0.0, 1.0));
}