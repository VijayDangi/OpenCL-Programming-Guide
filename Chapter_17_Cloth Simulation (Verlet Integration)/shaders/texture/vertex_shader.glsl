#version 450 core

in vec4 a_position;
in vec3 a_normal;
in vec2 a_texcoord;

uniform mat4 u_model_matrix;
uniform mat4 u_view_matrix;
uniform mat4 u_projection_matrix;

out vec2 o_texcoord;
out vec3 o_normal;

void main( void)
{
    o_texcoord = a_texcoord;
    o_normal = a_normal;
    gl_Position = u_projection_matrix * u_view_matrix * u_model_matrix * a_position;
}
