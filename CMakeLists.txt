cmake_minimum_required(VERSION 3.24)

project(fpfmt-cpp LANGUAGES CXX)

add_library(fpfmt INTERFACE)

target_sources(fpfmt INTERFACE FILE_SET yk_fpfmt_headers TYPE HEADERS BASE_DIRS include FILES include/yk/fpfmt.hpp)

target_compile_features(fpfmt INTERFACE cxx_std_11)

option(YK_FPFMT_BUILD_TESTING "build testing" ${PROJECT_IS_TOP_LEVEL})

if(YK_FPFMT_BUILD_TESTING)
  add_subdirectory(test)
endif()
