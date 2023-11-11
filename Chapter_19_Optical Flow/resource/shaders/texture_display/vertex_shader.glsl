#version 460 core

out vec2 o_texcoord;

void main()
{
    // code
    o_texcoord = vec2(
        (gl_VertexID << 1) & 2,
        gl_VertexID & 2
    );

    gl_Position = vec4( o_texcoord * 2.0 - 1.0, 0.0, 1.0);
}
