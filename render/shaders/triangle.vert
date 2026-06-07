#version 450

// The three corners, hardcoded in Vulkan clip space.
// (Vulkan's Y points DOWN, so -0.5 is the upper half of the window.)
vec2 positions[3] = vec2[](
    vec2( 0.0, -0.5),   // top
    vec2( 0.5,  0.5),   // bottom-right
    vec2(-0.5,  0.5)    // bottom-left
);

// One color per corner; the GPU blends them across the triangle.
vec3 colors[3] = vec3[](
    vec3(1.0, 0.0, 0.0),   // red
    vec3(0.0, 1.0, 0.0),   // green
    vec3(0.0, 0.0, 1.0)    // blue
);

layout(location = 0) out vec3 fragColor;   // hand-off to the fragment shader

void main() {
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    fragColor   = colors[gl_VertexIndex];
}