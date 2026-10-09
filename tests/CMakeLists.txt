# SPDX-License-Identifier: MPL-2.0
cmake_minimum_required(VERSION 3.16)
project(libcore_host_tests C)
enable_testing()
option(LIBCORE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" ON)
add_executable(libcore_host_tests test_lifecycle.c ../src/misc_nvs.c)
target_include_directories(libcore_host_tests PRIVATE ../include ../src)
target_compile_features(libcore_host_tests PRIVATE c_std_11)
target_compile_options(libcore_host_tests PRIVATE
    -Wall -Wextra -Werror -Wpedantic -Wconversion -Wshadow
    -Wstrict-prototypes -Wmissing-prototypes -Wformat=2)
if(LIBCORE_SANITIZERS)
    target_compile_options(libcore_host_tests PRIVATE
        -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(libcore_host_tests PRIVATE -fsanitize=address,undefined)
endif()
add_test(NAME libcore_lifecycle COMMAND libcore_host_tests)
add_test(NAME libcore_cmake_hook
    COMMAND ${CMAKE_COMMAND} -S ${CMAKE_CURRENT_SOURCE_DIR}/cmake-hook
                            -B "${CMAKE_CURRENT_BINARY_DIR}/cmake hook with spaces")
