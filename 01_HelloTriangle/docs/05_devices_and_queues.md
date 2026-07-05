# 05 · Physical Device, Queues & Logical Device

This step is where we pick a GPU and open a "session" with it. Three concepts:
**physical device**, **queue families**, and **logical device**.

---

## 1. Physical device — choosing a GPU

A `VkPhysicalDevice` represents a real GPU in your machine (you might have an
integrated *and* a discrete one). We enumerate them with the two-call idiom:

```cpp
uint32_t count;
vkEnumeratePhysicalDevices(instance, &count, nullptr);
std::vector<VkPhysicalDevice> devices(count);
vkEnumeratePhysicalDevices(instance, &count, devices.data());
```

Then we pick the first one that is **suitable**:

```cpp
bool isDeviceSuitable(VkPhysicalDevice dev) {
    QueueFamilyIndices indices = findQueueFamilies(dev); // has graphics+present?
    bool extensionsOk = checkDeviceExtensionSupport(dev); // has swapchain ext?
    bool swapchainOk = /* at least one format & present mode */;
    return indices.isComplete() && extensionsOk && swapchainOk;
}
```

For a real app you might *score* devices (prefer discrete GPUs, more memory,
etc.). For a triangle, "the first one that works" is fine. We print the chosen
GPU's name so you can confirm which one you got:

```cpp
VkPhysicalDeviceProperties props;
vkGetPhysicalDeviceProperties(physicalDevice, &props);
std::cout << "Using GPU: " << props.deviceName << "\n";
```

A physical device is only *queried*, never created or destroyed — it's owned by
the system.

---

## 2. Queue families — where work is submitted

You never "call" the GPU directly. Instead you record commands into a **command
buffer** and submit it to a **queue**. Queues come in **families**, each family
supporting certain operation types:

- **Graphics** — draw calls.
- **Compute** — general-purpose GPU compute.
- **Transfer** — memory copies.
- **Present** — showing an image on our surface (a surface-specific capability).

We need two capabilities for a triangle:

1. A queue family that supports **graphics** (to draw).
2. A queue family that supports **present** (to show the result on our surface).

Often one family does both, but not always, so we track them separately:

```cpp
struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;
    bool isComplete() const {
        return graphicsFamily && presentFamily;
    }
};
```

`std::optional` elegantly expresses "we might not have found this yet". We scan
every family:

```cpp
for (uint32_t i = 0; i < count; ++i) {
    if (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        indices.graphicsFamily = i;

    VkBool32 presentSupport = VK_FALSE;
    vkGetPhysicalDeviceSurfaceSupportKHR(dev, i, surface, &presentSupport);
    if (presentSupport)
        indices.presentFamily = i;
}
```

Note that graphics support is a simple bit flag, but *present* support depends
on the **surface**, so it has its own query function.

---

## 3. Logical device — your handle to the GPU

The `VkDevice` (logical device) is your program's actual interface to the chosen
GPU. It's where you create almost everything else (pipelines, buffers, command
pools...). You also request the queues you'll use *at device-creation time*.

### Requesting queues

We may need one or two distinct families, so we deduplicate with a `std::set`:

```cpp
std::set<uint32_t> uniqueFamilies = {
    indices.graphicsFamily.value(),
    indices.presentFamily.value()
};
float priority = 1.0f;                 // 0.0–1.0, relative scheduling priority
for (uint32_t family : uniqueFamilies) {
    VkDeviceQueueCreateInfo qi{};
    qi.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    qi.queueFamilyIndex = family;
    qi.queueCount = 1;
    qi.pQueuePriorities = &priority;
    queueInfos.push_back(qi);
}
```

### Creating the device

```cpp
VkDeviceCreateInfo createInfo{};
createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
createInfo.queueCreateInfoCount = queueInfos.size();
createInfo.pQueueCreateInfos = queueInfos.data();
createInfo.pEnabledFeatures = &features;                 // none needed here
createInfo.enabledExtensionCount = kDeviceExtensions.size();
createInfo.ppEnabledExtensionNames = kDeviceExtensions.data(); // swapchain ext
VK_CHECK(vkCreateDevice(physicalDevice, &createInfo, nullptr, &device));
```

Two kinds of "extensions" exist: *instance* extensions (doc 02) and *device*
extensions. `VK_KHR_swapchain` is a **device** extension — the ability to
present belongs to the device, so we enable it here.

### Retrieving the queue handles

Queues are created *with* the device. We just fetch their handles:

```cpp
vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
```

If both families are the same index, `graphicsQueue` and `presentQueue` will be
the same queue — that's fine.

### Then: tell volk about the device

Right after this we call `volkLoadDevice(device)` (doc 02) so all device-level
functions use the fast direct-pointer path.

Next: [`06_vma_and_buffers.md`](06_vma_and_buffers.md).
