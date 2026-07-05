# 06 · Memory, VMA & the Vertex Buffer

## Why GPU memory is hard in raw Vulkan

In OpenGL you say "make me a buffer" and it just works. In Vulkan, creating a
buffer is *two* separate steps:

1. Create the `VkBuffer` — this is just a **description** (size, usage). It has
   no memory yet.
2. Find a suitable **memory type**, allocate a `VkDeviceMemory`, and **bind** it
   to the buffer.

Step 2 is where it gets nasty. GPUs expose several **memory heaps** and
**memory types** with different properties:

- `DEVICE_LOCAL` — fast VRAM, but the CPU usually can't touch it directly.
- `HOST_VISIBLE` — the CPU can map and write it, but it may be slower for the
  GPU.
- `HOST_COHERENT` — writes are automatically visible to the GPU (no manual
  flush).

You must query which types satisfy your buffer's requirements *and* your access
needs, respect alignment, and — because drivers limit the number of individual
allocations — sub-allocate many buffers out of a few big allocations. Doing this
correctly by hand is genuinely tricky.

## Enter VMA

The **Vulkan Memory Allocator** does all of the above for you. You describe
*what you want to do* with the memory, and VMA picks the right type and manages
the big allocations internally.

### Creating the allocator

```cpp
VmaVulkanFunctions vk{};
vk.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
vk.vkGetDeviceProcAddr   = vkGetDeviceProcAddr;

VmaAllocatorCreateInfo info{};
info.vulkanApiVersion = VK_API_VERSION_1_3;
info.instance       = instance;
info.physicalDevice = physicalDevice;
info.device         = device;
info.pVulkanFunctions = &vk;
vmaCreateAllocator(&info, &allocator);
```

**Why the `VmaVulkanFunctions` part?** Normally VMA calls Vulkan functions
directly. But we use **volk**, which loads them dynamically — so the direct
symbols don't exist. We compiled VMA with `VMA_DYNAMIC_VULKAN_FUNCTIONS = 1`
(see `vk_backend_impl.cpp`), which means "I'll give you the two loader entry
points; fetch the rest yourself." We hand it volk's
`vkGetInstanceProcAddr`/`vkGetDeviceProcAddr` and it does the rest.

---

## The vertex buffer

A **vertex buffer** holds our triangle's vertices in GPU-accessible memory. Our
vertex format (using GLM types) is:

```cpp
struct Vertex {
    glm::vec2 pos;   // 2D position
    glm::vec3 color; // RGB
};
const std::vector<Vertex> kVertices = {
    {{ 0.0f, -0.5f}, {1,0,0}},   // top,    red
    {{ 0.5f,  0.5f}, {0,1,0}},   // right,  green
    {{-0.5f,  0.5f}, {0,0,1}},   // left,   blue
};
```

> **Coordinate note:** In Vulkan clip space, **+Y points down** and the visible
> range is [-1, 1] on both axes. So `y = -0.5` is *above* the centre. That's why
> the red vertex (`0, -0.5`) is at the top.

### Creating it with VMA

```cpp
VkBufferCreateInfo bufferInfo{};
bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
bufferInfo.size  = sizeof(Vertex) * kVertices.size();
bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;   // "this is vertex data"
bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;     // one queue family uses it

VmaAllocationCreateInfo allocInfo{};
allocInfo.usage = VMA_MEMORY_USAGE_AUTO;   // let VMA choose the memory type
allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
                | VMA_ALLOCATION_CREATE_MAPPED_BIT;

VmaAllocationInfo allocationInfo{};
vmaCreateBuffer(allocator, &bufferInfo, &allocInfo,
                &vertexBuffer, &vertexBufferAllocation, &allocationInfo);
```

`vmaCreateBuffer` does *both* Vulkan steps (create + allocate + bind) in one
call. The flags say:

- `HOST_ACCESS_SEQUENTIAL_WRITE` — "the CPU will write this straight through,
  once", so VMA picks host-visible memory.
- `MAPPED` — "keep it permanently mapped and give me the CPU pointer", saving us
  a manual `vmaMapMemory`/`vmaUnmapMemory` pair.

### Uploading the data

Because we asked for `MAPPED`, VMA already handed us a CPU pointer in
`allocationInfo.pMappedData`. We just copy into it:

```cpp
std::memcpy(allocationInfo.pMappedData, kVertices.data(), bufferInfo.size);
```

Done — the triangle's geometry is now in memory the GPU can read.

### A note on performance

For data that never changes (like this triangle), the *fastest* setup is a
`DEVICE_LOCAL` buffer that the CPU can't touch, filled via a temporary
"staging" buffer + a copy command. We use the simpler host-visible approach here
because it's easier to understand and the performance difference is irrelevant
for three vertices. VMA supports the staging pattern too when you need it.

### Cleanup

One call frees both the buffer and its memory:

```cpp
vmaDestroyBuffer(allocator, vertexBuffer, vertexBufferAllocation);
vmaDestroyAllocator(allocator);
```

Next: [`07_swapchain.md`](07_swapchain.md).
