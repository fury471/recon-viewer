add_library(io
    src/XyzReader.cpp
)

target_include_directories(io PUBLIC include)
target_link_libraries(io PUBLIC core)