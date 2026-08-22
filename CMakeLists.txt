# SPDX-License-Identifier: GPL-3.0-or-later
cmake_minimum_required(VERSION 3.20)

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" JAGLINK_PROJECT_VERSION)
string(STRIP "${JAGLINK_PROJECT_VERSION}" JAGLINK_PROJECT_VERSION)
if(NOT JAGLINK_PROJECT_VERSION MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+$")
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

set(JAGLINK_COMMON_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/infiltratr-common")
set(JAGLINK_COMMON_VERSION "1.10.0")
set(JAGLINK_COMMON_COMMIT "182e64cb8b8992879e443b941565058166fe0161")

if(NOT EXISTS "${JAGLINK_COMMON_DIR}/VERSION")
    message(FATAL_ERROR "Infiltratr Common ${JAGLINK_COMMON_VERSION} is required. Run git submodule update --init --recursive.")
endif()

file(READ "${JAGLINK_COMMON_DIR}/VERSION" JAGLINK_COMMON_FOUND_VERSION)
string(STRIP "${JAGLINK_COMMON_FOUND_VERSION}" JAGLINK_COMMON_FOUND_VERSION)
if(NOT JAGLINK_COMMON_FOUND_VERSION STREQUAL JAGLINK_COMMON_VERSION)
    message(FATAL_ERROR "Infiltratr Common ${JAGLINK_COMMON_VERSION} is required; found ${JAGLINK_COMMON_FOUND_VERSION}.")
endif()

find_program(JAGLINK_GIT_EXECUTABLE git)
if(JAGLINK_GIT_EXECUTABLE)
    execute_process(COMMAND "${JAGLINK_GIT_EXECUTABLE}" -C "${JAGLINK_COMMON_DIR}" rev-parse HEAD
        OUTPUT_VARIABLE JAGLINK_COMMON_FOUND_COMMIT OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET RESULT_VARIABLE JAGLINK_COMMON_COMMIT_RESULT)
    if(JAGLINK_COMMON_COMMIT_RESULT EQUAL 0 AND NOT JAGLINK_COMMON_FOUND_COMMIT STREQUAL JAGLINK_COMMON_COMMIT)
        message(FATAL_ERROR "Infiltratr Common must be pinned to ${JAGLINK_COMMON_COMMIT}; found ${JAGLINK_COMMON_FOUND_COMMIT}.")
    endif()
endif()

set(INFILTRATR_COMMON_BUILD_TESTS OFF CACHE BOOL "Build Infiltratr Common tests" FORCE)
set(INFILTRATR_COMMON_WARNINGS_AS_ERRORS ON CACHE BOOL "Treat Infiltratr Common warnings as errors" FORCE)
add_subdirectory("${JAGLINK_COMMON_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/infiltratr-common" EXCLUDE_FROM_ALL)

set(JAGLINK_LINK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/link")
set(JAGLINK_LINK_VERSION "0.1.0")
set(JAGLINK_LINK_COMMIT "8df05f3c5a1e7239a3841a4e028e2b6849f7ad31")

if(NOT EXISTS "${JAGLINK_LINK_DIR}/VERSION")
    message(FATAL_ERROR "LINK ${JAGLINK_LINK_VERSION} is required. Run git submodule update --init --recursive.")
endif()

file(READ "${JAGLINK_LINK_DIR}/VERSION" JAGLINK_LINK_FOUND_VERSION)
string(STRIP "${JAGLINK_LINK_FOUND_VERSION}" JAGLINK_LINK_FOUND_VERSION)
if(NOT JAGLINK_LINK_FOUND_VERSION STREQUAL JAGLINK_LINK_VERSION)
    message(FATAL_ERROR "LINK ${JAGLINK_LINK_VERSION} is required; found ${JAGLINK_LINK_FOUND_VERSION}.")
endif()

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
    src/core/isotp.c
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
target_link_libraries(jaglink PUBLIC InfiltratrCommon::Portable LINK::Core)

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
    add_executable(jaglink-linux app/linux/main.c)
    set_target_properties(jaglink-linux PROPERTIES OUTPUT_NAME jaglink)
    target_link_libraries(jaglink-linux PRIVATE jaglink PkgConfig::GTK4)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(jaglink-linux PRIVATE ${JAGLINK_WARNING_FLAGS} -Werror)
    endif()
    install(TARGETS jaglink-linux RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
    install(FILES README.md docs/JAGUAR.md docs/DISCOVER.md
            DESTINATION "${CMAKE_INSTALL_DOCDIR}")
endif()

if(JAGLINK_BUILD_WINDOWS_DISCOVER)
    if(NOT WIN32)
        message(FATAL_ERROR "JAGLINK_BUILD_WINDOWS_DISCOVER is supported on Windows only.")
    endif()
    add_executable(jaglink-discover WIN32 platform/windows/jaglink-discover.c)
    target_link_libraries(jaglink-discover PRIVATE jaglink comdlg32 advapi32)
    target_compile_definitions(jaglink-discover PRIVATE _CRT_SECURE_NO_WARNINGS)
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
