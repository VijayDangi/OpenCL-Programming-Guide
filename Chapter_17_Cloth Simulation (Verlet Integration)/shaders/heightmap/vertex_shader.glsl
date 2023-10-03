#version 450 core

in vec4 a_position;
in vec2 a_texcoord;

uniform mat4 u_model_matrix;
uniform mat4 u_view_matrix;
uniform mat4 u_projection_matrix;

uniform sampler2D u_texture;
uniform float max_height;

out vec2 o_texcoord;

void main( void)
{
    o_texcoord = a_texcoord;

    float height = texture( u_texture, o_texcoord).r * max_height;

    vec4 world_position = u_model_matrix * a_position;
    world_position.y += height;
    
    gl_Position = u_projection_matrix * u_view_matrix * world_position;


  /*  float d = length( a_position.xyz);
    float y = max_height * sin( -3.14159 * d * 4);
    gl_Position = u_projection_matrix * u_view_matrix * u_model_matrix * vec4( a_position.x, y, a_position.z, 1.0);*/
}
