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

if(JAGLINK_ENABLE_SANITIZERS)
    if(NOT CMAKE_C_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
        message(FATAL_ERROR "JAGLINK_ENABLE_SANITIZERS requires GCC or Clang.")
    endif()
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()

include(CTest)

set(JAGLINK_MBLINK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/upstream/mblink")
set(JAGLINK_MBLINK_COMMIT "e760b6ec05897c87e3531d68649b121403fcdec8")
set(JAGLINK_COMMON_DIR "${JAGLINK_MBLINK_DIR}/src/infiltratr-common")
set(JAGLINK_COMMON_VERSION "1.10.0")
set(JAGLINK_COMMON_COMMIT "182e64cb8b8992879e443b941565058166fe0161")

if(NOT EXISTS "${JAGLINK_MBLINK_DIR}/include/mblink/mblink.h")
    message(FATAL_ERROR "Pinned MBLINK source is required. Run git submodule update --init --recursive.")
endif()
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
    execute_process(COMMAND "${JAGLINK_GIT_EXECUTABLE}" -C "${JAGLINK_MBLINK_DIR}" rev-parse HEAD
        OUTPUT_VARIABLE JAGLINK_MBLINK_FOUND_COMMIT OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET RESULT_VARIABLE JAGLINK_MBLINK_COMMIT_RESULT)
    if(JAGLINK_MBLINK_COMMIT_RESULT EQUAL 0 AND NOT JAGLINK_MBLINK_FOUND_COMMIT STREQUAL JAGLINK_MBLINK_COMMIT)
        message(FATAL_ERROR "MBLINK must be pinned to ${JAGLINK_MBLINK_COMMIT}; found ${JAGLINK_MBLINK_FOUND_COMMIT}.")
    endif()
    execute_process(COMMAND "${JAGLINK_GIT_EXECUTABLE}" -C "${JAGLINK_COMMON_DIR}" rev-parse HEAD
        OUTPUT_VARIABLE JAGLINK_COMMON_FOUND_COMMIT OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET RESULT_VARIABLE JAGLINK_COMMON_COMMIT_RESULT)
    if(JAGLINK_COMMON_COMMIT_RESULT EQUAL 0 AND NOT JAGLINK_COMMON_FOUND_COMMIT STREQUAL JAGLINK_COMMON_COMMIT)
        message(FATAL_ERROR "Infiltratr Common must be pinned to ${JAGLINK_COMMON_COMMIT}; found ${JAGLINK_COMMON_FOUND_COMMIT}.")
    endif()
endif()

set(INFILTRATR_COMMON_BUILD_TESTS OFF CACHE BOOL "Build Infiltratr Common tests" FORCE)
set(INFILTRATR_COMMON_WARNINGS_AS_ERRORS ON CACHE BOOL "Treat Infiltratr Common warnings as errors" FORCE)
add_subdirectory("${JAGLINK_COMMON_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/infiltratr-common" EXCLUDE_FROM_ALL)

add_library(jaglink STATIC
    src/core/mblink.c
    src/jaguar/jaguar.c
    ${JAGLINK_MBLINK_DIR}/src/core/parameter.c
    ${JAGLINK_MBLINK_DIR}/src/core/scheduler.c
    ${JAGLINK_MBLINK_DIR}/src/core/telemetry_store.c
    ${JAGLINK_MBLINK_DIR}/src/core/telemetry_csv.c
    ${JAGLINK_MBLINK_DIR}/src/core/isotp.c
    ${JAGLINK_MBLINK_DIR}/src/elm327/elm327.c
    ${JAGLINK_MBLINK_DIR}/src/elm327/can.c
    ${JAGLINK_MBLINK_DIR}/src/elm327/session.c
    ${JAGLINK_MBLINK_DIR}/src/elm327/probe.c
    ${JAGLINK_MBLINK_DIR}/src/obd2/obd2.c
    ${JAGLINK_MBLINK_DIR}/src/uds/uds.c)

target_include_directories(jaglink PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${JAGLINK_MBLINK_DIR}/include")
target_compile_definitions(jaglink PRIVATE MBLINK_VERSION="${PROJECT_VERSION}")
target_link_libraries(jaglink PUBLIC InfiltratrCommon::Portable)

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
    target_link_libraries(jaglink-linux PRIVATE jaglink PkgConfig::GTK4)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(jaglink-linux PRIVATE ${JAGLINK_WARNING_FLAGS} -Werror)
    endif()
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
    jaglink_add_test(jaglink-test-core ${JAGLINK_MBLINK_DIR}/tests/test_core.c)
    target_compile_definitions(jaglink-test-core PRIVATE MBLINK_TEST_EXPECTED_VERSION="${PROJECT_VERSION}")
    jaglink_add_test(jaglink-test-elm327 ${JAGLINK_MBLINK_DIR}/tests/test_elm327.c)
    jaglink_add_test(jaglink-test-elm327-can ${JAGLINK_MBLINK_DIR}/tests/test_elm327_can.c)
    jaglink_add_test(jaglink-test-elm327-session ${JAGLINK_MBLINK_DIR}/tests/test_elm327_session.c)
    jaglink_add_test(jaglink-test-elm327-probe ${JAGLINK_MBLINK_DIR}/tests/test_elm327_probe.c)
    jaglink_add_test(jaglink-test-obd2 ${JAGLINK_MBLINK_DIR}/tests/test_obd2.c)
    jaglink_add_test(jaglink-test-runtime ${JAGLINK_MBLINK_DIR}/tests/test_runtime.c)
    jaglink_add_test(jaglink-test-parameter ${JAGLINK_MBLINK_DIR}/tests/test_parameter.c)
    jaglink_add_test(jaglink-test-parameter-store ${JAGLINK_MBLINK_DIR}/tests/test_parameter_store.c)
    jaglink_add_test(jaglink-test-scheduler-parameters ${JAGLINK_MBLINK_DIR}/tests/test_scheduler_parameters.c)
    jaglink_add_test(jaglink-test-isotp ${JAGLINK_MBLINK_DIR}/tests/test_isotp.c)
    jaglink_add_test(jaglink-test-isotp-failed-state ${JAGLINK_MBLINK_DIR}/tests/test_isotp_failed_state.c)
    jaglink_add_test(jaglink-test-isotp-functional-response ${JAGLINK_MBLINK_DIR}/tests/test_isotp_functional_response.c)
    jaglink_add_test(jaglink-test-uds ${JAGLINK_MBLINK_DIR}/tests/test_uds.c)
    jaglink_add_test(jaglink-test-uds-dtc ${JAGLINK_MBLINK_DIR}/tests/test_uds_dtc.c)
    jaglink_add_test(jaglink-test-jaguar tests/test_jaguar.c)
endif()
