# SPDX-License-Identifier: MPL-2.0
cmake_minimum_required(VERSION 3.16)
get_filename_component(LIBCORE_REPLACEMENT_DIR "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
set(EXTRA_COMPONENT_DIRS "${LIBCORE_REPLACEMENT_DIR}")
# Explicit selection also supports applications that restrict COMPONENTS.
set(COMPONENTS main libcore)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(libcore_replacement_example)
include("${LIBCORE_REPLACEMENT_DIR}/cmake/replace_core.cmake")
