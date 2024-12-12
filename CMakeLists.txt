cmake_minimum_required(VERSION 3.10)

project(MySystemCProject)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(SystemC_INCLUDE_DIRS "../systemc/src")
set(SystemC_LIBRARY_DIRS "../systemc/build/src")
set(SystemC_LIBRARIES systemc)

add_executable(MySystemCExecutable src/main.cpp src/cpu.cpp src/mem.cpp
        src/pe_core.cpp
        src/pe_core.h)

target_link_libraries(MySystemCExecutable ${SystemC_LIBRARIES})

target_include_directories(MySystemCExecutable PRIVATE "./src")
target_include_directories(MySystemCExecutable PRIVATE ${SystemC_INCLUDE_DIRS})

target_link_directories(MySystemCExecutable PRIVATE ${SystemC_LIBRARY_DIRS})



