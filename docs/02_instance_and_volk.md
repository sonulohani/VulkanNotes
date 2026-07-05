# 02 · volk & the Vulkan Instance

## What is volk and why use it?

When you call a Vulkan function like `vkCreateBuffer`, that call doesn't go
straight to your GPU driver. It first goes through the **Vulkan loader**
(`libvulkan.so`), which figures out which driver (Intel, NVIDIA, AMD...) should
handle it. Doing that lookup on *every single call* has a small cost.

**volk** removes that overhead by fetching the real function pointers once and
calling them directly. It works in three steps:

```cpp
volkInitialize();          // 1. dlopen libvulkan, get vkGetInstanceProcAddr
// ... create the instance ...
volkLoadInstance(instance);// 2. load all instance-level functions
// ... create the device ...
volkLoadDevice(device);    // 3. load device-level functions (fastest path)
```

- **Step 1** happens before anything else. It opens the Vulkan library and
  grabs the one bootstrap function, `vkGetInstanceProcAddr`, plus a few globals
  like `vkCreateInstance`.
- **Step 2** happens right after we create the instance. Now volk can load
  functions that need an instance (surface, physical-device queries, etc.).
- **Step 3** happens after we create the logical device. Device-level functions
  loaded this way skip the loader's dispatch entirely — the fastest option.

In our code:

```cpp
VK_CHECK(volkInitialize());
glfwInitVulkanLoader(vkGetInstanceProcAddr);  // make GLFW share volk's loader
createInstance();
volkLoadInstance(instance);
...
createLogicalDevice();
volkLoadDevice(device);
```

That `glfwInitVulkanLoader` call tells GLFW to use the exact same loader volk
opened, so both agree on where Vulkan functions come from. It must be called
before `glfwInit()` uses Vulkan (we call it early, which is safe).

> **Include order matters.** `#include <volk.h>` must come before anything that
> includes the Vulkan headers. volk defines `VK_NO_PROTOTYPES`, which stops the
> normal (statically-linked) Vulkan function declarations from appearing, so
> volk's function *pointers* are the only version in scope. We also set
> `GLFW_INCLUDE_NONE` so GLFW doesn't drag in its own Vulkan header.

---

## The Vulkan Instance

The **instance** is your program's connection to the Vulkan library. It stores
per-application state and is the object from which you enumerate GPUs. Nothing
else can be created without it.

Creating Vulkan objects always follows the same recipe:

1. Fill in a `Vk...CreateInfo` struct.
2. Call `vkCreate...` with it.
3. Get a handle back.

### Step 1 — describe the application

```cpp
VkApplicationInfo appInfo{};
appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
appInfo.pApplicationName = "Hello Triangle";
appInfo.apiVersion = VK_API_VERSION_1_3;
```

**Why the `sType` field?** Almost every Vulkan struct starts with an `sType`
(structure type) enum. This lets Vulkan and extensions extend structs safely via
the `pNext` pointer (a linked list of extra structs). You must set `sType`
correctly every time. The `{}` zero-initializes the rest, which is important —
uninitialized fields cause undefined behaviour.

`apiVersion` says which Vulkan version your app targets. We ask for 1.3.

### Step 2 — list the extensions we need

Vulkan's core knows nothing about windows. To draw to a screen we need the
*surface* extensions, and GLFW tells us which ones this platform requires:

```cpp
uint32_t glfwCount = 0;
const char** glfwExt = glfwGetRequiredInstanceExtensions(&glfwCount);
std::vector<const char*> extensions(glfwExt, glfwExt + glfwCount);
if (ENABLE_VALIDATION)
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
```

On Linux this typically yields `VK_KHR_surface` + `VK_KHR_xcb_surface` (or
Wayland). We also add the debug-utils extension when validation is on so we can
receive diagnostic messages.

> **Extensions vs. layers**
> - **Extensions** add *features* (e.g. the ability to present to a window).
> - **Layers** insert themselves *between* your app and the driver to observe
>   or modify calls (e.g. validation). More on layers in the next doc.

### Step 3 — create it

```cpp
VkInstanceCreateInfo createInfo{};
createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
createInfo.pApplicationInfo = &appInfo;
createInfo.enabledExtensionCount = extensions.size();
createInfo.ppEnabledExtensionNames = extensions.data();
// (validation layers + a temporary debug messenger via pNext, if enabled)
VK_CHECK(vkCreateInstance(&createInfo, nullptr, &instance));
```

Notice the pattern that repeats forever in Vulkan: **count + pointer**. Array
inputs are always given as a length plus a pointer to the first element
(`enabledExtensionCount` + `ppEnabledExtensionNames`).

The `nullptr` second argument is a custom CPU allocator callback — we never use
it, so it's always `nullptr`.

### About `VK_CHECK`

Every Vulkan function returns a `VkResult`. `VK_SUCCESS` (0) means OK; anything
else is an error or a status. Our macro throws a readable exception on failure
so we fail loudly instead of silently continuing with a broken object:

```cpp
#define VK_CHECK(call) /* throw if (call) != VK_SUCCESS */
```

Next: [`03_validation_layers.md`](03_validation_layers.md).
