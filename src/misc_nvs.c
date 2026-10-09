/* SPDX-License-Identifier: MPL-2.0 */
#include "libcore.h"
#include "platform.h"

#include <stddef.h>
#include <string.h>

enum {
    LIBCORE_NO_MEMORY = 0x101,
    LIBCORE_INVALID_STATE = 0x103
};

typedef struct {
    uint8_t reserved[8];
    uint32_t reset;
    uint8_t opaque[44];
    uint32_t handle;
} misc_nvs_record;

_Static_assert(sizeof(misc_nvs_record) == 60, "NVS record size");
_Static_assert(offsetof(misc_nvs_record, reset) == 8, "Reset word offset");
_Static_assert(offsetof(misc_nvs_record, handle) == 56, "NVS handle offset");

void *g_misc_nvs;
uint32_t g_log_mod[6];
uint32_t g_log_level;
const char *libcore_reversion_git = "cleanroom-libcore-implementation-01";

static bool initialized;
/* Ownership is independent of configuration and consumer-visible bytes. */
static uint32_t owned_handle;

int misc_nvs_restore(void)
{
    if (g_misc_nvs == NULL) {
        return LIBCORE_INVALID_STATE;
    }
    ((misc_nvs_record *)g_misc_nvs)->reset = 0;
    return 0;
}

int misc_nvs_load(uint8_t mode)
{
    if (mode != 0) {
        return misc_nvs_restore();
    }
    if (g_misc_nvs != NULL) {
        return LIBCORE_INVALID_STATE;
    }

    misc_nvs_record *record = libcore_platform_allocate(sizeof(*record));
    if (record == NULL) {
        return LIBCORE_NO_MEMORY;
    }
    memset(record, 0, sizeof(*record));

    if (libcore_platform_nvs_enabled()) {
        int result = libcore_platform_open(&record->handle);
        if (result != 0) {
            if (record->handle != 0) {
                libcore_platform_close(record->handle);
            }
            libcore_platform_release(record);
            return result;
        }
    }

    owned_handle = record->handle;
    g_misc_nvs = record;
    return 0;
}

int misc_nvs_init(void)
{
    if (initialized) {
        return 0;
    }
    int result = misc_nvs_load(0);
    if (result == 0) {
        initialized = true;
    }
    return result;
}

void misc_nvs_deinit(void)
{
    if (g_misc_nvs != NULL) {
        if (owned_handle != 0) {
            libcore_platform_close(owned_handle);
        }
        libcore_platform_release(g_misc_nvs);
    }
    g_misc_nvs = NULL;
    owned_handle = 0;
    initialized = false;
}
