cmake_minimum_required(VERSION 3.14)

set(WARNING_FLAGS
  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Werror -Wundef>
  $<$<CXX_COMPILER_ID:GNU>:-Wuseless-cast -Wmaybe-uninitialized>
  $<$<CXX_COMPILER_ID:MSVC>:/W4 /permissive->
)

set(PROJECT_NAME
  order_book_v1
)

project(${PROJECT_NAME} LANGUAGES C CXX)

add_executable(clob_cli
  src/main.cpp
)

add_library(orderbook
  SHARED
  src/orderbook.cpp
)

include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        52eb8108c5bdec04579160ae17225d66034bd723
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(orderbook_test
  tests/orderbook_test.cpp
)

target_link_libraries(orderbook_test
  PRIVATE
  GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(orderbook_test)

set(SAN_FLAGS
  -fsanitize=address,undefined
  -fno-omit-frame-pointer
  -g
)

add_library(project_defaults INTERFACE)

target_compile_features(project_defaults
  INTERFACE
  cxx_std_20
)
target_compile_options(project_defaults
  INTERFACE
  ${WARNING_FLAGS}
  $<$<CONFIG:Debug>:${SAN_FLAGS}>
)
target_link_options(project_defaults
  INTERFACE
  $<$<CONFIG:Debug>:${SAN_FLAGS}>
)

target_link_libraries(clob_cli PRIVATE project_defaults)
target_link_libraries(orderbook PRIVATE project_defaults)
target_link_libraries(orderbook_test PRIVATE project_defaults)
