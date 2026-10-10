cmake_minimum_required(VERSION 3.20)

project(
    Blaze
    VERSION 1.0.0
    DESCRIPTION "Photo and Video Sorting Application"
    LANGUAGES CXX
)

include(FetchContent)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ============================================================
# C++ Standard
# ============================================================

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ============================================================
# Compiler Warnings
# ============================================================

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
    )
endif()

# ============================================================
# Dependencies
# ============================================================

find_package(OpenSSL REQUIRED)

find_package(SQLite3 REQUIRED)

# ============================================================
# nlohmann/json
# ============================================================

FetchContent_Declare(
    nlohmann_json
    URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
)

FetchContent_MakeAvailable(nlohmann_json)

# ============================================================
# GoogleTest
# ============================================================

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/releases/download/v1.17.0/googletest-1.17.0.tar.gz
)

FetchContent_MakeAvailable(googletest)

FetchContent_Declare(
    drogon
    GIT_REPOSITORY https://github.com/drogonframework/drogon.git
    GIT_TAG v1.9.11
)

set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(TRANTOR_BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(BUILD_YAML_CONFIG OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(drogon)

if(TARGET trantor)
    target_compile_options(
        trantor
        PRIVATE
        -Wno-error=unused-result
    )
endif()

# ============================================================
# Backend Source Files
# ============================================================

file(
    GLOB_RECURSE
    SOURCES
    CONFIGURE_DEPENDS
    backend/src/*.cpp
)

# ============================================================
# Blaze Application
# ============================================================

add_executable(
    Blaze
    ${SOURCES}
)

target_include_directories(
    Blaze
    PRIVATE
    backend/include
)

target_link_libraries(
    Blaze
    PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
    SQLite::SQLite3
    nlohmann_json::nlohmann_json
    drogon
)

# ============================================================
# Blaze Tests
# ============================================================

add_executable(
    BlazeTest
    tests/DestinationPathGeneratorTests.cpp
    tests/HashGeneratorTests.cpp
    tests/ScannerTests.cpp
    tests/FileCopierTests.cpp
    tests/ConfigManagerTests.cpp
    tests/MetaDataReaderTests.cpp
    tests/DatabaseTests.cpp
    tests/SortEngineTests.cpp
    tests/SortingControllerTests.cpp
    tests/ApiServerTests.cpp

    backend/src/DestinationPathGenerator.cpp
    backend/src/HashGenerator.cpp
    backend/src/Scanner.cpp
    backend/src/FileCopier.cpp
    backend/src/ConfigManager.cpp
    backend/src/MetaDataReader.cpp
    backend/src/Database.cpp
    backend/src/SortEngine.cpp
    backend/src/Logger.cpp
    backend/src/SortingState.cpp
    backend/src/SortingController.cpp
    backend/src/ApiServer.cpp
)

target_include_directories(
    BlazeTest
    PRIVATE
    backend/include
)

target_link_libraries(
    BlazeTest
    PRIVATE
    GTest::gtest_main
    OpenSSL::SSL
    OpenSSL::Crypto
    SQLite::SQLite3
    nlohmann_json::nlohmann_json
    drogon
)

# ============================================================
# Testing
# ============================================================

enable_testing()

include(GoogleTest)

gtest_discover_tests(
    BlazeTest
)