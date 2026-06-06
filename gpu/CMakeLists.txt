find_package(Vulkan REQUIRED)
find_package(vk-bootstrap CONFIG REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

add_library(gpu src/Context.cpp)
target_include_directories(gpu PUBLIC include)
target_link_libraries(gpu
    PUBLIC  Vulkan::Vulkan
    PRIVATE vk-bootstrap::vk-bootstrap
            GPUOpen::VulkanMemoryAllocator
            spdlog::spdlog)