cmake_minimum_required(VERSION 3.31)

project(cpp_saucer_backend LANGUAGES CXX)

add_executable(cpp_saucer_host src/main.cpp)
target_compile_features(cpp_saucer_host PRIVATE cxx_std_23)
set_target_properties(
  cpp_saucer_host
  PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

include(FetchContent)

FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.12.0
)
FetchContent_Declare(
  saucer
  GIT_REPOSITORY "https://github.com/saucer/saucer"
  GIT_TAG v8.0.5
)
FetchContent_MakeAvailable(nlohmann_json)
FetchContent_MakeAvailable(saucer)

saucer_embed("../frontend/dist" TARGET cpp_saucer_host DESTINATION "${CMAKE_BINARY_DIR}/embedded")

target_link_libraries(
  cpp_saucer_host
  PRIVATE
    saucer::saucer
    saucer::embedded
    nlohmann_json::nlohmann_json
)

