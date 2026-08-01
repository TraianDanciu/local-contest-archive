cmake_minimum_required(VERSION 3.20)

project(lca)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(lca
  src/main.cpp
  src/cli/cli.cpp
  src/commands/archive.cpp
)

target_include_directories(lca PRIVATE src)