/* SPDX-License-Identifier: MPL-2.0 */
#include "platform.h"
#include "esp_idf_version.h"
#include "sdkconfig.h"
#include "esp_private/wifi_os_adapter.h"

#if !defined(CONFIG_IDF_TARGET_ESP32S3) || !CONFIG_IDF_TARGET_ESP32S3
#error "This libcore replacement supports ESP32-S3 only"
#endif
#if ESP_IDF_VERSION != ESP_IDF_VERSION_VAL(6, 0, 3)
#error "This libcore replacement requires ESP-IDF 6.0.3"
#endif

_Static_assert(sizeof(void *) == 4, "Target must have 32-bit pointers");
_Static_assert(sizeof(uint32_t) == 4, "Target must have 32-bit words");
_Static_assert(offsetof(wifi_osi_funcs_t, _malloc_internal) == 0x158,
               "SDK allocation callback offset changed");
_Static_assert(offsetof(wifi_osi_funcs_t, _free) == 0xb0,
               "SDK free callback offset changed");
_Static_assert(offsetof(wifi_osi_funcs_t, _nvs_open) == 0x124,
               "SDK NVS open callback offset changed");
_Static_assert(offsetof(wifi_osi_funcs_t, _nvs_close) == 0x128,
               "SDK NVS close callback offset changed");

extern wifi_osi_funcs_t *g_osi_funcs_p;
extern uint32_t g_wifi_menuconfig[];

bool libcore_platform_nvs_enabled(void)
{
    return g_wifi_menuconfig[9] != 0;
}

void *libcore_platform_allocate(size_t size)
{
    return g_osi_funcs_p->_malloc_internal(size);
}

void libcore_platform_release(void *allocation)
{
    g_osi_funcs_p->_free(allocation);
}

int libcore_platform_open(uint32_t *handle)
{
    return g_osi_funcs_p->_nvs_open("misc", 1u, handle);
}

void libcore_platform_close(uint32_t handle)
{
    g_osi_funcs_p->_nvs_close(handle);
}
