option(YK_FPFMT_CI "for CI" OFF)

set(CMAKE_COLOR_DIAGNOSTICS ${YK_FPFMT_CI})

find_package(Catch2 2 REQUIRED)

function(yk_fpfmt_add_test test_name)
    set(target_name yk_fpfmt_${test_name}_test)
    add_executable(${target_name} ${test_name}.cpp)
    target_link_libraries(${target_name} PRIVATE yk::fpfmt)
    target_link_libraries(${target_name} PRIVATE Catch2::Catch2WithMain)
    add_test(NAME ${target_name} COMMAND ${target_name})
endfunction()

yk_fpfmt_add_test(main)

# Every part of the split header compiles on its own, in the library's
# minimum standard: one translation unit per header, including it twice
if(NOT YK_FPFMT_SINGLE_INCLUDE)
    file(
        GLOB_RECURSE YK_FPFMT_PARTS
        RELATIVE ${PROJECT_SOURCE_DIR}/include
        CONFIGURE_DEPENDS
        ${PROJECT_SOURCE_DIR}/include/*.hpp
    )
    set(YK_FPFMT_PART_SOURCES)
    foreach(part IN LISTS YK_FPFMT_PARTS)
        string(MAKE_C_IDENTIFIER ${part} part_name)
        set(source ${CMAKE_CURRENT_BINARY_DIR}/self_contained/${part_name}.cpp)
        file(
            CONFIGURE
            OUTPUT ${source}
            CONTENT "#include <${part}>\n#include <${part}>\n"
        )
        list(APPEND YK_FPFMT_PART_SOURCES ${source})
    endforeach()
    add_library(yk_fpfmt_self_contained OBJECT ${YK_FPFMT_PART_SOURCES})
    set_target_properties(
        yk_fpfmt_self_contained
        PROPERTIES CXX_STANDARD 11 CXX_STANDARD_REQUIRED ON
    )
    target_link_libraries(yk_fpfmt_self_contained PRIVATE yk::fpfmt)
endif()
