#version 410 core

in vec2 o_texcoord;

uniform int u_use_texture;
uniform sampler2D u_texture_d;
uniform vec4 u_color;

out vec4 o_frag_color;

void main( void)
{
    // code
    vec4 color = vec4( 0.0);

    if( u_use_texture == 1)
    {
        //color = vec4( o_texcoord, 0.0, 1.0);
        color = texture( u_texture_d, o_texcoord);
    }
    else
    {
        color = u_color;
    }

    o_frag_color = color;
}
