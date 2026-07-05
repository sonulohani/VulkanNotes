# 00 · The Big Picture

Welcome! This document set walks you through **every line of a Vulkan program
that draws a single triangle**. By the end you will understand not just *what*
each call does, but *why* it exists.

Read these in order. Each one builds on the previous.

| # | File | Topic |
|---|------|-------|
| 00 | `00_overview.md` | Mental model, why Vulkan is verbose (this file) |
| 01 | `01_setup_and_build.md` | The libraries, CMake, building & running |
| 02 | `02_instance_and_volk.md` | volk, the Vulkan instance |
| 03 | `03_validation_layers.md` | Catching your mistakes automatically |
| 04 | `04_surface_and_window.md` | GLFW window + the surface |
| 05 | `05_devices_and_queues.md` | Physical device, queue families, logical device |
| 06 | `06_vma_and_buffers.md` | Memory, VMA, the vertex buffer |
| 07 | `07_swapchain.md` | The swapchain + image views |
| 08 | `08_render_pass.md` | Render passes & attachments |
| 09 | `09_shaders_and_pipeline.md` | GLSL → SPIR-V, the graphics pipeline |
| 10 | `10_framebuffers_and_commands.md` | Framebuffers, command pools & buffers |
| 11 | `11_drawing_and_sync.md` | Semaphores, fences, the frame loop, resizing |
| 12 | `12_glossary.md` | One-line definitions of every term |

---

## What are we actually building?

A window with a triangle whose corners are red, green, and blue, smoothly
blended across its surface:

```
        red
         /\
        /  \
       /    \
      /      \
   blue ---- green
```

That's it. But getting a GPU to draw this "simple" thing explicitly is what
makes Vulkan a great teacher: nothing is hidden.

---

## Why is Vulkan so verbose?

If you have used OpenGL, you may remember drawing a triangle in ~50 lines.
Vulkan needs ~800. Why?

**OpenGL is a car with automatic transmission.** You press the pedal and the
driver (the GPU driver) decides gears, timing, memory, and synchronization for
you. Convenient, but you have no control and it can make slow decisions.

**Vulkan is a manual transmission race car.** *You* decide:

- exactly which GPU to use,
- how memory is allocated,
- when the CPU and GPU are allowed to touch the same data,
- what the rendering steps are, ahead of time.

This is more work, but it means:

1. **Predictable performance** — no hidden driver magic.
2. **Multithreading** — you can record GPU commands from many CPU threads.
3. **Explicitness** — errors are your fault and are catchable (validation layers).

> **Key idea:** In Vulkan you *describe* everything up front by filling in
> big C structs, then hand them to `vkCreate...` functions. Almost every
> object is created this way.

---

## The core mental model

Vulkan is a pipeline of objects, each created from the one before it. Here is
the dependency chain we build, top to bottom:

```
Instance                (the Vulkan library connection)
  └── Surface           (a drawable area on our window)
        └── Physical Device   (a real GPU we picked)
              └── Logical Device   (our "session" with that GPU)
                    ├── Queues          (where we submit work)
                    ├── Allocator (VMA) (manages GPU memory)
                    ├── Swapchain       (the images shown on screen)
                    ├── Render Pass     (a plan for one drawing operation)
                    ├── Pipeline        (shaders + all fixed settings)
                    ├── Framebuffers    (render pass + real images)
                    ├── Command Pool    (allocates command buffers)
                    └── Sync objects    (semaphores + fences)
```

Every arrow means "needs the thing above it to exist first". When we clean up,
we destroy in **reverse** order.

---

## The two phases of a Vulkan app

1. **Setup (`initVulkan`)** — happens once. Slow, verbose, builds all the
   objects above. This is 90% of the code.
2. **Draw loop (`drawFrame`)** — happens ~60 times per second. Fast, small.
   It just: acquire an image → record commands → submit → present.

Most of the "800 lines" are the one-time setup. The actual per-frame work is
tiny, which is exactly why Vulkan is fast.

---

## How the pieces of *this* project map together

| You want... | The tool | Why not do it by hand? |
|-------------|----------|------------------------|
| A window & keyboard/mouse | **GLFW** | OS windowing is platform-specific and painful |
| Vector/matrix math | **GLM** | Correct, tested, GLSL-like C++ math |
| Load Vulkan functions | **volk** | Faster & cleaner than the default loader |
| Allocate GPU memory | **VMA** | Manual Vulkan memory management is error-prone |
| Compile shaders | **glslc** | Turns human GLSL into GPU-ready SPIR-V |

Next up: [`01_setup_and_build.md`](01_setup_and_build.md).
