cmake_minimum_required(VERSION 3.10)

project(RayTracer)

set(EXECUTABLE_NAME "RayTracer")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)
include_directories(external)

set(SOURCES 
    src/main.cpp
    src/scene_loader.cpp
    src/stb_impl.cpp
)
add_executable(raytracer ${SOURCES})

find_package(OpenMP REQUIRED)
target_link_libraries(raytracer OpenMP::OpenMP_CXX)