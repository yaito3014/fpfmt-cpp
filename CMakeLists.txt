cmake_minimum_required(VERSION 3.24)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    if(NOT DEFINED CMAKE_PROJECT_TOP_LEVEL_INCLUDES)
        set(CMAKE_PROJECT_TOP_LEVEL_INCLUDES
            "${CMAKE_CURRENT_LIST_DIR}/cmake/dependencies.cmake"
        )
    endif()
endif()

project(fpfmt LANGUAGES CXX)

add_library(fpfmt INTERFACE)

target_compile_features(fpfmt INTERFACE cxx_std_11)

file(GLOB_RECURSE YK_FPFMT_PUBLIC_HEADERS CONFIGURE_DEPENDS include/*.hpp)

target_sources(
    fpfmt
    INTERFACE
        FILE_SET fpfmt_public_headers
            TYPE HEADERS
            BASE_DIRS include
            FILES ${YK_FPFMT_PUBLIC_HEADERS}
)

add_library(yk::fpfmt ALIAS fpfmt)

option(YK_FPFMT_BUILD_TESTING "build test" ${PROJECT_IS_TOP_LEVEL})
if(YK_FPFMT_BUILD_TESTING)
    enable_testing()
    add_subdirectory(test)
endif()
