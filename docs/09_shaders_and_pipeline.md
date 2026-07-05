# 09 · Shaders & the Graphics Pipeline

This is the heart of "how the triangle gets drawn". Two parts: the **shaders**
(small programs that run on the GPU) and the **pipeline** (everything else,
frozen into one object).

---

## Part 1 · Shaders

A **shader** is a program that runs on the GPU. The graphics pipeline runs two
of them for us:

1. **Vertex shader** — runs once per vertex. Its job is to output the final
   position of each vertex (and pass data to the next stage).
2. **Fragment shader** — runs once per pixel ("fragment") covered by the
   triangle. Its job is to output that pixel's color.

Between them, the fixed-function **rasterizer** turns the triangle's three
positioned vertices into the set of pixels it covers, and *interpolates* the
per-vertex outputs across those pixels.

### The vertex shader (`triangle.vert`)

```glsl
#version 450
layout(location = 0) in vec2 inPosition;   // from the vertex buffer
layout(location = 1) in vec3 inColor;
layout(location = 0) out vec3 fragColor;   // to the fragment shader

void main() {
    gl_Position = vec4(inPosition, 0.0, 1.0);
    fragColor = inColor;
}
```

- `in` variables are **vertex attributes** — they come from our vertex buffer.
  The `location` numbers must match the
  `VkVertexInputAttributeDescription`s in C++.
- `gl_Position` is the built-in output: the vertex's **clip-space** position as a
  `vec4`. Our positions are already in [-1, 1], so z=0, w=1 is fine.
- We pass the color out to `fragColor` (location 0 of the *outputs*).

### The fragment shader (`triangle.frag`)

```glsl
#version 450
layout(location = 0) in vec3 fragColor;    // interpolated from the 3 vertices
layout(location = 0) out vec4 outColor;    // final pixel color (attachment 0)

void main() {
    outColor = vec4(fragColor, 1.0);
}
```

The magic of the smooth color gradient is **interpolation**: the rasterizer
blends the three corner colors across the triangle automatically, so a pixel in
the middle gets a mix of red, green, and blue.

### GLSL → SPIR-V

GPUs don't read GLSL text. We compile it to **SPIR-V** (a binary intermediate
language) with `glslc` at build time (see doc 01). At runtime we load the `.spv`
bytes and wrap them in a `VkShaderModule`:

```cpp
VkShaderModuleCreateInfo info{};
info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
info.codeSize = code.size();                          // in bytes
info.pCode = reinterpret_cast<const uint32_t*>(code.data());
vkCreateShaderModule(device, &info, nullptr, &module);
```

A shader module is a thin wrapper around the bytecode; it's only needed while
building the pipeline and can be destroyed right after.

---

## Part 2 · The Graphics Pipeline

In OpenGL you flip GPU state (blend on/off, depth test, etc.) whenever you like.
Vulkan bakes **almost all** of that state into one immutable object: the
`VkPipeline`. This is a big up-front struct, but it means the driver can fully
optimize it once, and switching pipelines at draw time is cheap.

Let's walk through each piece we configure.

### Shader stages

```cpp
VkPipelineShaderStageCreateInfo vertStage{ ..., .stage = VK_SHADER_STAGE_VERTEX_BIT,
    .module = vertModule, .pName = "main" };
VkPipelineShaderStageCreateInfo fragStage{ ..., .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
    .module = fragModule, .pName = "main" };
```

`pName = "main"` is the entry-point function name inside the shader.

### Vertex input — how a `Vertex` maps to shader inputs

This connects our C++ `Vertex` struct to the shader's `in` variables.

```cpp
// One binding: the whole per-vertex struct, advanced once per vertex.
VkVertexInputBindingDescription binding{};
binding.binding = 0;
binding.stride = sizeof(Vertex);
binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

// Two attributes: pos and color, with their formats and byte offsets.
attrs[0] = { .location=0, .binding=0, .format=VK_FORMAT_R32G32_SFLOAT,    .offset=offsetof(Vertex,pos) };
attrs[1] = { .location=1, .binding=0, .format=VK_FORMAT_R32G32B32_SFLOAT, .offset=offsetof(Vertex,color) };
```

- **binding** = which buffer, and how big each element is (`stride`).
- **attribute** = a field inside that element: its shader `location`, its
  `format` (e.g. `R32G32_SFLOAT` = two 32-bit floats = `vec2`), and its byte
  `offset` in the struct.

`offsetof` and `sizeof` make this robust to struct layout changes.

### Input assembly — how vertices form primitives

```cpp
inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
```

`TRIANGLE_LIST` means "every 3 vertices make one triangle". We have exactly 3.

### Viewport & scissor — DYNAMIC here

The **viewport** maps clip space to pixels; the **scissor** clips drawing to a
rectangle. We declare them **dynamic**, meaning we'll set them at draw time
instead of baking them in:

```cpp
std::array<VkDynamicState,2> dyn = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
```

Why dynamic? So a window resize doesn't force us to rebuild the whole pipeline —
we just call `vkCmdSetViewport`/`vkCmdSetScissor` with the new size each frame.

### Rasterizer

```cpp
raster.polygonMode = VK_POLYGON_MODE_FILL;         // fill triangles (vs wireframe)
raster.lineWidth = 1.0f;
raster.cullMode = VK_CULL_MODE_BACK_BIT;           // skip back-facing triangles
raster.frontFace = VK_FRONT_FACE_CLOCKWISE;        // clockwise = front
```

**Culling** discards triangles facing away from the viewer for efficiency. We
declare clockwise winding as "front". (Our triangle is wound so it shows; if it
ever disappears, culling/winding is the usual suspect.)

### Multisampling

```cpp
multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;  // MSAA off
```

Anti-aliasing is disabled to keep things simple.

### Color blending

```cpp
blendAttachment.colorWriteMask = R|G|B|A;   // write all channels
blendAttachment.blendEnable = VK_FALSE;      // no alpha blending; just overwrite
```

Blending would mix new pixels with what's already there (for transparency). We
just overwrite.

### Pipeline layout

Declares the **descriptor sets** (bound resources like textures/uniforms) and
**push constants** a pipeline uses. Our triangle uses none, so it's empty:

```cpp
VkPipelineLayoutCreateInfo layoutInfo{ .sType = ... };  // all zero/empty
vkCreatePipelineLayout(device, &layoutInfo, nullptr, &pipelineLayout);
```

### Putting it all together

```cpp
VkGraphicsPipelineCreateInfo pipelineInfo{};
pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
pipelineInfo.stageCount = 2;                pipelineInfo.pStages = stages;
pipelineInfo.pVertexInputState   = &vertexInput;
pipelineInfo.pInputAssemblyState = &inputAssembly;
pipelineInfo.pViewportState      = &viewportState;
pipelineInfo.pRasterizationState = &raster;
pipelineInfo.pMultisampleState   = &multisample;
pipelineInfo.pColorBlendState    = &colorBlend;
pipelineInfo.pDynamicState       = &dynamicState;
pipelineInfo.layout     = pipelineLayout;
pipelineInfo.renderPass = renderPass;   // pipeline is tied to a render pass
pipelineInfo.subpass    = 0;
vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr,
                          &graphicsPipeline);
```

Notice the pipeline references the **render pass** — a pipeline is compiled for a
specific render pass/subpass. Once built, we destroy the shader modules; the
pipeline has everything it needs.

That single `graphicsPipeline` object now encapsulates *both shaders plus every
fixed setting above*. Binding it at draw time is one cheap call.

Next: [`10_framebuffers_and_commands.md`](10_framebuffers_and_commands.md).
