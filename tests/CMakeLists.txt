project(wyagit_test)
cmake_minimum_required(VERSION 3.20)

include(FetchContent)
include(GoogleTest)

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest
  GIT_TAG b796f7d
)
FetchContent_MakeAvailable(googletest)

add_executable(wyagitTest wyagitTests.cpp)
target_link_libraries(wyagitTest  gtest
                                  ${CMAKE_BINARY_DIR}/lib${WYAGIT}.a
                                  ${Boost_LIBRARIES}
                                  fmt)

enable_testing()
gtest_discover_tests(wyagitTest)
