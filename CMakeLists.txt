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

# Find Lingmo dependencies (optional — Layer 3 libs may not be installed in CI)
execute_process(COMMAND dpkg-architecture -qDEB_HOST_MULTIARCH
    OUTPUT_VARIABLE DEB_HOST_MULTIARCH OUTPUT_STRIP_TRAILING_WHITESPACE)

find_path(LINGMO_IPC_INCLUDE_DIR LingmoIPC/DBusConnection.h QUIET)
find_library(LINGMO_IPC_LIBRARY NAMES LingmoIPC
    PATHS /usr/lib/${DEB_HOST_MULTIARCH} /usr/lib /usr/local/lib QUIET)
find_path(LINGMO_UTILS_INCLUDE_DIR LingmoUtils/Logger.h QUIET)
find_library(LINGMO_UTILS_LIBRARY NAMES LingmoUtils
    PATHS /usr/lib/${DEB_HOST_MULTIARCH} /usr/lib /usr/local/lib QUIET)
find_path(LINGMO_CONFIG_INCLUDE_DIR LingmoConfig/Config.h QUIET)
find_library(LINGMO_CONFIG_LIBRARY NAMES LingmoConfig
    PATHS /usr/lib/${DEB_HOST_MULTIARCH} /usr/lib /usr/local/lib QUIET)

set(LINGMO_IPC_FOUND FALSE)
set(LINGMO_UTILS_FOUND FALSE)
set(LINGMO_CONFIG_FOUND FALSE)
if(LINGMO_IPC_LIBRARY AND LINGMO_IPC_INCLUDE_DIR)
    set(LINGMO_IPC_FOUND TRUE)
endif()
if(LINGMO_UTILS_LIBRARY AND LINGMO_UTILS_INCLUDE_DIR)
    set(LINGMO_UTILS_FOUND TRUE)
endif()
if(LINGMO_CONFIG_LIBRARY AND LINGMO_CONFIG_INCLUDE_DIR)
    set(LINGMO_CONFIG_FOUND TRUE)
endif()

message(STATUS "LingmoIPC: found=${LINGMO_IPC_FOUND}")
message(STATUS "LingmoUtils: found=${LINGMO_UTILS_FOUND}")
message(STATUS "LingmoConfig: found=${LINGMO_CONFIG_FOUND}")

if(NOT LINGMO_IPC_FOUND OR NOT LINGMO_UTILS_FOUND OR NOT LINGMO_CONFIG_FOUND)
    message(WARNING "LingmoIPC, LingmoUtils, or LingmoConfig not found — building without Layer 3 dependencies")
endif()

add_subdirectory(src)

if(BUILD_TESTING AND Qt6Test_FOUND)
    enable_testing()
    add_subdirectory(tests)
endif()
