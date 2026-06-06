cmake_minimum_required(VERSION 3.20)
project(recon-viewer LANGUAGES CXX)

enable_testing()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(core)
add_subdirectory(io)
add_subdirectory(apps/cli)
add_subdirectory(tests)
add_subdirectory(apps/viewer)
add_subdirectory(gpu)
add_subdirectory(render)