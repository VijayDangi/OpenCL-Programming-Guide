#version 460 core

in vec3 a_world_pos;
in vec3 a_normal;
in vec2 a_tex_coord;

out vec3 o_world_pos;
out vec3 o_normal;
out vec3 o_view_vector;
out vec2 o_tex_coord;
out float o_height;

uniform mat4 u_projection;
uniform mat4 u_view;


/**

 y = a * sin(x)

    -------------------------------
 freq = 2 / Wavelength

 y = a * sin(freq * x)

    -------------------------------
    phase = speed * 2/Wavelength

y = a * sin(freq * x + time * phase)

*/

/**/
void main()
{
    vec3 position = a_world_pos;
    o_height = position.y;

    vec4 eye_coord = u_view * vec4( position, 1.0);
    gl_Position = u_projection * eye_coord;

    o_view_vector = -eye_coord.xyz / eye_coord.w;

    o_normal = a_normal;

    o_world_pos = position;
    o_tex_coord = a_tex_coord;
}

