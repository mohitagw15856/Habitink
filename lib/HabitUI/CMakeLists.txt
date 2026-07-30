# HabitUI: portable rendering (software 1-bit rasteriser + embedded assets) and
# the hardware-free application/flow layer. Depends only on HabitCore.
add_library(habitui STATIC
  src/Font8x13.cpp
  src/Icons.cpp
  src/FrameCanvas.cpp
  src/HabitApp.cpp
)
target_include_directories(habitui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(habitui PUBLIC habitcore)
target_compile_features(habitui PUBLIC cxx_std_17)
