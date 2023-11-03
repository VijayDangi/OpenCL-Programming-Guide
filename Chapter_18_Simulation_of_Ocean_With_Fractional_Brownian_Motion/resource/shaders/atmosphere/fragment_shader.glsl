#version 460 core

in vec2 o_texcoord;

uniform sampler2D u_color_buffer;
uniform sampler2D u_depth_buffer;
uniform samplerCube u_sky_map;

uniform vec2 u_texel_size;
uniform float u_time;

    // camera
uniform float u_near_plane;
uniform float u_far_plane;

uniform mat4 u_view;
uniform mat4 u_projection;

uniform vec3 u_camera_position;

    // fog
uniform float u_fog_density;
uniform float u_fog_offset;
uniform vec3 u_fog_color;

uniform float u_fog_height;
uniform float u_fog_attenuation;

    // sun
uniform vec3 u_sun_direction;
uniform vec3 u_sun_color;
uniform float u_sun_exponetial;

    // skyboxs
uniform vec3 u_sky_box_direction = vec3( 0.0, -1.0, 0.0);
uniform float u_sky_box_speed = 0.1;

out vec4 o_frag_color;

float Linear01Depth( float z)
{
    float Zx = 1.0 - u_far_plane / u_near_plane;
    float Zy = u_far_plane / u_near_plane;
    return 1.0 / ( Zx * z + Zy);
}

vec3 ComputeWorldSpacePosition( vec2 ndc_position, float depth, in mat4 inverseViewProjMatrix)
{
    vec4 positionCS = vec4( ndc_position * 2.0 - 1.0, depth, 1.0);
    vec4 positionWS = inverseViewProjMatrix * positionCS;
    return positionWS.xyz / positionWS.w;
}

vec4 flowUVW( vec3 direction, vec3 curl, float t, bool flowB)
{
    float phaseOffset = flowB ? 0.5 : 0.0;
    float progress = t + phaseOffset - floor( t + phaseOffset);
    vec3 offset = curl * progress;

    vec4 uvw = vec4( direction, 0.0);
    uvw.xz -= offset.xy;
    uvw.w = 1 - abs( 1.0 - 2.0 * progress);

    return uvw;
}

void main()
{
    // code
    mat4 viewProjectionMatrix = u_projection * u_view;
    mat4 inverseViewProjMatrix = inverse( viewProjectionMatrix);

    vec4 color = texture( u_color_buffer, o_texcoord);
    float depth = texture( u_depth_buffer, o_texcoord).r;

    vec3 world_position = ComputeWorldSpacePosition( o_texcoord, depth, inverseViewProjMatrix);
    vec3 view_direction = normalize( u_camera_position - world_position);

        // sky
    vec3 curl = normalize( u_sky_box_direction);
    float t = u_time * u_sky_box_speed;

    vec4 uvw1 = flowUVW( -view_direction, curl, t, false);
    vec4 uvw2 = flowUVW( -view_direction, curl, t, true);

    vec3 sky = texture( u_sky_map, uvw1.xyz).rgb * uvw1.w;
    vec3 sky2 = texture( u_sky_map, uvw2.xyz).rgb * uvw2.w;

    sky = ( sky + sky2);

    if( depth == 1)
    {
        color.rgb = sky;
    }

    depth = Linear01Depth(depth);
    float viewDistance = depth * u_far_plane;

    // sun
    vec3 sun_dir = normalize( u_sun_direction);
    vec3 sun = u_sun_color * pow( max( dot( view_direction, sun_dir), 0.0), u_sun_exponetial) * depth;

    // fog height attenuation
    float height = min( u_fog_height, world_position.y) / u_fog_height;
    height = pow( clamp( height, 0.0, 1.0), 1.0 / u_fog_attenuation);

    // fog color
    float fogFactor = ( u_fog_density / sqrt( log(2))) * max( 0.0f, viewDistance - u_fog_offset);
    fogFactor = exp2( -fogFactor * fogFactor);
    
    vec3 out_color = mix( u_fog_color, color.rgb, clamp( height + fogFactor, 0.0, 1.0));

    o_frag_color = vec4( out_color + sun, 1.0);
}
