cmake_minimum_required(VERSION 3.1)
project(skooma CXX)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(skooma SHARED
  exports.def
  src/dinput8hook.cpp
  src/dinput8hook.h
  src/dllmain.cpp
  src/gamehook.cpp
  src/gamehook.h
  src/hook.cpp
  src/hook.h
  src/win32.cpp
  src/win32.h)
target_link_libraries(skooma PUBLIC shlwapi ${CONAN_LIBS})
target_include_directories(skooma PUBLIC src)
target_compile_definitions(skooma PUBLIC
  NOMINMAX)
set_target_properties(skooma PROPERTIES
  OUTPUT_NAME dinput8
  LINK_FLAGS /ignore:4104)
