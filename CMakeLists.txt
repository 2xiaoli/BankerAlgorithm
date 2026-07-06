cmake_minimum_required(VERSION 3.31)
project(program)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (MSVC)
    add_compile_options(/utf-8)
elseif (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-finput-charset=UTF-8 -fexec-charset=UTF-8)
endif ()

add_executable(program
        main.cpp
        Encoding.cpp
        BankerSystem.cpp
        CommandParser.cpp
        ConsoleUI.cpp
)
