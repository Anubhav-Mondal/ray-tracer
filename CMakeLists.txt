cmake_minimum_required(VERSION 3.10)

project(RayTracer)

set(EXECUTABLE_NAME "RayTracer")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)
include_directories(external)

set(OIDN_DIR "${CMAKE_SOURCE_DIR}/external/oidn")
include_directories("${OIDN_DIR}/include")

set(SOURCES 
    src/main.cpp
    src/scene_loader.cpp
    src/stb_impl.cpp
)
add_executable(raytracer ${SOURCES})

find_package(OpenMP REQUIRED)
target_link_libraries(raytracer OpenMP::OpenMP_CXX)

if(WIN32)
    target_link_libraries(raytracer
        "${OIDN_DIR}/lib/OpenImageDenoise.lib"
        "${OIDN_DIR}/lib/OpenImageDenoise_core.lib"
    )
    file(GLOB OIDN_DLLS "${OIDN_DIR}/bin/*.dll")
    foreach(DLL ${OIDN_DLLS})
        add_custom_command(TARGET raytracer POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${DLL}" $<TARGET_FILE_DIR:raytracer>
        )
    endforeach()
else()
    target_link_libraries(raytracer
        "${OIDN_DIR}/lib/libOpenImageDenoise.so"
    )
endif()