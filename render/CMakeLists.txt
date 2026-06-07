find_package(Vulkan REQUIRED)
find_package(vk-bootstrap CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

add_library(render
    src/Swapchain.cpp
    src/Renderer.cpp)

# --- Compile GLSL shaders to SPIR-V -------------------------------------
set(SHADER_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders)
set(SHADER_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/shaders)
file(MAKE_DIRECTORY ${SHADER_OUT_DIR})

set(SHADERS triangle.vert triangle.frag)
set(SPV_FILES)
foreach(shader ${SHADERS})
    add_custom_command(
        OUTPUT  ${SHADER_OUT_DIR}/${shader}.spv
        COMMAND Vulkan::glslc ${SHADER_SRC_DIR}/${shader}
                              -o ${SHADER_OUT_DIR}/${shader}.spv
        DEPENDS ${SHADER_SRC_DIR}/${shader}
        COMMENT "Compiling ${shader} -> ${shader}.spv"
        VERBATIM)
    list(APPEND SPV_FILES ${SHADER_OUT_DIR}/${shader}.spv)
endforeach()

add_custom_target(shaders DEPENDS ${SPV_FILES})
add_dependencies(render shaders)

# Bake the output path so the C++ can find the .spv files at runtime (piece 2).
target_compile_definitions(render PRIVATE SHADER_OUTPUT_DIR="${SHADER_OUT_DIR}")

target_include_directories(render PUBLIC include)
target_link_libraries(render
    PUBLIC  Vulkan::Vulkan gpu
    PRIVATE vk-bootstrap::vk-bootstrap spdlog::spdlog)