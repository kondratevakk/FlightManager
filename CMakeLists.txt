cmake_minimum_required(VERSION 3.12)

project(
    labwork6
    VERSION 1.0
    DESCRIPTION "C++ Year 2025 labwork6"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)


include(FetchContent)
FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(json)

FetchContent_Declare(
    cpr
    GIT_REPOSITORY https://github.com/libcpr/cpr.git
    GIT_TAG 1.11.1
)
FetchContent_MakeAvailable(cpr)

add_executable(main main.cpp)

target_include_directories(main PRIVATE ${json_SOURCE_DIR}/include)

target_link_libraries(main PRIVATE cpr::cpr)
