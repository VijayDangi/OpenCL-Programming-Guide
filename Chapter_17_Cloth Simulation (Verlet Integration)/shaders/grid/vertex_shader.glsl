#version 410 core

in vec4 vPosition;

uniform mat4 MVPMatrix;
uniform mat4 MVMatrix;
uniform vec4 Color;
uniform float MaxDistance;

out vec4 out_color;
out vec4 out_viewPosition;
out float out_maxDistance;

void main()
{
    out_color = Color;
    out_viewPosition = MVMatrix * vPosition;
    out_maxDistance = MaxDistance;

    gl_Position = MVPMatrix * vPosition;
}
