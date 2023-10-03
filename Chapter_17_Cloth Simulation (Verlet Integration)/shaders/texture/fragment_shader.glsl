#version 450 core

in vec2 o_texcoord;
in vec3 o_normal;

uniform sampler2D u_texture;

out vec4 o_frag_color;

void main()
{
    o_frag_color = vec4( texture( u_texture, o_texcoord).rgb, 1.0);
}
