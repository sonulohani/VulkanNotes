# 10 · Framebuffers & Command Buffers

## Framebuffers — binding the render pass to real images

The render pass (doc 08) describes attachments *abstractly* ("attachment 0 is a
color image in this format"). A **framebuffer** binds that description to the
*actual* image views it will use.

Since we have several swapchain images, we create one framebuffer per image
view:

```cpp
for (size_t i = 0; i < swapchainImageViews.size(); ++i) {
    VkImageView attachments[] = { swapchainImageViews[i] };
    VkFramebufferCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    info.renderPass = renderPass;          // must be compatible with this pass
    info.attachmentCount = 1;
    info.pAttachments = attachments;       // attachment 0 = this image view
    info.width  = swapchainExtent.width;
    info.height = swapchainExtent.height;
    info.layers = 1;
    vkCreateFramebuffer(device, &info, nullptr, &swapchainFramebuffers[i]);
}
```

Mental model:
- **Render pass** = the form with blank fields ("color goes here").
- **Framebuffer** = the same form filled in with a specific image view.

At draw time we pick the framebuffer that matches the swapchain image we
acquired.

---

## Command pool — where command buffers come from

In Vulkan you don't execute GPU work immediately. You **record** commands into a
**command buffer**, then **submit** the whole buffer to a queue. Command buffers
are allocated from a **command pool**:

```cpp
VkCommandPoolCreateInfo info{};
info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // reset individually
info.queueFamilyIndex = indices.graphicsFamily.value();       // for the graphics queue
vkCreateCommandPool(device, &info, nullptr, &commandPool);
```

- A pool is tied to **one queue family** — buffers from it can only go to queues
  of that family (here, graphics).
- `RESET_COMMAND_BUFFER_BIT` lets us reset and re-record each buffer every frame
  (which we do), rather than throwing them away.

> Command pools are also the key to **multithreading**: give each thread its own
> pool and it can record commands in parallel with no locking.

---

## Command buffers

We allocate one command buffer per **frame in flight** (see doc 11 —
`MAX_FRAMES_IN_FLIGHT = 2`), so the CPU can be recording the next frame while the
GPU works on the current one:

```cpp
commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
VkCommandBufferAllocateInfo info{};
info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
info.commandPool = commandPool;
info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;   // submitted directly to a queue
info.commandBufferCount = commandBuffers.size();
vkAllocateCommandBuffers(device, &info, commandBuffers.data());
```

`PRIMARY` buffers are submitted straight to a queue. (`SECONDARY` buffers can be
called *from* primary ones — an advanced feature we don't need.)

### Recording the drawing commands

Each frame we reset the buffer and record it fresh. Here's the whole recording,
which is where the actual triangle draw happens:

```cpp
vkBeginCommandBuffer(cmd, &begin);

  VkRenderPassBeginInfo rp{...};
  rp.renderPass  = renderPass;
  rp.framebuffer = swapchainFramebuffers[imageIndex];  // the acquired image
  rp.renderArea.extent = swapchainExtent;
  rp.pClearValues = &clearColor;                       // background color
  vkCmdBeginRenderPass(cmd, &rp, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

    // Dynamic viewport & scissor (because we declared them dynamic).
    vkCmdSetViewport(cmd, 0, 1, &viewport);
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    // Bind our VMA vertex buffer to binding 0.
    VkBuffer buffers[] = { vertexBuffer };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(cmd, 0, 1, buffers, offsets);

    // Draw 3 vertices, 1 instance.
    vkCmdDraw(cmd, 3, 1, 0, 0);

  vkCmdEndRenderPass(cmd);
vkEndCommandBuffer(cmd);
```

Reading it top to bottom:

1. **Begin** the command buffer (start recording).
2. **Begin render pass** — this performs the `loadOp` (clears the image to our
   background color) and sets up attachment 0 = the acquired swapchain image.
3. **Bind the pipeline** — now the GPU knows which shaders + state to use.
4. **Set viewport/scissor** — required because they're dynamic state.
5. **Bind the vertex buffer** — tells the GPU where the triangle's vertices are.
6. **`vkCmdDraw(3, 1, 0, 0)`** — the actual draw: 3 vertices, 1 instance,
   starting at vertex 0. This runs the vertex shader 3 times and the fragment
   shader once per covered pixel.
7. **End render pass** — performs the `storeOp` (keeps the result) and
   transitions the image to `PRESENT_SRC_KHR`.
8. **End** the command buffer.

Note that *nothing has executed yet* — we've only written down instructions.
They run when we submit the buffer to the queue, which is the next doc.

Next: [`11_drawing_and_sync.md`](11_drawing_and_sync.md).
