cmake_minimum_required(VERSION 3.20)

project(lca)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(lca
  src/main.cpp
  src/cli/cli.cpp
  src/commands/codeforces.cpp
  src/commands/atcoder.cpp
  src/commands/convert.cpp
  src/commands/oi.cpp
  src/commands/work.cpp
  src/http/http.cpp
  src/providers/codeforces.cpp
  src/providers/atcoder.cpp
  src/providers/oi.cpp
  src/archive/archive.cpp
  src/convert/convert.cpp
)

target_include_directories(lca PRIVATE src)

find_package(CURL REQUIRED)
find_package(nlohmann_json REQUIRED)

target_link_libraries(lca PRIVATE CURL::libcurl nlohmann_json::nlohmann_json)

target_compile_definitions(lca PRIVATE "LCA_TOOLS_DIR=\"${CMAKE_SOURCE_DIR}/tools\"")