# SPDX-License-Identifier: GPL-3.0-or-later
cmake_minimum_required(VERSION 3.20)

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" JAGLINK_PROJECT_VERSION)
string(STRIP "${JAGLINK_PROJECT_VERSION}" JAGLINK_PROJECT_VERSION)
if(NOT JAGLINK_PROJECT_VERSION MATCHES "^[0-9]+\.[0-9]+\.[0-9]+$")
    message(FATAL_ERROR "VERSION must contain a semantic major.minor.patch version.")
endif()

project(JAGLINK VERSION "${JAGLINK_PROJECT_VERSION}" LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(JAGLINK_ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
option(JAGLINK_BUILD_LINUX_APP "Build the native GTK4 Linux front end" OFF)
if(WIN32)
    set(JAGLINK_WINDOWS_DISCOVER_DEFAULT ON)
else()
    set(JAGLINK_WINDOWS_DISCOVER_DEFAULT OFF)
endif()
option(JAGLINK_BUILD_WINDOWS_DISCOVER "Build the Win32 OpenPort/J2534 discovery GUI" ${JAGLINK_WINDOWS_DISCOVER_DEFAULT})

if(JAGLINK_ENABLE_SANITIZERS)
    if(NOT CMAKE_C_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
        message(FATAL_ERROR "JAGLINK_ENABLE_SANITIZERS requires GCC or Clang.")
    endif()
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()

include(CTest)
include(GNUInstallDirs)

# LINK is JAGLINK's only top-level implementation dependency. LINK owns and
# validates Infiltratr Common, preserving Common -> LINK -> product.
set(JAGLINK_LINK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/link")
set(JAGLINK_LINK_VERSION "0.7.1")
set(JAGLINK_LINK_COMMIT "4c75618da2085ecd09e8110f2883decf9b2162f9")

if(NOT EXISTS "${JAGLINK_LINK_DIR}/VERSION")
    message(FATAL_ERROR "LINK ${JAGLINK_LINK_VERSION} is required. Clone with --recurse-submodules or run git submodule update --init --recursive.")
endif()

file(READ "${JAGLINK_LINK_DIR}/VERSION" JAGLINK_LINK_FOUND_VERSION)
string(STRIP "${JAGLINK_LINK_FOUND_VERSION}" JAGLINK_LINK_FOUND_VERSION)
if(NOT JAGLINK_LINK_FOUND_VERSION STREQUAL JAGLINK_LINK_VERSION)
    message(FATAL_ERROR "LINK ${JAGLINK_LINK_VERSION} is required; found ${JAGLINK_LINK_FOUND_VERSION}.")
endif()

find_program(JAGLINK_GIT_EXECUTABLE git)
if(JAGLINK_GIT_EXECUTABLE)
    execute_process(COMMAND "${JAGLINK_GIT_EXECUTABLE}" -C "${JAGLINK_LINK_DIR}" rev-parse HEAD
        OUTPUT_VARIABLE JAGLINK_LINK_FOUND_COMMIT OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET RESULT_VARIABLE JAGLINK_LINK_COMMIT_RESULT)
    if(JAGLINK_LINK_COMMIT_RESULT EQUAL 0 AND NOT JAGLINK_LINK_FOUND_COMMIT STREQUAL JAGLINK_LINK_COMMIT)
        message(FATAL_ERROR "LINK must be pinned to ${JAGLINK_LINK_COMMIT}; found ${JAGLINK_LINK_FOUND_COMMIT}.")
    endif()
endif()

add_subdirectory("${JAGLINK_LINK_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/link" EXCLUDE_FROM_ALL)

add_library(jaglink STATIC
    src/core/jaglink.c
    src/core/parameter.c
    src/core/scheduler.c
    src/core/telemetry_store.c
    src/core/telemetry_csv.c
    src/elm327/elm327.c
    src/elm327/can.c
    src/elm327/session.c
    src/elm327/probe.c
    src/obd2/obd2.c
    src/uds/uds.c
    src/jaguar/jaguar.c)

add_library(JAGLINK::Core ALIAS jaglink)
target_include_directories(jaglink PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_compile_definitions(jaglink PRIVATE JAGLINK_VERSION="${PROJECT_VERSION}")
target_link_libraries(jaglink PUBLIC LINK::Core)

set(JAGLINK_WARNING_FLAGS -Wall -Wextra -Wpedantic -Wshadow -Wformat=2 -Wstrict-prototypes -Wmissing-prototypes)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    target_compile_options(jaglink PRIVATE ${JAGLINK_WARNING_FLAGS} -Werror)
endif()

if(JAGLINK_BUILD_LINUX_APP)
    if(NOT UNIX OR APPLE)
        message(FATAL_ERROR "JAGLINK_BUILD_LINUX_APP is supported on Linux only.")
    endif()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GTK4 REQUIRED IMPORTED_TARGET gtk4>=4.6)
    find_program(GLIB_COMPILE_RESOURCES_EXECUTABLE NAMES glib-compile-resources REQUIRED)

    set(JAGLINK_LINUX_RESOURCE_SOURCE "${CMAKE_CURRENT_BINARY_DIR}/jaglink-linux-resources.c")
    add_custom_command(
        OUTPUT "${JAGLINK_LINUX_RESOURCE_SOURCE}"
        COMMAND "${GLIB_COMPILE_RESOURCES_EXECUTABLE}"
            --generate-source
            "--target=${JAGLINK_LINUX_RESOURCE_SOURCE}"
            jaglink.gresource.xml
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/app/linux"
        DEPENDS
            "${CMAKE_CURRENT_SOURCE_DIR}/app/linux/jaglink.gresource.xml"
            "${CMAKE_CURRENT_SOURCE_DIR}/app/linux/assets/jaglink-emblem.png"
        VERBATIM)

    add_executable(jaglink-linux
        app/linux/main.c
        app/linux/about-dialog.c
        "${JAGLINK_LINUX_RESOURCE_SOURCE}")
    set_source_files_properties("${JAGLINK_LINUX_RESOURCE_SOURCE}" PROPERTIES COMPILE_OPTIONS "-w")
    set_target_properties(jaglink-linux PROPERTIES OUTPUT_NAME jaglink)
    target_link_libraries(jaglink-linux PRIVATE jaglink PkgConfig::GTK4)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(jaglink-linux PRIVATE ${JAGLINK_WARNING_FLAGS} -Werror)
    endif()
    install(TARGETS jaglink-linux RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
    install(FILES app/linux/assets/jaglink-emblem.png
            DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/128x128/apps"
            RENAME jaglink.png)
    install(FILES app/linux/com.github.The-First-Infiltrator.Jaglink.desktop
            DESTINATION "${CMAKE_INSTALL_DATADIR}/applications")
    install(FILES README.md docs/JAGUAR.md docs/DISCOVER.md
            DESTINATION "${CMAKE_INSTALL_DOCDIR}")
endif()

if(JAGLINK_BUILD_WINDOWS_DISCOVER)
    if(NOT WIN32)
        message(FATAL_ERROR "JAGLINK_BUILD_WINDOWS_DISCOVER is supported on Windows only.")
    endif()
    link_add_windows_discover(jaglink-discover
        PRODUCT_NAME "JAGLINK"
        PRODUCT_SLUG "jaglink"
        WINDOW_CLASS "JAGLINKDiscoverWindow")
    target_sources(jaglink-discover PRIVATE app/windows/jaglink.rc)
    target_include_directories(jaglink-discover PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/app/windows")
endif()

function(jaglink_add_test target source)
    add_executable(${target} ${source})
    target_link_libraries(${target} PRIVATE jaglink)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
        target_compile_options(${target} PRIVATE ${JAGLINK_WARNING_FLAGS} -Werror)
    endif()
    add_test(NAME ${target} COMMAND ${target})
endfunction()

if(BUILD_TESTING)
    jaglink_add_test(jaglink-test-core tests/test_core.c)
    target_compile_definitions(jaglink-test-core PRIVATE JAGLINK_TEST_EXPECTED_VERSION="${PROJECT_VERSION}")
    jaglink_add_test(jaglink-test-elm327 tests/test_elm327.c)
    jaglink_add_test(jaglink-test-elm327-can tests/test_elm327_can.c)
    jaglink_add_test(jaglink-test-elm327-session tests/test_elm327_session.c)
    jaglink_add_test(jaglink-test-elm327-probe tests/test_elm327_probe.c)
    jaglink_add_test(jaglink-test-obd2 tests/test_obd2.c)
    jaglink_add_test(jaglink-test-runtime tests/test_runtime.c)
    jaglink_add_test(jaglink-test-parameter tests/test_parameter.c)
    jaglink_add_test(jaglink-test-parameter-store tests/test_parameter_store.c)
    jaglink_add_test(jaglink-test-scheduler-parameters tests/test_scheduler_parameters.c)
    jaglink_add_test(jaglink-test-isotp tests/test_isotp.c)
    jaglink_add_test(jaglink-test-isotp-failed-state tests/test_isotp_failed_state.c)
    jaglink_add_test(jaglink-test-isotp-functional-response tests/test_isotp_functional_response.c)
    jaglink_add_test(jaglink-test-uds tests/test_uds.c)
    jaglink_add_test(jaglink-test-uds-dtc tests/test_uds_dtc.c)
    jaglink_add_test(jaglink-test-jaguar tests/test_jaguar.c)
    jaglink_add_test(jaglink-test-discover-safety tests/test_discover_safety.c)
    jaglink_add_test(jaglink-test-evidence tests/test_evidence.c)
endif()

set(CPACK_PACKAGE_NAME "jaglink")
set(CPACK_PACKAGE_VENDOR "JAGLINK Project")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Jaguar X-Type X400 diagnostics platform")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "JAGLINK Project")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libgtk-4-1 (>= 4.6)")
set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
include(CPack)
