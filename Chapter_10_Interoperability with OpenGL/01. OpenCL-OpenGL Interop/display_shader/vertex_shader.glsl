#version 410 core

layout( location = 0) in vec2 a_position;

uniform int u_use_texture;

out vec2 o_texcoord;

void main( void)
{
    // code
    if( u_use_texture == 1)
    {
        o_texcoord = a_position * 0.5 + 0.5;
    }

    gl_Position = vec4( a_position, 0.0, 1.0);
}
