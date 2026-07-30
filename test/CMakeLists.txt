cmake_minimum_required(VERSION 3.16)
project(habitink_tests CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_compile_options(-Wall -Wextra)

# Portable libraries under test.
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../lib/HabitCore ${CMAKE_BINARY_DIR}/HabitCore)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../lib/HabitUI ${CMAKE_BINARY_DIR}/HabitUI)

enable_testing()

# A single test binary keeps the harness trivial; tinytest registers every
# TEST() across all translation units. The firmware controller is portable
# (it only touches the Hal interfaces), so it is compiled and tested here too.
add_executable(habitink_tests
  test_main.cpp
  habitcore/test_date.cpp
  habitcore/test_config.cpp
  habitcore/test_log_stats.cpp
  habitui/test_canvas.cpp
  habitui/test_app.cpp
  firmware/test_controller.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../src/HabitInkController.cpp
)
target_include_directories(habitink_tests PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/firmware
  ${CMAKE_CURRENT_SOURCE_DIR}/..  # so "src/..." includes resolve
)
target_link_libraries(habitink_tests PRIVATE habitcore habitui)

add_test(NAME habitink_tests COMMAND habitink_tests)
