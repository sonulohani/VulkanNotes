// ============================================================================
//  vk_backend_impl.cpp
//
//  volk and VMA are "single-header" libraries: the header both DECLARES the
//  API and, when a special macro is defined, DEFINES it. Those definitions
//  must live in exactly ONE .cpp file in the whole program, otherwise the
//  linker sees duplicate symbols. This file is that one place.
//
//  Everywhere else we include <volk.h> / <vk_mem_alloc.h> normally, which
//  only pulls in the declarations.
// ============================================================================

// --- volk --------------------------------------------------------------------
// Defining VOLK_IMPLEMENTATION compiles the bodies of volkInitialize(),
// volkLoadInstance(), and all the vkXxx function-pointer globals.
#define VOLK_IMPLEMENTATION
#include <volk.h>

// --- VMA (Vulkan Memory Allocator) ------------------------------------------
// VMA needs to call Vulkan functions. Because we use volk (which loads them
// dynamically), we turn OFF static linking and turn ON the "dynamic" path,
// where VMA fetches the function pointers it needs from the two loader
// entry points we hand it (vkGetInstanceProcAddr / vkGetDeviceProcAddr).
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>
