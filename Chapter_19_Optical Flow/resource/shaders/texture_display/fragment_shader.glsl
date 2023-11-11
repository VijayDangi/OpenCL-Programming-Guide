#version 460 core

in vec2 o_texcoord;

uniform sampler2D u_texture_buffer;
uniform int u_use_vector_image;
uniform int u_flip_image;

out vec4 o_frag_color;

void main()
{
    // code
    vec4 color = vec4( 0.0, 0.0, 0.0, 1.0);
    vec2 tex_coord = o_texcoord;

    if( u_flip_image == 1)
    {
        tex_coord.y = 1.0 - tex_coord.y;
        tex_coord.x = 1.0 - tex_coord.x;
    }

    if( u_use_vector_image == 1)
    {
        color = texture( u_texture_buffer, tex_coord).rrrr;
    }
    else
    {
        color.rgb = texture( u_texture_buffer, tex_coord).rgb;
        color.a = 1.0;
    }

    o_frag_color = color; //vec4( o_texcoord, 0.0, 1.0);
}
