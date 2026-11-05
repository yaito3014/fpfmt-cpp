include(CTest)
include(FetchContent)

# Catch2 v2 declares this with option(), which ignores a plain variable (CMP0077), so set it in the cache.
set(CATCH_BUILD_STATIC_LIBRARY ON CACHE BOOL "")
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2
  GIT_TAG v2.13.10
  GIT_SHALLOW ON
)
FetchContent_MakeAvailable(Catch2)

add_executable(fpfmt_test test.cpp)
target_link_libraries(fpfmt_test fpfmt Catch2::Catch2WithMain)

add_test(NAME fmfmt_test COMMAND fpfmt_test)
