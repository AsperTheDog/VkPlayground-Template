#version 450 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;

layout(location = 0) out vec3 color;

void main()
{
    gl_Position = vec4(aPos, 1.0);
    color = aColor;
}