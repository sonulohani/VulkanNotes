# 04 · Window & Surface

## The window (GLFW)

Vulkan itself cannot create a window — it's a pure GPU API. GLFW handles the OS
window for us:

```cpp
glfwInit();
glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);  // no OpenGL context!
window = glfwCreateWindow(WIDTH, HEIGHT, "Hello Triangle", nullptr, nullptr);
```

The crucial hint is `GLFW_NO_API`. By default GLFW creates an OpenGL context;
we tell it not to, because we're using Vulkan.

### Handling resize

When the window is resized, our swapchain (the set of images we draw into,
covered in doc 07) becomes the wrong size and must be rebuilt. We can't rebuild
it mid-draw, so we just set a flag and handle it at a safe point:

```cpp
glfwSetWindowUserPointer(window, this);
glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);

static void framebufferResizeCallback(GLFWwindow* win, int, int) {
    auto app = reinterpret_cast<HelloTriangleApplication*>(
        glfwGetWindowUserPointer(win));
    app->framebufferResized = true;
}
```

GLFW callbacks are plain C functions, so they can't be member functions. The
trick is `glfwSetWindowUserPointer`, which stashes our `this` pointer inside the
window; the static callback fishes it back out to reach our object.

---

## The surface

A **surface** (`VkSurfaceKHR`) is the bridge between Vulkan and the window
system. It represents "the drawable area of this window" in a platform-neutral
way. The GPU renders into images that are then handed to the surface to display.

Creating a surface is platform-specific (X11, Wayland, Win32...), but GLFW wraps
all of that in one call:

```cpp
VK_CHECK(glfwCreateWindowSurface(instance, window, nullptr, &surface));
```

That's it — one line. Behind the scenes GLFW picks the right
`vkCreate*SurfaceKHR` for your platform. This is why we enabled the surface
extensions on the instance back in doc 02: without them this call would fail.

### Why the surface matters for GPU selection

The surface influences the rest of setup:

- We only accept a GPU that can **present** to this surface (doc 05).
- The surface tells us which **image formats** and **present modes** are
  supported (doc 07).

So the order is important: instance → surface → pick GPU → device.

> **`KHR` suffix?** It marks an extension promoted by the Khronos group.
> Windowing is not in Vulkan core (some devices are headless / servers), so
> everything about surfaces and swapchains is a `KHR` extension.

Next: [`05_devices_and_queues.md`](05_devices_and_queues.md).
