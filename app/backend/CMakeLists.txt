cmake_minimum_required(VERSION 3.21)

project(cpp_webview_backend LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

FetchContent_Declare(
  webview
  GIT_REPOSITORY https://github.com/webview/webview.git
  GIT_TAG 0.12.0
)
FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.12.0
)
FetchContent_Declare(
  cpp_httplib
  GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
  GIT_TAG v0.20.1
)
FetchContent_MakeAvailable(webview)
FetchContent_MakeAvailable(nlohmann_json)
FetchContent_MakeAvailable(cpp_httplib)

add_executable(cpp_webview_host src/app_main.cpp)

target_link_libraries(
  cpp_webview_host
  PRIVATE
    webview::core
    nlohmann_json::nlohmann_json
    httplib::httplib
)

if(WIN32)
  target_link_libraries(cpp_webview_host PRIVATE user32 ole32 shell32 version ws2_32)
endif()

target_compile_definitions(
  cpp_webview_host
  PRIVATE
    FRONTEND_DIST_INDEX="${CMAKE_SOURCE_DIR}/../frontend/dist/index.html"
)
