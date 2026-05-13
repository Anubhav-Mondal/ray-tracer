cmake_minimum_required(VERSION 3.10)

project(RayTracer)

set(EXECUTABLE_NAME "RayTracer")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(OIDN_DIR "${CMAKE_SOURCE_DIR}/external/oidn")

set(SOURCES 
    src/main.cpp
    src/scene_loader.cpp
    src/stb_impl.cpp
)
add_executable(${EXECUTABLE_NAME} ${SOURCES})

target_include_directories(${EXECUTABLE_NAME} PRIVATE
    include
    external
    "${OIDN_DIR}/include"
)

find_package(OpenMP REQUIRED)
target_link_libraries(${EXECUTABLE_NAME} OpenMP::OpenMP_CXX)

if(WIN32)
    target_link_libraries(${EXECUTABLE_NAME}
        "${OIDN_DIR}/lib/OpenImageDenoise.lib"
        "${OIDN_DIR}/lib/OpenImageDenoise_core.lib"
    )
    file(GLOB OIDN_DLLS "${OIDN_DIR}/bin/*.dll")
    foreach(DLL ${OIDN_DLLS})
        add_custom_command(TARGET ${EXECUTABLE_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${DLL}" $<TARGET_FILE_DIR:${EXECUTABLE_NAME}>
        )
    endforeach()
else()
    target_link_libraries(${EXECUTABLE_NAME}
        "${OIDN_DIR}/lib/libOpenImageDenoise.so"
    )
endif()