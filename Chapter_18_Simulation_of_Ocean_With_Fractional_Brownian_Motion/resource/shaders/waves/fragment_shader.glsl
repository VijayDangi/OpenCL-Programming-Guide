#version 460 core

#define NORMALS_IN_FRAG_SHADER 1
#define PI 3.14159265358979323846

in vec3 o_world_pos;
in vec3 o_normal;
in vec3 o_view_vector;
in vec2 o_tex_coord;
in float o_height;

uniform samplerCube u_sky_map;
uniform sampler2D u_foam_texture;

uniform vec3 u_camera_position;

    // sun
uniform vec3 u_sun_direction;
uniform vec3 u_sun_color;

    // light
uniform vec3 u_light_color;

    // material
uniform vec3  u_ambient;
uniform vec3  u_diffuse_reflectance;
uniform vec3  u_specular_reflectance;
uniform float u_specular_normal_strength;
uniform float u_shininess;

    // fresnel
uniform vec4 u_fresnel_parameter;   // normal_strength, shininess, bias, strength
uniform vec3 u_fresnel_color;
uniform int u_use_environment_map;

    // wave tip
uniform vec3  u_tip_color;
uniform float u_tip_attenuation;

uniform float u_normal_strength = 1.0;


out vec4 o_frag_color;


void main()
{
    // code
    vec3 light_direction = -normalize( u_sun_direction);
    vec3 view_direction = normalize( u_camera_position - o_world_pos);
    vec3 halfway_direction = normalize( light_direction + view_direction);

        // normal calculation
    float height = o_height;
    vec3 normal = normalize( o_normal);

    normal.xz *= u_normal_strength;

        // foam
    vec2 texel_size = 1.0 / vec2( textureSize(u_foam_texture, 0));
    vec3 foam = vec3(0.0f);

    /*
        1   2   1
        2   4   2
        1   2   1
     */

    foam += textureLod( u_foam_texture, vec2( o_tex_coord.x + texel_size.x, o_tex_coord.y + texel_size.y), 2).rrr;
    foam += textureLod( u_foam_texture, vec2( o_tex_coord.x + texel_size.x, o_tex_coord.y - texel_size.y), 2).rrr;
    foam += textureLod( u_foam_texture, vec2( o_tex_coord.x - texel_size.x, o_tex_coord.y + texel_size.y), 2).rrr;
    foam += textureLod( u_foam_texture, vec2( o_tex_coord.x - texel_size.x, o_tex_coord.y - texel_size.y), 2).rrr;

    foam += 2.0 * textureLod( u_foam_texture, vec2( o_tex_coord.x + texel_size.x, o_tex_coord.y + 0), 2).rrr;
    foam += 2.0 * textureLod( u_foam_texture, vec2( o_tex_coord.x - texel_size.x, o_tex_coord.y + 0), 2).rrr;
    foam += 2.0 * textureLod( u_foam_texture, vec2( o_tex_coord.x +            0, o_tex_coord.y + texel_size.y), 2).rrr;
    foam += 2.0 * textureLod( u_foam_texture, vec2( o_tex_coord.x +            0, o_tex_coord.y - texel_size.y), 2).rrr;

    foam += 4.0 * textureLod( u_foam_texture, vec2( o_tex_coord.x +            0, o_tex_coord.y + 0), 2).rrr;


    foam = foam / 16.0f;

        // diffuse
    float N_dot_L = max( dot( light_direction, normal), 0.0);

    vec3 diffuse_reflectance = u_diffuse_reflectance / PI;
    vec3 diffuse = u_light_color * N_dot_L * diffuse_reflectance;
    

        // schlick fresnel
    float fresnel_normal_strength = u_fresnel_parameter.x;
    float fresnel_shininess = u_fresnel_parameter.y;
    float fresnel_bias = u_fresnel_parameter.z;
    float fresnel_strength = u_fresnel_parameter.w;

    vec3 fresnel_normal = normal;
    fresnel_normal.xz *= fresnel_normal_strength;
    fresnel_normal = normalize( fresnel_normal);
    float base = 1.0 - dot( view_direction, fresnel_normal);
    float exponential = pow( base, fresnel_shininess);
    float R = exponential + fresnel_bias * ( 1.0 - exponential);
    R *= fresnel_strength;
    vec3 fresnel = u_fresnel_color * R;

        // environment reflection
    if( u_use_environment_map == 1)
    {
        vec3 reflected_direction = reflect( -view_direction, normal);
        vec3 sky_color = texture( u_sky_map, reflected_direction).rgb;
        vec3 sun = u_sun_color * pow( max( dot( reflected_direction, light_direction), 0.0), 500.0);

        fresnel = sky_color.rgb * R;
        fresnel += sun * R;
    }

        // specular
    vec3 spec_normal = normal;
    spec_normal.xz *= u_specular_normal_strength;
    spec_normal = normalize( spec_normal);
    float spec = pow( max( dot( spec_normal, halfway_direction), 0.0), u_shininess) * N_dot_L;
    vec3 specular = u_light_color * u_specular_reflectance * spec;

        // schlick fresnel but again for specular
    base = 1.0 - max( dot( view_direction, halfway_direction), 0.0);
    exponential = pow( base, 5.0);
    R = exponential + fresnel_bias * ( 1.0 - exponential);

    specular *= R;


        // tip of wave
    vec3 tip_color = u_tip_color * pow( height, u_tip_attenuation);


    vec3 color = u_ambient + diffuse + specular + fresnel + tip_color + foam;

    // o_frag_color = vec4( o_tex_coord, 0.2, 1.0);
    // height /= u_wave_height;
    // o_frag_color = vec4( height, height, height, 1.0);
    o_frag_color = vec4( color, 1.0);
}

