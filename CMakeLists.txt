cmake_minimum_required(VERSION 3.21)
cmake_policy(SET CMP0077 NEW)
cmake_policy(SET CMP0092 NEW)

project(LingmoSession
    VERSION 1.0.0
    DESCRIPTION "Lingmo Desktop - Session manager"
    HOMEPAGE_URL "https://lingmo.org"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
include(GNUInstallDirs)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(LingmoCompilerSettings)
include(LingmoSanitizers)

option(BUILD_TESTING "Build tests" ON)

find_package(Qt6 REQUIRED COMPONENTS Core DBus)
find_package(Qt6 COMPONENTS Test QUIET)
find_package(LingmoIPC QUIET)
find_package(LingmoUtils QUIET)
find_package(LingmoConfig QUIET)

if(NOT TARGET Lingmo::IPC)
    message(WARNING "LingmoIPC not found - IPC features will be stubbed")
    set(LINGMO_IPC_FOUND FALSE)
else()
    set(LINGMO_IPC_FOUND TRUE)
endif()

add_subdirectory(src)

if(BUILD_TESTING AND Qt6Test_FOUND)
    enable_testing()
    add_subdirectory(tests)
endif()
