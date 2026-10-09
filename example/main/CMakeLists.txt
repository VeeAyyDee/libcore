# SPDX-License-Identifier: MPL-2.0
idf_component_register(SRCS "main.c" REQUIRES libcore esp_wifi nvs_flash esp_event esp_netif)
