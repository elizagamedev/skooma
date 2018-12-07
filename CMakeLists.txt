cmake_minimum_required(VERSION 3.1)
project(skooma CXX)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(skooma_dll SHARED
  dll/IDirect3DDevice9Vtbl.h
  dll/WindowsHook.cpp
  dll/WindowsHook.h
  dll/dllmain.cpp
  dll/prochook.cpp
  dll/prochook.h
  dll/win32.cpp
  dll/win32.h)
target_link_libraries(skooma_dll PUBLIC shlwapi ${CONAN_LIBS})
target_include_directories(skooma_dll PUBLIC dll)
target_compile_definitions(skooma_dll PUBLIC NOMINMAX)
set_target_properties(skooma_dll PROPERTIES
  OUTPUT_NAME skooma
  WINDOWS_EXPORT_ALL_SYMBOLS ON)

add_executable(skooma_exe
  exe/Menu.cpp
  exe/Menu.h
  exe/NotificationIcon.cpp
  exe/NotificationIcon.h
  exe/Skooma.cpp
  exe/Skooma.h
  exe/main.cpp
  skooma.rc)
target_link_libraries(skooma_exe PUBLIC skooma_dll)
target_include_directories(skooma_exe PUBLIC exe)
set_target_properties(skooma_exe PROPERTIES
  OUTPUT_NAME skooma)
