/* SPDX-License-Identifier: MPL-2.0 */
#ifndef CLEANROOM_LIBCORE_PLATFORM_H
#define CLEANROOM_LIBCORE_PLATFORM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Implemented by platform_idf.c, or by the independent host test platform. */
bool libcore_platform_nvs_enabled(void);
void *libcore_platform_allocate(size_t size);
void libcore_platform_release(void *allocation);
int libcore_platform_open(uint32_t *handle);
void libcore_platform_close(uint32_t handle);

#endif
