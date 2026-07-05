# 07 · The Swapchain & Image Views

## What is a swapchain?

The **swapchain** is a set of images that take turns being displayed on screen.
It's how you avoid *tearing* and *flicker*:

- While one image is on the screen, you draw the next frame into a **different**
  image.
- When you're done, you "present" it — the swapchain swaps which image is shown.

This is the classic **double / triple buffering** idea, formalized as a Vulkan
object (`VkSwapchainKHR`). The images belong to the swapchain; you don't create
them yourself, you just render into them and hand them back for display.

Think of a projectionist with a few slides: one is projected while the next is
being prepared, then they swap.

## Choosing swapchain settings

Different GPUs/surfaces support different options, so we first query what's
available (this is the `querySwapchainSupport` function), then choose sensible
values.

### Surface format (color format + color space)

```cpp
for (const auto& f : formats)
    if (f.format == VK_FORMAT_B8G8R8A8_SRGB &&
        f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        return f;                 // ideal: 8-bit BGRA in sRGB
return formats[0];                // otherwise just take the first
```

`B8G8R8A8_SRGB` means 8 bits each for blue, green, red, alpha, interpreted as
sRGB (so colors look correct on a normal monitor).

### Present mode (how images are swapped)

```cpp
for (const auto& m : modes)
    if (m == VK_PRESENT_MODE_MAILBOX_KHR) return m;  // low-latency triple buffer
return VK_PRESENT_MODE_FIFO_KHR;                     // guaranteed to exist
```

- **FIFO** — a queue; presents on vertical blank. This is classic v-sync and is
  the only mode *guaranteed* to be available.
- **MAILBOX** — like FIFO but if the queue is full it *replaces* the waiting
  image, giving lower latency (triple buffering). We prefer it when available.

### Extent (resolution in pixels)

Usually the surface dictates the exact size (`currentExtent`). If it lets us
choose (some window managers set it to a special "undefined" value), we use the
window's framebuffer size, clamped to the allowed min/max:

```cpp
glfwGetFramebufferSize(window, &w, &h);
extent.width  = clamp(w, caps.minImageExtent.width,  caps.maxImageExtent.width);
extent.height = clamp(h, caps.minImageExtent.height, caps.maxImageExtent.height);
```

### Image count

```cpp
uint32_t imageCount = caps.minImageCount + 1;   // one extra to reduce stalls
if (caps.maxImageCount > 0 && imageCount > caps.maxImageCount)
    imageCount = caps.maxImageCount;
```

Asking for one more than the minimum means we're less likely to wait on the
driver to release an image.

## Creating the swapchain

```cpp
VkSwapchainCreateInfoKHR createInfo{};
createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
createInfo.surface = surface;
createInfo.minImageCount = imageCount;
createInfo.imageFormat = surfaceFormat.format;
createInfo.imageColorSpace = surfaceFormat.colorSpace;
createInfo.imageExtent = extent;
createInfo.imageArrayLayers = 1;                          // not stereoscopic
createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // we render into them
```

### Sharing mode

If the graphics and present queue families differ, the images are used by two
families and we pick the simple `CONCURRENT` mode. If they're the same family,
`EXCLUSIVE` is both simpler and faster:

```cpp
if (graphicsFamily != presentFamily) {
    createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    createInfo.queueFamilyIndexCount = 2;
    createInfo.pQueueFamilyIndices = familyIndices;
} else {
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
}
```

The rest:

```cpp
createInfo.preTransform = caps.currentTransform;        // no rotation
createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // ignore window alpha
createInfo.presentMode = presentMode;
createInfo.clipped = VK_TRUE;   // don't shade pixels hidden by other windows
createInfo.oldSwapchain = VK_NULL_HANDLE;
vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapchain);
```

Then we retrieve the images the swapchain created (two-call idiom again):

```cpp
vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);
swapchainImages.resize(imageCount);
vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages.data());
```

---

## Image views

You cannot use a `VkImage` directly in rendering — you need a **`VkImageView`**,
which describes *how* to interpret the image: its format, which mip levels, which
array layers, and which aspect (color vs depth).

An image is the raw pixels; an image view is a "lens" through which the GPU reads
or writes them. We create one view per swapchain image:

```cpp
VkImageViewCreateInfo info{};
info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
info.image = swapchainImages[i];
info.viewType = VK_IMAGE_VIEW_TYPE_2D;
info.format = swapchainImageFormat;
info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; // it's a color image
info.subresourceRange.levelCount = 1;   // no mipmaps
info.subresourceRange.layerCount = 1;   // single layer
vkCreateImageView(device, &info, nullptr, &swapchainImageViews[i]);
```

These views are what the framebuffers (doc 10) will attach to.

Next: [`08_render_pass.md`](08_render_pass.md).
