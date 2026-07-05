# 01 · Setup & Build

Before touching Vulkan, let's understand the tools and how the project is put
together.

## The libraries, one by one

### Vulkan
The graphics API itself. On Linux it comes from the **Vulkan SDK** (or your
distro's `vulkan-headers`, `vulkan-loader`, and `vulkan-validationlayers`
packages). It gives us:
- the headers (`vulkan/vulkan.h`),
- the *loader* (`libvulkan.so`) that finds your GPU driver,
- `glslc`, the shader compiler,
- the validation layers.

### GLFW
Creating a window, handling resize, and getting keyboard/mouse events is
different on Windows, Linux (X11/Wayland), and macOS. GLFW hides all of that
behind one small API. It also knows how to make a Vulkan **surface** for its
windows. We tell it *not* to create an OpenGL context (`GLFW_NO_API`) because we
drive the GPU with Vulkan.

### GLM (OpenGL Mathematics)
A header-only C++ math library whose types (`glm::vec2`, `glm::vec3`, `glm::mat4`)
mirror GLSL. We use it here for our vertex positions and colors. In bigger
programs you'll use it for model/view/projection matrices.

### volk
The Vulkan loader normally routes every function call through a small dispatch
step. **volk** loads the function *pointers* directly, which is faster and lets
us use the newest extensions cleanly. It's a single header. See
[`02_instance_and_volk.md`](02_instance_and_volk.md).

### VMA (Vulkan Memory Allocator)
GPU memory in Vulkan is low-level: you must find the right memory *type*, manage
alignment, and sub-allocate to avoid hitting driver limits. VMA is AMD's
battle-tested library that does all of this for you with a couple of function
calls. See [`06_vma_and_buffers.md`](06_vma_and_buffers.md).

---

## Project layout

```
01_HelloTriangle/
├── CMakeLists.txt          # build script (explained below)
├── README.md               # quick start
├── src/
│   ├── main.cpp            # the entire application
│   └── vk_backend_impl.cpp # volk + VMA "implementation" TU
├── shaders/
│   ├── triangle.vert       # vertex shader (GLSL)
│   └── triangle.frag       # fragment shader (GLSL)
└── docs/                   # you are here
```

### Why is `vk_backend_impl.cpp` separate?

volk and VMA are **single-header libraries**. Their `.h` file contains both the
*declarations* (used everywhere) and the *definitions* (the actual code). You
enable the definitions by defining a macro like `VMA_IMPLEMENTATION` before
including the header — but you must do that in **exactly one** `.cpp` file,
otherwise the linker complains about duplicate symbols. That one file is
`vk_backend_impl.cpp`:

```cpp
#define VOLK_IMPLEMENTATION
#include <volk.h>

#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>
```

Everywhere else (`main.cpp`) we include the same headers *without* the macros,
so we only get the declarations.

---

## Understanding `CMakeLists.txt`

CMake is a "meta build system": it generates the actual build files (Makefiles,
Ninja, etc.) for your platform. Walking through ours:

```cmake
find_package(Vulkan REQUIRED)     # gives Vulkan::Vulkan and Vulkan::glslc
find_package(glfw3 3.3 REQUIRED)  # gives the 'glfw' target
find_package(glm REQUIRED)        # gives glm::glm
```

`find_package` locates a library installed on your system and hands you a
"target" you can link against. Linking a target automatically pulls in its
include directories too.

```cmake
find_path(VOLK_INCLUDE_DIR NAMES volk.h REQUIRED)
find_path(VMA_INCLUDE_DIR  NAMES vk_mem_alloc.h REQUIRED)
```

volk and VMA are just headers, so we only need to find where they live.

```cmake
add_executable(HelloTriangle src/main.cpp src/vk_backend_impl.cpp)
target_link_libraries(HelloTriangle PRIVATE
    Vulkan::Vulkan glfw glm::glm ${CMAKE_DL_LIBS})
```

`${CMAKE_DL_LIBS}` is the library that provides `dlopen` — volk needs it to load
`libvulkan.so` at runtime.

> **A note on linking Vulkan with volk:** because volk loads Vulkan at runtime,
> you technically don't *need* to link the Vulkan loader. We still link
> `Vulkan::Vulkan` for convenience (it provides the headers and the `glslc`
> tool). This is harmless because we build with `VK_NO_PROTOTYPES` (volk sets
> this), so no duplicate function symbols exist.

### Compiling shaders during the build

GPUs don't read GLSL text; they read **SPIR-V** bytecode. We compile our
`.vert`/`.frag` files to `.spv` at build time:

```cmake
add_custom_command(
    OUTPUT ${spv}
    COMMAND Vulkan::glslc ${src} -o ${spv}
    DEPENDS ${src})
```

This runs `glslc triangle.vert -o triangle.vert.spv`. The
`add_dependencies(HelloTriangle shaders)` line makes sure shaders are compiled
before the app is built.

Finally we pass the output folder to the C++ code:

```cmake
target_compile_definitions(HelloTriangle PRIVATE SHADER_DIR="${SHADER_BIN_DIR}")
```

So `main.cpp` can do `readFile(std::string(SHADER_DIR) + "/triangle.vert.spv")`
and always find the shaders regardless of where you run the binary from.

---

## Build & run

```bash
# from 01_HelloTriangle/
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
./build/HelloTriangle
```

- **Debug** build → validation layers are ON (great while learning).
- **Release** build (`-DCMAKE_BUILD_TYPE=Release`) → validation OFF, faster.

You should see a window with the triangle, and in the terminal:

```
Using GPU: <your GPU name>
```

If something is wrong, the validation layers will print a `[validation]` message
explaining exactly what. Read those carefully — they are your best friend.

Next: [`02_instance_and_volk.md`](02_instance_and_volk.md).
