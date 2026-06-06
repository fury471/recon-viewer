find_package(Vulkan REQUIRED)
find_package(vk-bootstrap CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

add_library(render
    src/Swapchain.cpp
    src/Renderer.cpp)

target_include_directories(render PUBLIC include)
target_link_libraries(render
    PUBLIC  Vulkan::Vulkan gpu
    PRIVATE vk-bootstrap::vk-bootstrap spdlog::spdlog)