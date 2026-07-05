# 11 · Synchronization & the Frame Loop

This is the part that trips up everyone. Take it slowly.

## Why synchronization exists

The CPU and GPU run **independently and in parallel**. When you submit a command
buffer, the function returns immediately — the GPU works on it *later*. So you
constantly face questions like:

- Is the swapchain image I want to draw into still being displayed?
- Has the GPU finished the frame before I reuse its command buffer?
- Is rendering done before I ask the display to show the image?

Vulkan gives you two synchronization primitives to answer these:

| Primitive | Synchronizes | Waited on by |
|-----------|--------------|--------------|
| **Semaphore** | GPU ↔ GPU (queue operations) | the GPU |
| **Fence** | GPU → CPU | the CPU (`vkWaitForFences`) |

- A **semaphore** orders work *between GPU operations* (e.g. "present only after
  rendering"). The CPU never waits on it.
- A **fence** lets the *CPU* know the GPU finished something (e.g. "don't reuse
  this command buffer until its previous submission is done").

## Frames in flight

To keep both processors busy we allow **2 frames in flight**
(`MAX_FRAMES_IN_FLIGHT`). While the GPU renders frame N, the CPU records frame
N+1. We keep one set of sync objects per in-flight frame so they don't clash.

We create:

```cpp
imageAvailableSemaphores  // per frame-in-flight: image is ready to render into
renderFinishedSemaphores  // per swapchain image: rendering is done, safe to present
inFlightFences            // per frame-in-flight: this frame's GPU work is complete
```

> Why is `renderFinished` **per swapchain image** rather than per frame? Because
> we signal it based on which image we acquired, and a semaphore must not be
> re-signalled before its previous signal is consumed. Sizing it to the image
> count keeps the validation layers happy.

The fences are created **already signalled** so the very first frame doesn't
deadlock waiting on work that never happened:

```cpp
fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
```

---

## The frame loop, step by step

Here is `drawFrame()` broken into its five stages.

### 1. Wait for this frame slot to be free

```cpp
vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
```

Blocks the CPU until the GPU has finished the *previous* time we used this frame
slot. Now it's safe to reuse this slot's command buffer and semaphores.

### 2. Acquire an image from the swapchain

```cpp
VkResult acquire = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX,
    imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
if (acquire == VK_ERROR_OUT_OF_DATE_KHR) { recreateSwapchain(); return; }
```

Asks the swapchain for the index of the next image we can draw into. The image
may not be *physically* ready yet, so the swapchain will signal
`imageAvailableSemaphores[currentFrame]` once it is. If the swapchain is stale
(e.g. window resized), we rebuild it and skip this frame.

We reset the fence only *after* we've decided to actually submit work, to avoid
deadlocks on the early-return path:

```cpp
vkResetFences(device, 1, &inFlightFences[currentFrame]);
```

### 3. Record the command buffer

```cpp
vkResetCommandBuffer(commandBuffers[currentFrame], 0);
recordCommandBuffer(commandBuffers[currentFrame], imageIndex);
```

(See doc 10 for what recording does.)

### 4. Submit to the graphics queue

```cpp
VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[imageIndex] };

VkSubmitInfo submit{};
submit.pWaitSemaphores   = waitSemaphores;   // wait: image available
submit.pWaitDstStageMask = waitStages;       // ...specifically before writing color
submit.pCommandBuffers   = &commandBuffers[currentFrame];
submit.pSignalSemaphores = signalSemaphores; // signal: render finished
vkQueueSubmit(graphicsQueue, 1, &submit, inFlightFences[currentFrame]);
```

This is the key orchestration:

- **Wait** on `imageAvailable` — but cleverly, only at the
  `COLOR_ATTACHMENT_OUTPUT` stage. The GPU can run the vertex shader etc. *before*
  the image is ready; it only needs to stall right before writing color.
- **Signal** `renderFinished` when the commands complete.
- The **fence** `inFlightFences[currentFrame]` is signalled when the GPU finishes
  everything — that's what step 1 waited on.

### 5. Present

```cpp
VkPresentInfoKHR present{};
present.pWaitSemaphores = signalSemaphores;  // wait for render to finish
present.pSwapchains = &swapchain;
present.pImageIndices = &imageIndex;
VkResult r = vkQueuePresentKHR(presentQueue, &present);
if (r == VK_ERROR_OUT_OF_DATE_KHR || r == VK_SUBOPTIMAL_KHR || framebufferResized)
    recreateSwapchain();
```

Hands the finished image back to the swapchain to be displayed — but only after
`renderFinished` is signalled, so we never show a half-drawn frame.

Finally, advance to the next frame slot:

```cpp
currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
```

### The whole dance in one picture

```
CPU: wait fence ─ acquire ─ record ─ submit ───────────────► (loops)
                     │                   │            │
                     ▼ signals           │ waits      │ signals
              imageAvailable ────────────┘            ▼
                                              renderFinished ──► present shows image
GPU:                       [ run pipeline, write color ][ done → signal fence ]
```

---

## Swapchain recreation (resize / minimise)

When the window changes size, the swapchain becomes invalid. We detect this via
the `VK_ERROR_OUT_OF_DATE_KHR`/`VK_SUBOPTIMAL_KHR` results or our
`framebufferResized` flag, then rebuild:

```cpp
void recreateSwapchain() {
    // If minimised (size 0), wait until it's visible again.
    int w = 0, h = 0;
    glfwGetFramebufferSize(window, &w, &h);
    while (w == 0 || h == 0) { glfwGetFramebufferSize(window, &w, &h); glfwWaitEvents(); }

    vkDeviceWaitIdle(device);      // never destroy in-use objects
    cleanupSwapchain();            // framebuffers, image views, swapchain
    createSwapchain();
    createImageViews();
    createFramebuffers();
    // recreate per-image renderFinished semaphores (image count may change)
}
```

`vkDeviceWaitIdle` blocks until the GPU is completely idle — the safe, simple way
to ensure nothing we're about to destroy is still in use. Because our viewport
and scissor are **dynamic**, we do *not* need to rebuild the pipeline. 

## Shutdown

Before destroying anything at program end, we again wait for the GPU:

```cpp
void mainLoop() {
    while (!glfwWindowShouldClose(window)) { glfwPollEvents(); drawFrame(); }
    vkDeviceWaitIdle(device);   // let in-flight frames finish before cleanup
}
```

Then `cleanup()` destroys every object in **reverse** creation order. See
[`12_glossary.md`](12_glossary.md) for a recap of all the terms.
