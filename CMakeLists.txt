# SPDX-License-Identifier: MPL-2.0
idf_component_register(SRCS "src/misc_nvs.c" "src/platform_idf.c"
                       INCLUDE_DIRS "include"
                       PRIV_INCLUDE_DIRS "src"
                       REQUIRES esp_wifi)
# Component name is libcore, while the required archive name is libcore.a.
set_target_properties(${COMPONENT_LIB} PROPERTIES OUTPUT_NAME core)
target_compile_features(${COMPONENT_LIB} PRIVATE c_std_11)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Werror)
