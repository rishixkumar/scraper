cmake_minimum_required(VERSION 3.20)
project(recall LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(PkgConfig REQUIRED)
pkg_check_modules(POPPLER_CPP REQUIRED IMPORTED_TARGET poppler-cpp)
pkg_check_modules(LIBZIP REQUIRED IMPORTED_TARGET libzip)
pkg_check_modules(PUGIXML REQUIRED IMPORTED_TARGET pugixml)

add_library(recall_core
  src/tokenize/tokenizer.cpp
  src/crawl/ignore.cpp
  src/crawl/crawler.cpp
  src/extract/extractor.cpp
  src/extract/pdf.cpp
  src/extract/office.cpp
)
target_include_directories(recall_core PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(recall_core
  PUBLIC
    PkgConfig::POPPLER_CPP
    PkgConfig::LIBZIP
    PkgConfig::PUGIXML
)

add_executable(recall src/cli/main.cpp)
target_link_libraries(recall PRIVATE recall_core)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.tar.gz
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()
add_executable(recall_tests
  tests/tokenizer_test.cpp
  tests/crawler_test.cpp
  tests/extractor_test.cpp
)
target_link_libraries(recall_tests PRIVATE recall_core GTest::gtest_main)
target_compile_definitions(recall_tests PRIVATE RECALL_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
include(GoogleTest)
gtest_discover_tests(recall_tests)
