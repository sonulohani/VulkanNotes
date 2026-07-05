#version 450

// Per-vertex inputs. These "locations" must match the
// VkVertexInputAttributeDescription entries we set up on the CPU side.
layout(location = 0) in vec2 inPosition;   // clip-space-ish 2D position
layout(location = 1) in vec3 inColor;      // per-vertex RGB colour

// Output handed to the fragment shader. The rasterizer interpolates it
// across the triangle's surface for us.
layout(location = 0) out vec3 fragColor;

void main() {
    // gl_Position is a vec4 in clip space. Our positions are already in the
    // [-1, 1] range, so z = 0 and w = 1 puts them straight onto the screen.
    gl_Position = vec4(inPosition, 0.0, 1.0);
    fragColor = inColor;
}
