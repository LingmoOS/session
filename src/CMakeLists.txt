set(PUBLIC_HEADERS
    ${CMAKE_SOURCE_DIR}/include/LingmoSession/Component.h
    ${CMAKE_SOURCE_DIR}/include/LingmoSession/SessionManager.h
    ${CMAKE_SOURCE_DIR}/include/LingmoSession/EnvironmentSetup.h
    ${CMAKE_SOURCE_DIR}/include/LingmoSession/AutostartManager.h
    ${CMAKE_SOURCE_DIR}/include/LingmoSession/CrashRecovery.h
)
set(PRIVATE_HEADERS
    LingmoSession/private/Component_p.h
    LingmoSession/private/SessionManager_p.h
    LingmoSession/private/AutostartManager_p.h
    LingmoSession/private/CrashRecovery_p.h
)
set(SOURCES
    LingmoSession/Component.cpp
    LingmoSession/SessionManager.cpp
    LingmoSession/EnvironmentSetup.cpp
    LingmoSession/AutostartManager.cpp
    LingmoSession/CrashRecovery.cpp
)
set(CMAKE_AUTOMOC ON)

# Shared library
add_library(LingmoSession SHARED ${SOURCES} ${PUBLIC_HEADERS} ${PRIVATE_HEADERS})
add_library(Lingmo::Session ALIAS LingmoSession)
target_include_directories(LingmoSession
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(LingmoSession
    PUBLIC Qt6::Core Qt6::DBus
    PRIVATE Lingmo::IPC Lingmo::Utils Lingmo::Config
)
target_compile_definitions(LingmoSession PRIVATE LINGMOSESSION_LIBRARY)
set_target_properties(LingmoSession PROPERTIES
    SOVERSION 1 VERSION ${PROJECT_VERSION}
    DEFINE_SYMBOL LINGMOSESSION_LIBRARY OUTPUT_NAME LingmoSession
)

# Executable
add_executable(lingmo-session ${CMAKE_CURRENT_SOURCE_DIR}/LingmoSession/main.cpp)
target_link_libraries(lingmo-session PRIVATE LingmoSession Qt6::Core Qt6::DBus)

configure_file("${CMAKE_SOURCE_DIR}/include/LingmoSession/LingmoSessionExport.h.in"
    "${CMAKE_BINARY_DIR}/include/LingmoSession/LingmoSessionExport.h")
configure_file("${CMAKE_SOURCE_DIR}/include/LingmoSession/LingmoSessionVersion.h.in"
    "${CMAKE_BINARY_DIR}/include/LingmoSession/LingmoSessionVersion.h")

install(TARGETS LingmoSession EXPORT LingmoSessionTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(TARGETS lingmo-session RUNTIME DESTINATION ${CMAKE_INSTALL_LIBEXECDIR})
install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/LingmoSession/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/LingmoSession
    FILES_MATCHING PATTERN "*.h" PATTERN "*.h.in" EXCLUDE)
install(FILES
    "${CMAKE_BINARY_DIR}/include/LingmoSession/LingmoSessionExport.h"
    "${CMAKE_BINARY_DIR}/include/LingmoSession/LingmoSessionVersion.h"
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/LingmoSession)
install(EXPORT LingmoSessionTargets FILE LingmoSessionTargets.cmake
    NAMESPACE Lingmo:: DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/LingmoSession)

install(FILES ${CMAKE_SOURCE_DIR}/systemd/lingmo-session.service
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/systemd/user)
install(FILES ${CMAKE_SOURCE_DIR}/systemd/lingmo-session.target
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/systemd/user)
