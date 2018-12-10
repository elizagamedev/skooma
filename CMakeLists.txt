cmake_minimum_required(VERSION 3.1)
project(skooma CXX)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(skooma
  src/Skooma.cpp
  src/Skooma.h
  src/WindowsHook.cpp
  src/WindowsHook.h
  src/main.cpp
  src/win32.cpp
  src/win32.h
  skooma.rc)
target_link_libraries(skooma PUBLIC ${CONAN_LIBS})
target_include_directories(skooma PUBLIC src)
