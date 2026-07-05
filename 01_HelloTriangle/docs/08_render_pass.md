# 08 · The Render Pass

## What is a render pass?

A **render pass** is a *plan* that describes one rendering operation to the GPU
ahead of time:

- Which images (**attachments**) will be drawn into.
- What to do with them at the **start** (clear? keep?) and **end** (store?
  discard?).
- What **layout** each image should be in before and after.
- How the work is split into **subpasses**.

Why describe all this up front? Because tiled GPUs (especially on mobile, but
the model is universal) can use this plan to organize memory traffic
efficiently. You're giving the driver a roadmap of your frame.

For our triangle we need exactly **one attachment** (the color image we display)
and **one subpass**.

## The color attachment

```cpp
VkAttachmentDescription colorAttachment{};
colorAttachment.format  = swapchainImageFormat;      // must match the swapchain
colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;     // no multisampling (MSAA)
colorAttachment.loadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;  // clear it at frame start
colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // keep the result
colorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;      // "don't care what was here"
colorAttachment.finalLayout   = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;// "ready to display"
```

Key ideas:

- **`loadOp = CLEAR`** wipes the image to a background color before we draw, so
  we don't see garbage from a previous frame.
- **`storeOp = STORE`** keeps what we drew (obviously we want to see it).
- **Image layouts** describe how an image's memory is arranged for a particular
  use. The GPU stores images differently depending on whether it's *being drawn
  to*, *being read as a texture*, or *being presented*. We start from
  `UNDEFINED` (contents don't matter) and end at `PRESENT_SRC_KHR` (optimal for
  handing to the display).

## The subpass

A render pass has one or more **subpasses**. Each subpass reads/writes a set of
attachments. Multiple subpasses let you do things like deferred shading where one
pass reads another's output efficiently. We only need one:

```cpp
VkAttachmentReference colorRef{};
colorRef.attachment = 0; // index into the render pass's attachment array
colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // layout during drawing

VkSubpassDescription subpass{};
subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
subpass.colorAttachmentCount = 1;
subpass.pColorAttachments = &colorRef;
```

The `layout = COLOR_ATTACHMENT_OPTIMAL` tells Vulkan to transition the image into
the best layout for being rendered to *while this subpass runs*. Vulkan performs
these layout transitions for us based on the render pass description.

## The subpass dependency

Here's the subtle part. Our subpass writes color, but the swapchain image might
not be ready yet (the previous frame could still be reading it). A **subpass
dependency** expresses this ordering to the GPU:

```cpp
VkSubpassDependency dependency{};
dependency.srcSubpass = VK_SUBPASS_EXTERNAL;   // "work before this render pass"
dependency.dstSubpass = 0;                      // our subpass
dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
dependency.srcAccessMask = 0;
dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
```

In plain English: *"Don't start writing color in my subpass until the previous
use of this image's color-output stage is finished."* This pairs with the
`imageAvailable` semaphore in the draw loop (doc 11) to guarantee we never draw
into an image that's still being displayed.

Synchronization is the hardest part of Vulkan. For now, just know this
dependency is what makes the render-into-swapchain-image step safe.

## Assembling the render pass

```cpp
VkRenderPassCreateInfo info{};
info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
info.attachmentCount = 1;   info.pAttachments  = &colorAttachment;
info.subpassCount    = 1;   info.pSubpasses    = &subpass;
info.dependencyCount = 1;   info.pDependencies = &dependency;
vkCreateRenderPass(device, &info, nullptr, &renderPass);
```

The render pass is a *template*. It gets paired with actual images through
**framebuffers** (doc 10) and used at draw time with `vkCmdBeginRenderPass`
(doc 11).

Next: [`09_shaders_and_pipeline.md`](09_shaders_and_pipeline.md).
