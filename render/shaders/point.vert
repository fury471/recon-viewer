#version 450

layout(location = 0) in vec3 inPosition;   // from the buffer, attribute 0
layout(location = 1) in vec3 inColor;      // from the buffer, attribute 1

layout(location = 0) out vec3 fragColor;

void main() {
    gl_Position  = vec4(inPosition, 1.0);
    gl_PointSize = 8.0;          // draw each point as a visible 8-pixel dot
    fragColor    = inColor;
}