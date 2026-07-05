# 03 · Validation Layers

This is the most important tool for learning Vulkan. Read it carefully.

## The problem

Vulkan is **fast because it barely checks anything**. If you pass a wrong enum,
forget to set a field, use an object after destroying it, or synchronize wrong,
the driver often won't tell you — you'll just get a black screen, a crash, or
(worst of all) something that works on your machine but breaks on someone
else's.

## The solution: layers

A **layer** is code that inserts itself between your application and the driver.
The Khronos **validation layer** (`VK_LAYER_KHRONOS_validation`) checks every
call against the spec and reports mistakes with detailed messages.

Because it's a layer, it costs nothing in release builds — you simply don't
enable it. We turn it on only for debug builds:

```cpp
#ifdef NDEBUG
constexpr bool ENABLE_VALIDATION = false;   // release
#else
constexpr bool ENABLE_VALIDATION = true;    // debug
#endif
```

## Enabling the layer

First we check it's actually installed:

```cpp
bool checkValidationLayerSupport() {
    uint32_t count;
    vkEnumerateInstanceLayerProperties(&count, nullptr);          // how many?
    std::vector<VkLayerProperties> available(count);
    vkEnumerateInstanceLayerProperties(&count, available.data()); // fill in
    // ...then search for "VK_LAYER_KHRONOS_validation"
}
```

> **The two-call idiom.** This "call once for the count, allocate, call again to
> fill" pattern appears *everywhere* in Vulkan for returning arrays. Get used to
> it — you'll see it for physical devices, queue families, surface formats, and
> swapchain images.

Then we pass the layer name to `vkCreateInstance`:

```cpp
createInfo.enabledLayerCount = kValidationLayers.size();
createInfo.ppEnabledLayerNames = kValidationLayers.data();
```

## Receiving messages: the debug messenger

The layer needs somewhere to send its messages. We give it a **callback
function**:

```cpp
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT* data,
    void* userData) {
    if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        std::cerr << "[validation] " << data->pMessage << "\n";
    return VK_FALSE;
}
```

- `severity` ranges from verbose → info → warning → error. We only print
  warnings and errors to reduce noise.
- `data->pMessage` is the human-readable explanation.
- Returning `VK_FALSE` means "don't abort the call that triggered this". You
  almost always return `VK_FALSE`.
- `VKAPI_ATTR` / `VKAPI_CALL` set the correct calling convention so Vulkan can
  call our C++ function safely across the API boundary.

We register the callback by creating a `VkDebugUtilsMessengerEXT`:

```cpp
VkDebugUtilsMessengerCreateInfoEXT info{};
info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
info.messageSeverity = /* verbose | warning | error */;
info.messageType = /* general | validation | performance */;
info.pfnUserCallback = debugCallback;
vkCreateDebugUtilsMessengerEXT(instance, &info, nullptr, &debugMessenger);
```

## The chicken-and-egg trick

The messenger is created *after* the instance — so who catches errors that
happen *during* `vkCreateInstance` and `vkDestroyInstance` themselves?

We solve this by putting a *temporary* messenger struct into the instance's
`pNext` chain:

```cpp
VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
populateDebugMessengerCreateInfo(debugCreateInfo);
createInfo.pNext = &debugCreateInfo;   // active during create/destroy instance
```

This is a great first look at `pNext`: a linked list you use to attach extra
structs to a base struct. Vulkan follows the chain and finds the debug info.

## Try it yourself

The best way to appreciate validation is to *break* something on purpose. For
example, comment out a `vkDestroy...` call in `cleanup()` and run a debug build.
The layer will report a leaked object at exit. Or set a wrong `sType`. Learning
to read these messages is a core Vulkan skill.

Next: [`04_surface_and_window.md`](04_surface_and_window.md).
