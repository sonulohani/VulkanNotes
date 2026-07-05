# 01 · Hello Triangle (Vulkan)

A fully-documented Vulkan program that draws one colored triangle, built to
**teach**. It uses modern, practical tooling:

- **Vulkan** — the graphics API
- **volk** — fast Vulkan function loading
- **VMA** (Vulkan Memory Allocator) — GPU memory management
- **GLFW** — window & surface
- **GLM** — math types for vertex data
- **glslc** — GLSL → SPIR-V shader compilation

The code lives in `src/`, but the **real content is the step-by-step guide in
[`docs/`](docs/)**. Every Vulkan concept is explained from scratch, in order.

![triangle](https://raw.githubusercontent.com/KhronosGroup/Vulkan-Docs/main/appendices/images/vulkantriangle.png)
<sub>(illustrative — your window shows a red/green/blue blended triangle)</sub>

---

## Read the docs (start here)

Go through them in order — each builds on the last:

1. [The Big Picture](docs/00_overview.md) — mental model, why Vulkan is verbose
2. [Setup & Build](docs/01_setup_and_build.md) — libraries, CMake, building
3. [volk & the Instance](docs/02_instance_and_volk.md)
4. [Validation Layers](docs/03_validation_layers.md) — your best debugging friend
5. [Window & Surface](docs/04_surface_and_window.md)
6. [Devices & Queues](docs/05_devices_and_queues.md)
7. [Memory, VMA & Vertex Buffer](docs/06_vma_and_buffers.md)
8. [Swapchain & Image Views](docs/07_swapchain.md)
9. [Render Pass](docs/08_render_pass.md)
10. [Shaders & the Graphics Pipeline](docs/09_shaders_and_pipeline.md)
11. [Framebuffers & Command Buffers](docs/10_framebuffers_and_commands.md)
12. [Synchronization & the Frame Loop](docs/11_drawing_and_sync.md)
13. [Glossary & Quick Reference](docs/12_glossary.md)

---

## Build & run

### Prerequisites (installed system-wide)

- A C++17 compiler (GCC/Clang)
- CMake ≥ 3.20
- The Vulkan SDK / loader / validation layers (`vulkan-headers`,
  `vulkan-loader`, `vulkan-validationlayers`)
- `glslc` (ships with the Vulkan SDK)
- GLFW 3.3+, GLM
- volk (`volk.h`) and VMA (`vk_mem_alloc.h`) headers

On Arch/CachyOS these come from:
`vulkan-headers vulkan-icd-loader vulkan-validation-layers shaderc glfw glm volk vulkan-memory-allocator`.

### Commands

```bash
# from this folder (01_HelloTriangle)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
./build/HelloTriangle
```

- **Debug** build → validation layers ON (recommended while learning).
- **Release** build (`-DCMAKE_BUILD_TYPE=Release`) → validation OFF.

You should see a window with the triangle and, in the terminal:

```
Using GPU: <your GPU>
```

Close the window to exit. Resize it to watch the swapchain recreation logic work.

---

## Project layout

```
01_HelloTriangle/
├── CMakeLists.txt          # build + shader compilation (explained in docs/01)
├── README.md               # this file
├── src/
│   ├── main.cpp            # the whole application, top-to-bottom
│   └── vk_backend_impl.cpp # single TU that compiles volk + VMA
├── shaders/
│   ├── triangle.vert       # vertex shader
│   └── triangle.frag       # fragment shader
└── docs/                   # the learning guide (13 chapters)
```

## Tips for learning

- Read a doc chapter, then read the matching part of `src/main.cpp`. The code is
  ordered to match the docs.
- **Break things on purpose** in a debug build (e.g. skip a `vkDestroy...`, set a
  wrong `sType`) and read what the validation layer says. This is the fastest
  way to build intuition.
- Keep the [Vulkan spec](https://registry.khronos.org/vulkan/) and
  [vulkan-tutorial.com](https://vulkan-tutorial.com/) open as references.
