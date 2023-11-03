#version 460 core

// in vec2 a_position;

out vec2 o_texcoord;

void main()
{
    // code
    // const vec2 vertices[] = vec2[6]
    // (
    //     vec2(  1.0,  1.0),
    //     vec2( -1.0,  1.0),
    //     vec2( -1.0, -1.0),

    //     vec2(  1.0,  1.0),
    //     vec2( -1.0, -1.0),
    //     vec2(  1.0, -1.0)
    // );

    // gl_Position = vec4( vertices[gl_VertexID], 0.0, 1.0);

    // o_texcoord = vertices[gl_VertexID] * 0.5 + 0.5;

    o_texcoord = vec2(
        (gl_VertexID << 1) & 2,
        gl_VertexID & 2
    );

    gl_Position = vec4( o_texcoord * 2.0 - 1.0, 0.0, 1.0);
}
