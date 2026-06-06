find_package(glfw3 CONFIG REQUIRED)

add_executable(viewer main.cpp)
target_link_libraries(viewer PRIVATE glfw)