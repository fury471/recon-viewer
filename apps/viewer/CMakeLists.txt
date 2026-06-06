find_package(glfw3 CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

add_executable(viewer main.cpp)
target_link_libraries(viewer PRIVATE glfw gpu render spdlog::spdlog)