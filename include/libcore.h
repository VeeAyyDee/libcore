/* SPDX-License-Identifier: MPL-2.0 */
#ifndef CLEANROOM_LIBCORE_H
#define CLEANROOM_LIBCORE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Exported ABI. The owned record is 60 bytes; see README.md for its layout. */
extern void *g_misc_nvs;
extern uint32_t g_log_mod[6];
extern uint32_t g_log_level;
extern const char *libcore_reversion_git;

int misc_nvs_load(uint8_t mode);
int misc_nvs_init(void);
void misc_nvs_deinit(void);
int misc_nvs_restore(void);

#ifdef __cplusplus
}
#endif
#endif
