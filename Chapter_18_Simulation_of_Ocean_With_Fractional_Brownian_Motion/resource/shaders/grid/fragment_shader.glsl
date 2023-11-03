#version 410 core

in vec4 out_color;
in vec4 out_viewPosition;
in float out_maxDistance;

out vec4 fragColor;

void main()
{
    float blendColor = 0.5f - length( out_viewPosition.xyz) / out_maxDistance;
    fragColor = vec4( out_color.xyz, blendColor);
}
