cmake_minimum_required(VERSION 3.24)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    if(NOT DEFINED CMAKE_PROJECT_TOP_LEVEL_INCLUDES)
        set(CMAKE_PROJECT_TOP_LEVEL_INCLUDES
            "${CMAKE_CURRENT_LIST_DIR}/cmake/dependencies.cmake"
        )
    endif()
endif()

project(fpfmt VERSION 0.0.1 LANGUAGES CXX)

add_library(fpfmt INTERFACE)

target_compile_features(fpfmt INTERFACE cxx_std_11)

# The option builds and installs the library as one file, generated from
# include/ by tools/amalgamate.py into the build tree, instead of the parts
option(
    YK_FPFMT_SINGLE_INCLUDE
    "use fpfmt as a single header generated from include/ (needs Python 3)"
    OFF
)
if(YK_FPFMT_SINGLE_INCLUDE)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    set(YK_FPFMT_INCLUDE_DIR ${CMAKE_CURRENT_BINARY_DIR}/single_include)
    file(
        GLOB_RECURSE YK_FPFMT_PARTS
        CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp
    )
    # regenerated whenever a part or the script changes, through a reconfigure
    set_property(
        DIRECTORY
        APPEND
        PROPERTY
            CMAKE_CONFIGURE_DEPENDS
                ${YK_FPFMT_PARTS}
                ${CMAKE_CURRENT_SOURCE_DIR}/tools/amalgamate.py
    )
    execute_process(
        COMMAND
            ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tools/amalgamate.py
            ${YK_FPFMT_INCLUDE_DIR}/yk/fpfmt.hpp
        COMMAND_ERROR_IS_FATAL ANY
    )
else()
    set(YK_FPFMT_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
endif()

file(
    GLOB_RECURSE YK_FPFMT_PUBLIC_HEADERS
    CONFIGURE_DEPENDS
    ${YK_FPFMT_INCLUDE_DIR}/*.hpp
)

target_sources(
    fpfmt
    INTERFACE
        FILE_SET fpfmt_public_headers
            TYPE HEADERS
            BASE_DIRS ${YK_FPFMT_INCLUDE_DIR}
            FILES ${YK_FPFMT_PUBLIC_HEADERS}
)

add_library(yk::fpfmt ALIAS fpfmt)

option(YK_FPFMT_BUILD_TESTING "build test" ${PROJECT_IS_TOP_LEVEL})
if(YK_FPFMT_BUILD_TESTING)
    enable_testing()
    add_subdirectory(test)
endif()

option(
    YK_FPFMT_INSTALL
    "generate install rules for fpfmt"
    ${PROJECT_IS_TOP_LEVEL}
)

if(YK_FPFMT_INSTALL)
    include(GNUInstallDirs)
    include(CMakePackageConfigHelpers)

    set(YK_FPFMT_INSTALL_CMAKEDIR ${CMAKE_INSTALL_LIBDIR}/cmake/fpfmt)

    install(
        TARGETS fpfmt
        EXPORT fpfmt-targets
        FILE_SET fpfmt_public_headers DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    install(
        EXPORT fpfmt-targets
        FILE fpfmt-targets.cmake
        NAMESPACE yk::
        DESTINATION ${YK_FPFMT_INSTALL_CMAKEDIR}
    )

    configure_package_config_file(
        cmake/fpfmt-config.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/fpfmt-config.cmake
        INSTALL_DESTINATION ${YK_FPFMT_INSTALL_CMAKEDIR}
    )

    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/fpfmt-config-version.cmake
        COMPATIBILITY SameMajorVersion
        ARCH_INDEPENDENT
    )

    install(
        FILES
            ${CMAKE_CURRENT_BINARY_DIR}/fpfmt-config.cmake
            ${CMAKE_CURRENT_BINARY_DIR}/fpfmt-config-version.cmake
        DESTINATION ${YK_FPFMT_INSTALL_CMAKEDIR}
    )
endif()
