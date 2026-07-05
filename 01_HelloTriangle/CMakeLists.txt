# ============================================================================
#  01_HelloTriangle - CMake build script
#
#  This build does five things:
#    1. Finds Vulkan, GLFW and GLM on the system.
#    2. Locates the single-header libraries volk and VMA.
#    3. Builds the application from the C++ sources.
#    4. Compiles the GLSL shaders to SPIR-V using glslc.
#    5. Tells the app (via a compile definition) where the .spv files live.
#
#  See docs/01_setup_and_build.md for a line-by-line explanation.
# ============================================================================

cmake_minimum_required(VERSION 3.20)
project(HelloTriangle LANGUAGES CXX)

# ---- C++ standard -----------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Put the executable in the top of the build folder so relative paths are tidy.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# ---- Dependencies -----------------------------------------------------------
# Vulkan::Vulkan          -> headers + loader, and the Vulkan::glslc tool.
# glfw                    -> windowing / surface creation.
# glm::glm                -> header-only maths (vectors, matrices).
find_package(Vulkan REQUIRED)
find_package(glfw3 3.3 REQUIRED)
find_package(glm REQUIRED)

# volk and VMA ship as single headers (installed under /usr/include here).
# We only need their include directories; the implementation is compiled in
# src/vk_backend_impl.cpp via the *_IMPLEMENTATION macros.
find_path(VOLK_INCLUDE_DIR NAMES volk.h REQUIRED)
find_path(VMA_INCLUDE_DIR  NAMES vk_mem_alloc.h REQUIRED)

# ---- Executable -------------------------------------------------------------
add_executable(HelloTriangle
    src/main.cpp
    src/vk_backend_impl.cpp
)

target_include_directories(HelloTriangle PRIVATE
    ${VOLK_INCLUDE_DIR}
    ${VMA_INCLUDE_DIR}
)

# NOTE ON LINKING WITH VOLK:
#   volk loads Vulkan function pointers at runtime via dlopen, so we do not
#   strictly need to link the Vulkan loader. We still link Vulkan::Vulkan for
#   its include directories, and ${CMAKE_DL_LIBS} gives volk access to dlopen.
target_link_libraries(HelloTriangle PRIVATE
    Vulkan::Vulkan
    glfw
    glm::glm
    ${CMAKE_DL_LIBS}
)

# ---- Shader compilation (GLSL -> SPIR-V) ------------------------------------
set(SHADER_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders)
set(SHADER_BIN_DIR ${CMAKE_CURRENT_BINARY_DIR}/shaders)
file(MAKE_DIRECTORY ${SHADER_BIN_DIR})

set(SHADERS
    triangle.vert
    triangle.frag
)

set(SPV_OUTPUTS "")
foreach(shader ${SHADERS})
    set(src ${SHADER_SRC_DIR}/${shader})
    set(spv ${SHADER_BIN_DIR}/${shader}.spv)
    add_custom_command(
        OUTPUT ${spv}
        COMMAND Vulkan::glslc ${src} -o ${spv}
        DEPENDS ${src}
        COMMENT "Compiling shader ${shader} -> ${shader}.spv"
        VERBATIM
    )
    list(APPEND SPV_OUTPUTS ${spv})
endforeach()

add_custom_target(shaders ALL DEPENDS ${SPV_OUTPUTS})
add_dependencies(HelloTriangle shaders)

# Tell the C++ code where to find the compiled shaders at runtime.
target_compile_definitions(HelloTriangle PRIVATE
    SHADER_DIR="${SHADER_BIN_DIR}"
)
