# HabitCore: portable, hardware-independent habit logic. Builds anywhere a
# C++17 compiler is available (native tests, the firmware, tooling).
add_library(habitcore STATIC
  src/Date.cpp
  src/Habit.cpp
  src/IconSet.cpp
  src/CompletionLog.cpp
  src/HabitConfig.cpp
  src/Stats.cpp
)
target_include_directories(habitcore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(habitcore PUBLIC cxx_std_17)
