# 12 · Glossary & Quick Reference

One-line reminders for every term used in this project.

## Core objects

| Term | What it is |
|------|-----------|
| **Instance** (`VkInstance`) | Your app's connection to the Vulkan library; used to find GPUs. |
| **Physical device** (`VkPhysicalDevice`) | A real GPU in the machine (queried, never created). |
| **Logical device** (`VkDevice`) | Your active session with a chosen GPU; creates most other objects. |
| **Queue** (`VkQueue`) | Where you submit command buffers for the GPU to execute. |
| **Queue family** | A group of queues supporting the same operations (graphics/present/compute/transfer). |
| **Surface** (`VkSurfaceKHR`) | Platform-neutral handle to a window's drawable area. |
| **Swapchain** (`VkSwapchainKHR`) | The rotating set of images shown on screen (double/triple buffering). |
| **Image** (`VkImage`) | Raw GPU pixel data (2D here). |
| **Image view** (`VkImageView`) | A "lens" describing how to interpret an image (format, mips, aspect). |
| **Render pass** (`VkRenderPass`) | A plan describing attachments, load/store ops, and subpasses. |
| **Attachment** | An image a render pass draws into (or reads), e.g. our color target. |
| **Subpass** | One phase of a render pass working on a set of attachments. |
| **Framebuffer** (`VkFramebuffer`) | Binds a render pass's abstract attachments to concrete image views. |
| **Pipeline** (`VkPipeline`) | Shaders + all fixed render state, frozen into one immutable object. |
| **Pipeline layout** (`VkPipelineLayout`) | Declares descriptor sets & push constants a pipeline uses. |
| **Shader module** (`VkShaderModule`) | A wrapper around compiled SPIR-V bytecode. |
| **Command pool** (`VkCommandPool`) | Allocates command buffers; tied to one queue family. |
| **Command buffer** (`VkCommandBuffer`) | A recorded list of GPU commands, submitted to a queue. |
| **Buffer** (`VkBuffer`) | A linear chunk of GPU-accessible memory (our vertices live here). |

## Synchronization

| Term | What it is |
|------|-----------|
| **Semaphore** (`VkSemaphore`) | Orders work between GPU operations; CPU never waits on it. |
| **Fence** (`VkFence`) | Lets the CPU wait for the GPU to finish something. |
| **Subpass dependency** | Declares ordering/memory rules between subpasses (and external work). |
| **Frames in flight** | How many frames the CPU may prepare ahead of the GPU (2 here). |
| **Image layout** | How an image's memory is arranged for a given use (draw / present / sample). |

## Libraries & tools

| Term | What it is |
|------|-----------|
| **GLFW** | Cross-platform window + input + surface creation. |
| **GLM** | Header-only GLSL-style C++ math (vectors, matrices). |
| **volk** | Loads Vulkan function pointers directly for speed and clean extension use. |
| **VMA** | Vulkan Memory Allocator; picks memory types and sub-allocates for you. |
| **SPIR-V** | Binary shader bytecode the GPU consumes. |
| **glslc** | Compiler that turns GLSL text into SPIR-V. |
| **Validation layers** | Optional debug layer that checks every call against the spec. |

## Recurring patterns you'll see everywhere

1. **`sType` first.** Every create-info struct sets its `sType`, then is zero-
   initialized with `{}`.
2. **`pNext` chains.** Extra structs are attached via the `pNext` linked list.
3. **Count + pointer.** Arrays are passed as a length plus a data pointer.
4. **Two-call idiom.** Query arrays by calling once for the count, allocating,
   then calling again to fill.
5. **CreateInfo → vkCreate → handle.** Almost every object is born this way.
6. **Destroy in reverse.** Clean up objects opposite to creation order.

## The dependency chain (creation order)

```
Instance → Surface → Physical Device → Logical Device (+Queues)
→ VMA Allocator → Swapchain → Image Views → Render Pass
→ Pipeline → Framebuffers → Command Pool → Vertex Buffer
→ Command Buffers → Sync objects
```

Cleanup runs this list in reverse.

## Where to go next

Now that you can draw a triangle, natural next steps are:

- **Index buffers** — reuse vertices for quads/meshes (`vkCmdDrawIndexed`).
- **Uniform buffers + descriptor sets** — send matrices to shaders (MVP transform).
- **Textures** — sample images in the fragment shader.
- **Depth buffering** — draw 3D scenes correctly.
- **Push constants** — small, fast per-draw data.

Each of these builds directly on the objects you learned here.
