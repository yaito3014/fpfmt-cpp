include(FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2
    GIT_TAG v2.13.10
    GIT_SHALLOW ON
)

# A macro rather than a function, as the CMake documentation recommends for providers,
# so that the variables the request sets reach the find_package caller.
macro(fpfmt_provide_dependency method package_name)
    if("${package_name}" STREQUAL "Catch2")
        # Catch2 v2 declares this with option(), which ignores a plain variable (CMP0077), so set it in the cache.
        set(CATCH_BUILD_STATIC_LIBRARY ON CACHE BOOL "")
        FetchContent_MakeAvailable(Catch2)
        set(Catch2_FOUND ON)
    endif()
endmacro()

cmake_language(
    SET_DEPENDENCY_PROVIDER fpfmt_provide_dependency
    SUPPORTED_METHODS FIND_PACKAGE
)
