#version 450

// Interpolated colour coming from the vertex shader (matches location 0 there).
layout(location = 0) in vec3 fragColor;

// The final colour written to the swapchain image (attachment 0).
layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(fragColor, 1.0);
}
