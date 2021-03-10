cmake_minimum_required(VERSION 3.0.0)
set(TARGET_NAME "SkullSolver")
project(${TARGET_NAME})

add_executable(${TARGET_NAME} main.cpp)

# Use C++20:
target_compile_features(${TARGET_NAME} PRIVATE cxx_std_20)

# Enforce warnings:
if(MSVC)
  target_compile_options(${TARGET_NAME} PRIVATE /W4 /WX)
endif()

# fmt
find_package(fmt CONFIG REQUIRED)
target_link_libraries(${TARGET_NAME} PRIVATE fmt::fmt)