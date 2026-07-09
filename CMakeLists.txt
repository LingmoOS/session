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

# Find Lingmo dependencies
find_path(LINGMO_IPC_INCLUDE_DIR LingmoIPC/DBusConnection.h
    PATHS /usr/include /usr/local/include)
find_library(LINGMO_IPC_LIBRARY NAMES LingmoIPC
    PATHS /usr/lib /usr/lib/x86_64-linux-gnu /usr/local/lib)
find_path(LINGMO_UTILS_INCLUDE_DIR LingmoUtils/Logger.h
    PATHS /usr/include /usr/local/include)
find_library(LINGMO_UTILS_LIBRARY NAMES LingmoUtils
    PATHS /usr/lib /usr/lib/x86_64-linux-gnu /usr/local/lib)
find_path(LINGMO_CONFIG_INCLUDE_DIR LingmoConfig/Config.h
    PATHS /usr/include /usr/local/include)
find_library(LINGMO_CONFIG_LIBRARY NAMES LingmoConfig
    PATHS /usr/lib /usr/lib/x86_64-linux-gnu /usr/local/lib)

message(STATUS "LingmoIPC: include=${LINGMO_IPC_INCLUDE_DIR} lib=${LINGMO_IPC_LIBRARY}")
message(STATUS "LingmoUtils: include=${LINGMO_UTILS_INCLUDE_DIR} lib=${LINGMO_UTILS_LIBRARY}")
message(STATUS "LingmoConfig: include=${LINGMO_CONFIG_INCLUDE_DIR} lib=${LINGMO_CONFIG_LIBRARY}")

if(NOT LINGMO_IPC_LIBRARY OR NOT LINGMO_UTILS_LIBRARY OR NOT LINGMO_CONFIG_LIBRARY)
    message(FATAL_ERROR "LingmoIPC, LingmoUtils, and LingmoConfig must be installed")
endif()

add_subdirectory(src)

if(BUILD_TESTING AND Qt6Test_FOUND)
    enable_testing()
    add_subdirectory(tests)
endif()
